#include "CanvasDock.hpp"

#include <components/Multiview.hpp>
#include <dialogs/NameDialog.hpp>
#include <utility/display-helpers.hpp>
#include <widgets/OBSBasic.hpp>
#include <widgets/OBSQTDisplay.hpp>

#include <qt-wrappers.hpp>

#include <QHBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QVBoxLayout>

#include <cstring>

#include "moc_CanvasDock.cpp"

namespace {
constexpr uint32_t kPreviewBackground = 0x000000;
constexpr uint32_t kFallbackWidth = 1920;
constexpr uint32_t kFallbackHeight = 1080;
} // namespace

CanvasDock::CanvasDock(OBSBasic *main_, OBSCanvas canvas_, QWidget *parent)
	: OBSDock(parent),
	  main(main_),
	  canvas(canvas_)
{
	const char *uuid = obs_canvas_get_uuid(canvas);
	canvasUuid = uuid ? uuid : "";

	setObjectName(QStringLiteral("canvasDock_") + QString::fromStdString(canvasUuid));

	const char *name = obs_canvas_get_name(canvas);
	QString title = QString::fromUtf8(name ? name : "");
	struct obs_video_info ovi;
	if (obs_canvas_get_video_info(canvas, &ovi)) {
		title += QString(" (%1x%2)").arg(ovi.base_width).arg(ovi.base_height);
	}
	setWindowTitle(title);

	sceneList = new QListWidget();
	sceneList->setContextMenuPolicy(Qt::CustomContextMenu);

	QWidget *sceneContainer = new QWidget();
	QVBoxLayout *sceneLayout = new QVBoxLayout(sceneContainer);
	sceneLayout->setContentsMargins(0, 0, 0, 0);
	sceneLayout->addWidget(sceneList, 1);

	QHBoxLayout *sceneButtons = new QHBoxLayout();
	QPushButton *addButton = new QPushButton(QTStr("Add"));
	QPushButton *removeButton = new QPushButton(QTStr("Remove"));
	sceneButtons->addWidget(addButton);
	sceneButtons->addWidget(removeButton);
	sceneButtons->addStretch();
	sceneLayout->addLayout(sceneButtons);

	preview = new OBSQTDisplay();

	splitter = new QSplitter();
	splitter->addWidget(sceneContainer);
	splitter->addWidget(preview);
	splitter->setStretchFactor(1, 1);
	setWidget(splitter);

	connect(addButton, &QPushButton::clicked, this, &CanvasDock::AddScene);
	connect(removeButton, &QPushButton::clicked, this, &CanvasDock::RemoveScene);
	connect(sceneList, &QListWidget::itemSelectionChanged, this, &CanvasDock::OnSceneSelected);
	connect(sceneList, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		QMenu menu(this);
		menu.addAction(QTStr("Add"), this, &CanvasDock::AddScene);
		if (sceneList->currentItem()) {
			menu.addAction(QTStr("Rename"), this, &CanvasDock::RenameScene);
			menu.addAction(QTStr("Remove"), this, &CanvasDock::RemoveScene);
		}
		menu.exec(sceneList->mapToGlobal(pos));
	});
	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(preview->GetDisplay(), OBSRender, this);
		obs_display_set_background_color(preview->GetDisplay(), kPreviewBackground);
	};
	connect(preview, &OBSQTDisplay::DisplayCreated, this, addDrawCallback);

	/* Scene add/remove and current-scene changes reflect even when triggered
	 * outside this dock (scene-link sync, Settings edits). */
	signal_handler_t *handler = obs_canvas_get_signal_handler(canvas);
	sigs.emplace_back(handler, "source_add", SceneListChanged, this);
	sigs.emplace_back(handler, "source_remove", SceneListChanged, this);
	sigs.emplace_back(handler, "channel_change", ChannelChanged, this);

	RefreshScenes();
	ApplyOrientation();

	ready = true;
}

CanvasDock::~CanvasDock()
{
	/* Disconnect the canvas signals first: they fire on core/graphics threads
	 * with `this` as param, so leaving them live during teardown is a UAF. */
	sigs.clear();

	/* libobs blocks until the graphics thread is out of the callback, so the
	 * canvas ref is safe to drop after this returns. */
	obs_display_remove_draw_callback(preview->GetDisplay(), OBSRender, this);
}

void CanvasDock::OBSRender(void *data, uint32_t cx, uint32_t cy)
{
	CanvasDock *window = static_cast<CanvasDock *>(data);

	if (!window->ready) {
		return;
	}

	uint32_t targetCX = kFallbackWidth;
	uint32_t targetCY = kFallbackHeight;
	struct obs_video_info ovi;
	if (obs_canvas_get_video_info(window->canvas, &ovi)) {
		targetCX = std::max(ovi.base_width, 1u);
		targetCY = std::max(ovi.base_height, 1u);
	}

	int x, y;
	float scale;
	GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);

	int newCX = int(scale * float(targetCX));
	int newCY = int(scale * float(targetCY));

	startRegion(x, y, newCX, newCY, 0.0f, float(targetCX), 0.0f, float(targetCY));
	obs_canvas_render(window->canvas);
	endRegion();
}

