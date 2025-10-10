#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_client(new ChatClient(this))
{
    ui->setupUi(this);

    connect(ui->connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui->disconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(ui->registerButton, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(ui->loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(ui->privateSendButton, &QPushButton::clicked, this, &MainWindow::onPrivateSendClicked);
    connect(ui->encryptedSendButton, &QPushButton::clicked, this, &MainWindow::onEncryptedSendClicked);
    connect(ui->onlineButton, &QPushButton::clicked, this, &MainWindow::onOnlineClicked);
    connect(ui->helpButton, &QPushButton::clicked, this, &MainWindow::onHelpClicked);
    connect(ui->p2pConnectButton, &QPushButton::clicked, this, &MainWindow::onP2PConnectClicked);

    connect(m_client, &ChatClient::connected, this, [](){
        QMessageBox::information(nullptr, "Connected", "Connected to server successfully!");
    });
    connect(m_client, &ChatClient::disconnected, this, [](){
        QMessageBox::information(nullptr, "Disconnected", "Disconnected from server.");
    });
    connect(m_client, &ChatClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_client, &ChatClient::loginFailed, this, &MainWindow::onLoginFailed);
    connect(m_client, &ChatClient::registrationSuccess, this, &MainWindow::onRegistrationSuccess);
    connect(m_client, &ChatClient::registrationFailed, this, &MainWindow::onRegistrationFailed);
    connect(m_client, &ChatClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_client, &ChatClient::privateMessageReceived, this, &MainWindow::onPrivateMessageReceived);
    connect(m_client, &ChatClient::encryptedMessageReceived, this, &MainWindow::onEncryptedMessageReceived);
    connect(m_client, &ChatClient::onlineUsersReceived, this, &MainWindow::onOnlineUsersReceived);
    connect(m_client, &ChatClient::helpReceived, this, &MainWindow::onHelpReceived);
    connect(m_client, &ChatClient::p2pConnectionInitiated, this, &MainWindow::onP2PConnectionInitiated);
    connect(m_client, &ChatClient::errorOccurred, this, &MainWindow::onErrorOccurred);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onConnectClicked()
{
    QString host = ui->hostEdit->text();
    int port = ui->portEdit->text().toInt();
    if (host.isEmpty() || port == 0) {
        QMessageBox::warning(this, "Error", "Please enter valid host and port.");
        return;
    }
    if (m_client->connectToServer(host, port)) {
        ui->statusLabel->setText("Connecting...");
    }
}

void MainWindow::onDisconnectClicked()
{
    m_client->disconnectFromServer();
    ui->statusLabel->setText("Disconnected");
    ui->nickEdit->clear();
    ui->tokenEdit->clear();
    ui->messageEdit->clear();
    ui->chatArea->clear();
    ui->onlineUsersList->clear();
    m_currentUser.clear();
}

void MainWindow::onRegisterClicked()
{
    QString nick = ui->nickEdit->text();
    if (nick.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a nickname.");
        return;
    }
    m_client->registerUser(nick);
}

void MainWindow::onLoginClicked()
{
    QString nick = ui->nickEdit->text();
    QString token = ui->tokenEdit->text();
    if (nick.isEmpty() || token.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter nickname and token.");
        return;
    }
    m_client->loginUser(nick, token);
}

void MainWindow::onSendClicked()
{
    QString message = ui->messageEdit->text();
    if (message.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter a message.");
        return;
    }
    m_client->sendMessage(message);
    ui->messageEdit->clear();
}

void MainWindow::onPrivateSendClicked()
{
    QString to = ui->toEdit->text();
    QString message = ui->messageEdit->text();
    if (to.isEmpty() || message.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter recipient and message.");
        return;
    }
    m_client->sendPrivateMessage(to, message);
    ui->messageEdit->clear();
}

void MainWindow::onEncryptedSendClicked()
{
    QString to = ui->toEdit->text();
    QString message = ui->messageEdit->text();
    if (to.isEmpty() || message.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter recipient and message.");
        return;
    }
    m_client->sendEncryptedMessage(to, message);
    ui->messageEdit->clear();
}

void MainWindow::onOnlineClicked()
{
    m_client->requestOnlineUsers();
}

void MainWindow::onHelpClicked()
{
    m_client->requestHelp();
}

void MainWindow::onP2PConnectClicked()
{
    QString target = ui->toEdit->text();
    if (target.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please enter target user.");
        return;
    }
    m_client->p2pConnect(target);
}

void MainWindow::onMessageReceived(const QString &from, const QString &message)
{
    QString formatted = QString("[%1] %2: %3").arg(QDateTime::currentDateTime().toString("HH:mm:ss")).arg(from).arg(message);
    ui->chatArea->append(formatted);
}

void MainWindow::onPrivateMessageReceived(const QString &from, const QString &to, const QString &message)
{
    QString formatted = QString("[PRIVATE from %1 to %2] %3").arg(from).arg(to).arg(message);
    ui->chatArea->append(formatted);
}

void MainWindow::onEncryptedMessageReceived(const QString &from, const QString &to, const QString &data)
{
    QString formatted = QString("[ENCRYPTED from %1 to %2] %3").arg(from).arg(to).arg(data);
    ui->chatArea->append(formatted);
}

void MainWindow::onOnlineUsersReceived(const QStringList &users)
{
    ui->onlineUsersList->clear();
    ui->onlineUsersList->addItems(users);
    QMessageBox::information(this, "Online Users", QString("Online users: %1").arg(users.join(", ")));
}

void MainWindow::onHelpReceived(const QStringList &commands)
{
    QString helpText = "Available commands:\n" + commands.join("\n");
    QMessageBox::information(this, "Help", helpText);
}

void MainWindow::onP2PConnectionInitiated(const QString &target, bool success)
{
    if (success) {
        QMessageBox::information(this, "P2P Connection", QString("P2P connection initiated with %1 successfully.").arg(target));
    } else {
        QMessageBox::warning(this, "P2P Connection", QString("Failed to initiate P2P connection with %1.").arg(target));
    }
}

void MainWindow::onLoginSuccess(const QString &nick)
{
    m_currentUser = nick;
    ui->statusLabel->setText(QString("Logged in as %1").arg(nick));
    QMessageBox::information(this, "Login Success", QString("Welcome, %1!").arg(nick));
}

void MainWindow::onLoginFailed(const QString &error)
{
    QMessageBox::critical(this, "Login Failed", error);
}

void MainWindow::onRegistrationSuccess(const QString &nick, const QString &token)
{
    ui->tokenEdit->setText(token);
    QMessageBox::information(this, "Registration Success", QString("Registered as %1. Your token is: %2").arg(nick).arg(token));
}

void MainWindow::onRegistrationFailed(const QString &error)
{
    QMessageBox::critical(this, "Registration Failed", error);
}

void MainWindow::onErrorOccurred(const QString &error)
{
    QMessageBox::critical(this, "Error", error);
}