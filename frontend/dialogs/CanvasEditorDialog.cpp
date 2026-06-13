#include "CanvasEditorDialog.hpp"

#include <widgets/OBSBasic.hpp>
#include <qt-wrappers.hpp>

#include <cstdio>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

#include "moc_CanvasEditorDialog.cpp"

CanvasEditorDialog::CanvasEditorDialog(CanvasDefinition &def_, OBSBasic *main_, QWidget *parent)
	: QDialog(parent),
	  def(def_),
	  main(main_)
{
	setModal(true);
	setWindowTitle(def.isDefault ? QTStr("Basic.Settings.Canvas.Editor.TitleDefault")
				     : QTStr("Basic.Settings.Canvas.Editor.Title"));
	BuildUI();
}

void CanvasEditorDialog::BuildUI()
{
	QVBoxLayout *root = new QVBoxLayout(this);

	QFormLayout *form = new QFormLayout();

	nameEdit = new QLineEdit(QString::fromStdString(def.name));
	nameEdit->setEnabled(!def.isDefault);
	form->addRow(QTStr("Basic.Settings.Canvas.Editor.Name"), nameEdit);

	resCombo = new QComboBox();
	resCombo->setEditable(true);
	resCombo->addItem("1920x1080");
	resCombo->addItem("1280x720");
	resCombo->addItem("1080x1920");
	resCombo->addItem("720x1280");
	resCombo->setCurrentText(QString("%1x%2").arg(def.width).arg(def.height));
	form->addRow(QTStr("Basic.Settings.Canvas.Editor.Resolution"), resCombo);

	QWidget *fpsWidget = new QWidget();
	QHBoxLayout *fpsLayout = new QHBoxLayout(fpsWidget);
	fpsLayout->setContentsMargins(0, 0, 0, 0);

	fpsNum = new QSpinBox();
	fpsNum->setRange(1, 1000000);
	fpsNum->setValue((int)def.fpsNum);

	fpsDen = new QSpinBox();
	fpsDen->setRange(1, 1000000);
	fpsDen->setValue((int)def.fpsDen);

	fpsLayout->addWidget(fpsNum);
	fpsLayout->addWidget(new QLabel("/"));
	fpsLayout->addWidget(fpsDen);
	fpsLayout->addStretch();
	form->addRow(QTStr("Basic.Settings.Canvas.Editor.FPS"), fpsWidget);

	root->addLayout(form);

	/* Tasks 2-5 insert the encoder/color QTabWidget here, before the buttons. */

	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
		ReadBack();
		accept();
	});
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	root->addWidget(buttons);
}

void CanvasEditorDialog::ReadBack()
{
	if (!def.isDefault) {
		def.name = QT_TO_UTF8(nameEdit->text());
	}

	std::string res = QT_TO_UTF8(resCombo->currentText());
	unsigned cx = 0, cy = 0;
	if (sscanf(res.c_str(), "%ux%u", &cx, &cy) == 2 && cx > 0 && cy > 0) {
		def.width = cx;
		def.height = cy;
	}

	def.fpsNum = (uint32_t)fpsNum->value();
	def.fpsDen = (uint32_t)fpsDen->value();
}