void CanvasDock::SceneListChanged(void *data, calldata_t *)
{
	CanvasDock *window = static_cast<CanvasDock *>(data);
	QMetaObject::invokeMethod(window, "RefreshScenes", Qt::QueuedConnection);
}

void CanvasDock::ChannelChanged(void *data, calldata_t *params)
{
	long long channel = calldata_int(params, "channel");
	if (channel != 0) {
		return;
	}
	CanvasDock *window = static_cast<CanvasDock *>(data);
	QMetaObject::invokeMethod(window, "RefreshScenes", Qt::QueuedConnection);
}

void CanvasDock::RefreshScenes()
{
	refreshing = true;
	sceneList->clear();

	OBSSource current = main->GetCanvasCurrentScene(canvas);
	const char *currentUuid = current ? obs_source_get_uuid(current) : nullptr;

	struct Ctx {
		QListWidget *list;
		const char *currentUuid;
		int currentRow;
	} ctx{sceneList, currentUuid, -1};

	obs_canvas_enum_scenes(
		canvas,
		[](void *param, obs_source_t *scene) {
			auto *c = static_cast<Ctx *>(param);
			const char *name = obs_source_get_name(scene);
			const char *uuid = obs_source_get_uuid(scene);
			QListWidgetItem *item = new QListWidgetItem(QString::fromUtf8(name ? name : ""), c->list);
			item->setData(Qt::UserRole, QString::fromUtf8(uuid ? uuid : ""));
			if (c->currentUuid && uuid && strcmp(c->currentUuid, uuid) == 0) {
				c->currentRow = c->list->row(item);
			}
			return true;
		},
		&ctx);

	if (ctx.currentRow >= 0) {
		sceneList->setCurrentRow(ctx.currentRow);
	}

	refreshing = false;
}

void CanvasDock::OnSceneSelected()
{
	if (refreshing) {
		return;
	}

	QListWidgetItem *item = sceneList->currentItem();
	if (!item) {
		return;
	}

	std::string uuid = item->data(Qt::UserRole).toString().toStdString();
	obs_source_t *scene = main->FindCanvasSceneByUuid(canvas, uuid);
	if (scene) {
		main->SetCanvasCurrentScene(canvas, scene);
	}
}

void CanvasDock::AddScene()
{
	std::string name;
	if (!NameDialog::AskForName(this, QTStr("Basic.Main.AddSceneDlg.Title"),
				    QTStr("Basic.Main.AddSceneDlg.Text"), name)) {
		return;
	}
	if (name.empty()) {
		return;
	}

	OBSSceneAutoRelease scene = obs_canvas_scene_create(canvas, name.c_str());
	if (scene) {
		main->SetCanvasCurrentScene(canvas, obs_scene_get_source(scene));
	}
	RefreshScenes();
}

void CanvasDock::RenameScene()
{
	QListWidgetItem *item = sceneList->currentItem();
	if (!item) {
		return;
	}

	std::string uuid = item->data(Qt::UserRole).toString().toStdString();
	std::string name = item->text().toStdString();
	if (!NameDialog::AskForName(this, QTStr("Basic.Main.AddSceneDlg.Title"),
				    QTStr("Basic.Main.AddSceneDlg.Text"), name)) {
		return;
	}
	if (name.empty()) {
		return;
	}

	/* Re-resolve after the modal: the event loop spun while it was open, so the
	 * scene found before it could have been removed in the meantime. */
	obs_source_t *scene = main->FindCanvasSceneByUuid(canvas, uuid);
	if (!scene) {
		return;
	}

	obs_source_set_name(scene, name.c_str());
	RefreshScenes();
}

void CanvasDock::RemoveScene()
{
	QListWidgetItem *item = sceneList->currentItem();
	if (!item) {
		return;
	}

	std::string uuid = item->data(Qt::UserRole).toString().toStdString();
	obs_source_t *scene = main->FindCanvasSceneByUuid(canvas, uuid);
	if (!scene) {
		return;
	}

	obs_source_remove(scene);
	RefreshScenes();
}

void CanvasDock::resizeEvent(QResizeEvent *event)
{
	OBSDock::resizeEvent(event);
	ApplyOrientation();
}

void CanvasDock::ApplyOrientation()
{
	if (!splitter) {
		return;
	}

	QWidget *content = widget();
	if (!content) {
		return;
	}

	/* Orientation follows the dock's shape; the splitter handle stays
	 * user-draggable for sizing within a given orientation. */
	Qt::Orientation wanted = content->height() > content->width() ? Qt::Vertical : Qt::Horizontal;
	if (splitter->orientation() != wanted) {
		splitter->setOrientation(wanted);
	}
}
