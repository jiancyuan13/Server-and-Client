#include <QApplication>
#include "tcpfileserver.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // 創建主窗口
    TcpFileServer mainWindow;
    mainWindow.setWindowTitle(QObject::tr("TCP File Server and Sender"));
    mainWindow.resize(800, 600); // 設置窗口大小
    mainWindow.show();          // 顯示主窗口
    return app.exec();          // 啟動應用程式事件循環
}
