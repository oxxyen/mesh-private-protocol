// src/assets/mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTextCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), client(new ChatClient(this))
{
    ui->setupUi(this);
    ui->tokenDisplay->setReadOnly(true);
    ui->chatDisplay->setReadOnly(true);
    ui->messageEdit->setEnabled(false);
    ui->sendButton->setEnabled(false);

    // Подключение сигналов кнопок
    connect(ui->registerButton, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(ui->loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    // Подключение сигналов клиента
    connect(client, &ChatClient::registered, this, &MainWindow::onRegistered);
    connect(client, &ChatClient::registrationFailed, this, &MainWindow::onRegistrationFailed);
    connect(client, &ChatClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(client, &ChatClient::loginFailed, this, &MainWindow::onLoginFailed);
    connect(client, &ChatClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(client, &ChatClient::onlineUsersUpdated, this, &MainWindow::onOnlineUsersUpdated);
    connect(client, &ChatClient::connected, this, &MainWindow::onConnected);
    connect(client, &ChatClient::disconnected, this, &MainWindow::onDisconnected);
    connect(client, &ChatClient::error, this, &MainWindow::onError);

    // Подключение к серверу
    ui->statusLabel->setText("Connecting to server...");
    client->connectToServer("127.0.0.1", 5555);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onRegisterClicked()
{
    QString nick = ui->nickLineEdit->text().trimmed();
    if (nick.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter a nickname.");
        return;
    }
    client->registerUser(nick);
    ui->statusLabel->setText("Registering...");
}

void MainWindow::onLoginClicked()
{
    QString nick = ui->nickLineEdit->text().trimmed();
    QString token = ui->tokenLineEdit->text().trimmed();
    if (nick.isEmpty() || token.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please enter nickname and token.");
        return;
    }
    client->loginUser(nick, token);
    ui->statusLabel->setText("Logging in...");
}

void MainWindow::onSendClicked()
{
    QString message = ui->messageEdit->text().trimmed();
    if (message.isEmpty()) return;

    if (message.startsWith("/msg ")) {
        client->sendCommand(message);
    } else {
        client->sendCommand(message);
    }
    ui->messageEdit->clear();
}

void MainWindow::onRegistered(const QString &nick, const QString &token)
{
    ui->statusLabel->setText("✅ Registration successful!");
    ui->tokenDisplay->setText(QString("Nickname: %1\nToken: %2").arg(nick, token));
    QMessageBox::information(this, "Success", "Registration completed! Save your token.");
}

void MainWindow::onRegistrationFailed(const QString &reason)
{
    ui->statusLabel->setText("❌ Registration failed");
    QMessageBox::warning(this, "Error", reason);
}

void MainWindow::onLoginSuccess(const QString &nick)
{
    ui->statusLabel->setText("✅ Logged in as " + nick);
    ui->nickLineEdit->setEnabled(false);
    ui->tokenLineEdit->setEnabled(false);
    ui->registerButton->setEnabled(false);
    ui->loginButton->setEnabled(false);
    ui->messageEdit->setEnabled(true);
    ui->sendButton->setEnabled(true);
    ui->chatDisplay->append("<b>Logged in as " + nick + "</b>");
}

void MainWindow::onLoginFailed(const QString &reason)
{
    ui->statusLabel->setText("❌ Login failed");
    QMessageBox::warning(this, "Error", reason);
}

void MainWindow::onMessageReceived(const QString &from, const QString &text, const QString &type)
{
    QString prefix = "[" + from + "] ";
    if (type == "private") {
        prefix = "[PRIVATE from " + from + "] ";
    }
    ui->chatDisplay->append(prefix + text);
    QTextCursor cursor = ui->chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chatDisplay->setTextCursor(cursor);
}

void MainWindow::onOnlineUsersUpdated(const QStringList &users)
{
    qDebug() << "Online users:" << users;
}

void MainWindow::onConnected()
{
    ui->statusLabel->setText("✅ Connected to server");
}

void MainWindow::onDisconnected()
{
    ui->statusLabel->setText("⚠️ Disconnected from server");
    ui->nickLineEdit->setEnabled(true);
    ui->tokenLineEdit->setEnabled(true);
    ui->registerButton->setEnabled(true);
    ui->loginButton->setEnabled(true);
    ui->messageEdit->setEnabled(false);
    ui->sendButton->setEnabled(false);
}

void MainWindow::onError(const QString &msg)
{
    ui->statusLabel->setText("❌ Error: " + msg);
    QMessageBox::critical(this, "Connection Error", msg);
}