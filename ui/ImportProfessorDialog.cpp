#include "ImportProfessorDialog.h"
#include "ui_ImportProfessorDialog.h"
#include <QFileDialog>

ImportProfessorDialog::ImportProfessorDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ImportProfessorDialog) {
    ui->setupUi(this);
}

ImportProfessorDialog::~ImportProfessorDialog() {
    delete ui;
}

QString ImportProfessorDialog::getFilePath() const {
    return ui->filePathEdit->text();
}

void ImportProfessorDialog::onBrowseClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8("选择教师CSV文件"),
        QString(),
        QString::fromUtf8("CSV文件 (*.csv);;所有文件 (*.*)")
    );

    if (!fileName.isEmpty()) {
        ui->filePathEdit->setText(fileName);
    }
}
