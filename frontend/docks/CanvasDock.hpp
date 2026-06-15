#pragma once

#include <docks/OBSDock.hpp>

#include <obs.hpp>

#include <string>
#include <vector>

class OBSBasic;
class OBSQTDisplay;
class QListWidget;
class QListWidgetItem;
class QSplitter;
class QResizeEvent;

/* One dockable panel per additional canvas: a per-canvas scene list driven by
 * 2a's canvas scene model plus a live preview of that canvas's video mix. The
 * scene-list/preview split orientation follows the dock's shape so the preview
 * always gets usable area. */
class CanvasDock : public OBSDock {
	Q_OBJECT

public:
	CanvasDock(OBSBasic *main, OBSCanvas canvas, QWidget *parent = nullptr);
	~CanvasDock() override;

public slots:
	/* Repopulate the scene list from the canvas. Invoked (queued) from the
	 * canvas signal trampolines so external scene edits reflect. */
	void RefreshScenes();

protected:
	void resizeEvent(QResizeEvent *event) override;

private:
	static void OBSRender(void *data, uint32_t cx, uint32_t cy);

	static void SceneListChanged(void *data, calldata_t *params);
	static void ChannelChanged(void *data, calldata_t *params);

	void ApplyOrientation();
	void OnSceneSelected();
	void AddScene();
	void RenameScene();
	void RemoveScene();

	OBSBasic *main = nullptr;
	OBSCanvas canvas;
	std::string canvasUuid;

	OBSQTDisplay *preview = nullptr;
	QListWidget *sceneList = nullptr;
	QSplitter *splitter = nullptr;

	std::vector<OBSSignal> sigs;

	bool ready = false;      // guards the draw callback before the display is wired
	bool refreshing = false; // guards programmatic list rebuild from firing selection
};
