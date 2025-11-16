#include <QApplication>
#include <QIcon>
#include "ui/MainWindow.h"

// Qt 5 需要 QTextCodec 设置编码，Qt 6 默认使用 UTF-8
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置应用信息
    app.setApplicationName("Schedule Manager");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("Student Schedule System");
    
    // 设置UTF-8编码支持（仅Qt5需要）
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    #endif
    
    app.setWindowIcon(QIcon(":/icons/icon.png"));

    // 创建并显示主窗口
    MainWindow mainWindow;
    mainWindow.setWindowIcon(QIcon(":/icons/icon.png"));
    mainWindow.show();
    
    return app.exec();
}

