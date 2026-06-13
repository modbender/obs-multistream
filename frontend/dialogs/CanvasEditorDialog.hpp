#pragma once

#include <utility/CanvasDefinition.hpp>

#include <QDialog>

class QLineEdit;
class QComboBox;
class QSpinBox;
class OBSBasic;

/* Modal editor for a single CanvasDefinition. Holds a reference to the live
 * manager entry and writes back into it only on accept(); cancel writes nothing. */
class CanvasEditorDialog : public QDialog {
	Q_OBJECT

public:
	CanvasEditorDialog(CanvasDefinition &def, OBSBasic *main, QWidget *parent);

private:
	void BuildUI();
	void ReadBack();

	CanvasDefinition &def;
	OBSBasic *main;

	QLineEdit *nameEdit = nullptr;
	QComboBox *resCombo = nullptr;
	QSpinBox *fpsNum = nullptr;
	QSpinBox *fpsDen = nullptr;
};
