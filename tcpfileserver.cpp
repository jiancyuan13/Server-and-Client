#include "tcpfileserver.h"
TcpFileServer::TcpFileServer(QWidget *parent)
    : QWidget(parent)
{
    // 初始化各種元件（按鈕、標籤、進度條等）
    totalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;
    clientProgressBar = new QProgressBar;
    clientStatusLabel = new QLabel(QStringLiteral("客戶端就緒"));
    startButton = new QPushButton(QStringLiteral("開始"));
    quitButton = new QPushButton(QStringLiteral("退出"));
    openButton = new QPushButton(QStringLiteral("開檔"));
    URLLabel = new QLabel(QStringLiteral("URL:"));
    portLabel = new QLabel(QStringLiteral("port:"));
    URLinput = new QLineEdit;
    portinput  = new QLineEdit;
    URLinput->setText("127.0.0.1");  // 預設值
    portinput->setText("16998");  // 預設值
    tabWidget = new QTabWidget;
    // 佈局設計（按鈕、進度條等）
    QHBoxLayout *ipLayout = new QHBoxLayout;
    ipLayout->addWidget(URLLabel);
    ipLayout->addWidget(URLinput);
    QHBoxLayout *portLayout = new QHBoxLayout;
    portLayout->addWidget(portLabel);
    portLayout->addWidget(portinput);
    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(openButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);
    QWidget *senderPage = new QWidget;
    QVBoxLayout *senderLayout = new QVBoxLayout(senderPage);
    senderLayout->addLayout(ipLayout);
    senderLayout->addLayout(portLayout);
    senderLayout->addWidget(clientProgressBar);
    senderLayout->addWidget(clientStatusLabel);
    senderLayout->addStretch(1);
    senderLayout->addSpacing(10);
    senderLayout->addWidget(buttonBox);
    // 伺服器部分
    totalBytes1 = 0;
    byteReceived = 0;
    fileNameSize = 0;
    serverProgressBar1 = new QProgressBar;
    serverStatusLabel1 = new QLabel(QStringLiteral("伺服器端就緒"));
    startButton1 = new QPushButton(QStringLiteral("接收"));
    quitButton1 = new QPushButton(QStringLiteral("退出"));
    buttonBox1 = new QDialogButtonBox;
    buttonBox1->addButton(startButton1, QDialogButtonBox::ActionRole);
    buttonBox1->addButton(quitButton1,QDialogButtonBox::RejectRole);
    QWidget *serverPage = new QWidget;
    QVBoxLayout *serverLayout = new QVBoxLayout(serverPage);
    serverLayout->addWidget(serverProgressBar1);
    serverLayout->addWidget(serverStatusLabel1);
    serverLayout->addStretch();
    serverLayout->addWidget(buttonBox1);
    tabWidget->addTab(senderPage, "Sender");
    tabWidget->addTab(serverPage, "Server");
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
    // 信號與槽的連接
    connect(openButton, SIGNAL(clicked()), this, SLOT(openFile()));
    connect(startButton, SIGNAL(clicked()), this, SLOT(startSender()));  // 修改為 startSender()
    connect(&tcpClient, SIGNAL(connected()), this, SLOT(startTransfer()));
    connect(&tcpClient, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));

    connect(startButton1, SIGNAL(clicked()), this, SLOT(startServer()));  // 修改為 startServer()
    connect(quitButton1, SIGNAL(clicked()), this, SLOT(close()));
    connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
    connect(&tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)),this,
            SLOT(displayError(QAbstractSocket::SocketError)));
}
//客戶端
void TcpFileServer::openFile()
{
    fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) startButton->setEnabled(true);
}
void TcpFileServer::startSender()
{
    QString ipAddress = URLinput->text();
    bool ok;
    quint16 port = portinput->text().toUShort(&ok);

    if (ipAddress.isEmpty() || !ok) {
        QMessageBox::warning(this, QStringLiteral("錯誤"),
                             QStringLiteral("請輸入有效的 IP 位址與 Port！"));
        return;
    }
    qDebug() << "Connecting to " << ipAddress << " on port " << port;
    startButton->setEnabled(false);
    bytesWritten = 0;
    clientStatusLabel->setText(QStringLiteral("連接中..."));
    tcpClient.connectToHost(QHostAddress(ipAddress), port);
}
void TcpFileServer::startTransfer()
{
    localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly))
    {
        QMessageBox::warning(this,QStringLiteral("應用程式"),
                             QStringLiteral("無法讀取 %1:\n%2.").arg(fileName)
                                 .arg(localFile->errorString()));
        return;
    }
    totalBytes1 = localFile->size();
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_6);
    QString currentFile = fileName.right(fileName.size() -
                                         fileName.lastIndexOf("/")-1);
    sendOut <<qint64(0)<<qint64(0)<<currentFile;
    totalBytes1 += outBlock.size();

    sendOut.device()->seek(0);
    sendOut<<totalBytes1<<qint64((outBlock.size()-sizeof(qint64)*2));
    bytesToWrite = totalBytes1 - tcpClient.write(outBlock);
    clientStatusLabel->setText(QStringLiteral("已連接"));
    qDebug() << currentFile <<totalBytes1;
    outBlock.resize(0);
}
void TcpFileServer::updateClientProgress(qint64 numBytes)
{
    bytesWritten += (int) numBytes;
    if(bytesToWrite > 0)
    {
        outBlock = localFile->read(qMin(bytesToWrite, loadSize));
        bytesToWrite -= (int) tcpClient.write(outBlock);
        outBlock.resize(0);
    }else
    {
        localFile->close();
    }

    clientProgressBar->setMaximum(totalBytes1);
    clientProgressBar->setValue(bytesWritten);
    clientStatusLabel->setText(QStringLiteral("已傳送 %1 Bytes").arg(bytesWritten));
}
//伺服器
void TcpFileServer::startServer()
{
    startButton1->setEnabled(false);
    byteReceived = 0;
    fileNameSize = 0;
    qDebug() << "Trying to start the server...";
    while (!tcpServer.isListening() && !tcpServer.listen(QHostAddress::AnyIPv4, 16998))
    {
        QMessageBox::StandardButton ret = QMessageBox::critical(this,
                                                                QStringLiteral("Error"),
                                                                QStringLiteral("無法啟動伺服器: %1.").arg(tcpServer.errorString()),
                                                                QMessageBox::Retry | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel)
        {
            startButton1->setEnabled(true);
            return;
        }
    }
    qDebug() << "Server is listening on port 16998...";
    serverStatusLabel1->setText(QStringLiteral("監聽中..."));
}
void TcpFileServer::acceptConnection()
{
    tcpServerConnection = tcpServer.nextPendingConnection();
    connect(tcpServerConnection, SIGNAL(readyRead()),
            this, SLOT(updateServerProgress())); // 正確的槽函式名
    connect(tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError1(QAbstractSocket::SocketError)));

    serverStatusLabel1->setText(QStringLiteral("接受連線"));
    tcpServer.close();  // 暫停接受其他連接
}
void TcpFileServer::updateServerProgress()
{
    qDebug() << "Data available: " << tcpServerConnection->bytesAvailable();  // 顯示可用的字節數
    QDataStream in(tcpServerConnection);
    in.setVersion(QDataStream::Qt_4_6);
    if (byteReceived <= sizeof(qint64) * 2)
    {
        if ((fileNameSize == 0) && (tcpServerConnection->bytesAvailable() >= sizeof(qint64) * 2))
        {
            in >> totalBytes1 >> fileNameSize;
            byteReceived += sizeof(qint64) * 2;
            qDebug() << "File size: " << totalBytes1 << ", File name size: " << fileNameSize;
        }
        if ((fileNameSize != 0) && (tcpServerConnection->bytesAvailable() >= fileNameSize))
        {
            in >> fileName;
            byteReceived += fileNameSize;
            qDebug() << "File name: " << fileName;

            localFile = new QFile(fileName);
            if (!localFile->open(QFile::WriteOnly))
            {
                QMessageBox::warning(this, QStringLiteral("應用程式"),
                                     QStringLiteral("無法讀取檔案 %1：\n%2.").arg(fileName)
                                         .arg(localFile->errorString()));
                return;
            }
        }
        else
        {
            return;
        }
    }
    if (byteReceived < totalBytes1)
    {
        byteReceived += tcpServerConnection->bytesAvailable();
        inBlock = tcpServerConnection->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }
    serverProgressBar1->setMaximum(totalBytes1);
    serverProgressBar1->setValue(byteReceived);
    qDebug() << "Bytes received: " << byteReceived;
    serverStatusLabel1->setText(QStringLiteral("已接收 %1 Bytes").arg(byteReceived));
    if (byteReceived == totalBytes1)
    {
        tcpServerConnection->close();
        startButton1->setEnabled(true);
        localFile->close();
        startServer();  // 重啟伺服器，準備接收下一個檔案
    }
}
void TcpFileServer::displayError1(QAbstractSocket::SocketError socketError)
{
    QObject *server = qobject_cast<QObject *>(sender());
    if (server == tcpServerConnection)
        qDebug() << "TCP Socket error";
    if (server == &tcpServer)
        qDebug() << "TCP Server error";

    QMessageBox::information(this, QStringLiteral("網絡錯誤"),
                             QStringLiteral("產生如下錯誤: %1.")
                                 .arg(tcpServerConnection->errorString()));
    tcpServerConnection->close();
    serverProgressBar1->reset();
    serverStatusLabel1->setText(QStringLiteral("伺服器就緒"));
    startButton1->setEnabled(true);
}
TcpFileServer::~TcpFileServer() {
}
