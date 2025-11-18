#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog();

    bool getAutoJumpChecked() const;
    void setAutoJumpChecked(bool checked);

private:
    Ui::SettingsDialog* ui;
};

#endif // SETTINGSDIALOG_H