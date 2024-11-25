#ifndef TCPFILESERVER_H
#define TCPFILESERVER_H
#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QDebug>
class TcpFileServer : public QWidget
{
    Q_OBJECT // 必須包含這個宏
public:
    explicit TcpFileServer(QWidget *parent = nullptr);
    ~TcpFileServer();
private slots:
    // 客戶端
    void openFile();
    void startSender();
    void startTransfer();
    void updateClientProgress(qint64 numBytes);
    // 伺服器
    void startServer();
    void acceptConnection();
    void updateServerProgress();
    void displayError1(QAbstractSocket::SocketError socketError);
private:
    // 客戶端屬性
    QProgressBar *clientProgressBar;
    QLabel *clientStatusLabel;
    QPushButton *startButton, *quitButton, *openButton;
    QLabel *URLLabel, *portLabel;
    QLineEdit *URLinput, *portinput;
    QDialogButtonBox *buttonBox;
    QTcpSocket tcpClient;
    QString fileName;
    QFile *localFile;
    QByteArray outBlock;
    qint64 totalBytes, bytesWritten, bytesToWrite, loadSize;
    // 伺服器屬性
    QTcpServer tcpServer;
    QTcpSocket *tcpServerConnection = nullptr;
    qint64 totalBytes1, byteReceived, fileNameSize;
    QString fileName1;
    QFile *localFile1;
    QByteArray inBlock;
    QProgressBar *serverProgressBar1;
    QLabel *serverStatusLabel1;
    QPushButton *startButton1, *quitButton1;
    QDialogButtonBox *buttonBox1;
    // 主界面
    QTabWidget *tabWidget;
};
#endif // TCPFILESERVER_H
