#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}

bool SettingsDialog::getAutoJumpChecked() const {
    return ui->autoJumpCheckBox->isChecked();
}

void SettingsDialog::setAutoJumpChecked(bool checked) {
    ui->autoJumpCheckBox->setChecked(checked);
}