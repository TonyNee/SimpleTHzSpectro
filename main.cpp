#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SimpleSpectro");
    app.setApplicationVersion("1.0");

    // 设置暗色主题风格
    app.setStyleSheet(R"(
        QMainWindow { background-color: #2b2b2b; }
        QGroupBox { color: #ccc; font-weight: bold; border: 1px solid #555; border-radius: 4px; margin-top: 8px; padding-top: 12px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
        QLabel { color: #ccc; }
        QPushButton { background-color: #444; color: #eee; border: 1px solid #666; border-radius: 3px; padding: 4px 12px; }
        QPushButton:hover { background-color: #555; }
        QPushButton:pressed { background-color: #333; }
        QLineEdit { background-color: #3a3a3a; color: #eee; border: 1px solid #555; border-radius: 3px; padding: 3px 6px; }
        QSpinBox { background-color: #3a3a3a; color: #eee; border: 1px solid #555; border-radius: 3px; padding: 3px; }
        QTextEdit { background-color: #1a1a2e; color: #aaa; }
    )");

    MainWindow w;
    w.show();

    return app.exec();
}
