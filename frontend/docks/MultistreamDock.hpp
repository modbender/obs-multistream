#pragma once

#include <QFrame>

class OBSBasic;
class QVBoxLayout;
class QShowEvent;

/* Read-only control panel for the fan-out streaming engine. Lists every output
 * binding grouped by canvas with live status, exposes per-canvas cascade
 * toggles and per-binding enable toggles, and a master Go Live / Stop All. It
 * never edits bindings or profiles — that is done in Settings -> Outputs. */
class MultistreamDock : public QFrame {
	Q_OBJECT

public:
	explicit MultistreamDock(OBSBasic *main, QWidget *parent = nullptr);
	~MultistreamDock() override;

	/* Rebuild the whole list from the engine's current statuses. Safe to call
	 * from the engine's onStatusChanged callback (Qt main thread). */
	void Refresh();

protected:
	void showEvent(QShowEvent *event) override;

private:
	OBSBasic *main = nullptr;
	QVBoxLayout *groupsLayout = nullptr; // inside the scroll area; one entry per canvas
	bool refreshing = false;             // guards programmatic setChecked from firing Start/Stop
};
