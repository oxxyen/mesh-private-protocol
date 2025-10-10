#include "chatclient.h"
#include <QDebug>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>

ChatClient::ChatClient(QObject *parent) : QObject(parent), m_socket(new QSslSocket(this)), m_timeoutTimer(new QTimer(this)), m_loggedIn(false) {
    connect(m_socket, &QSslSocket::connected, this, &ChatClient::onConnected);
    connect(m_socket, &QSslSocket::disconnected, this, &ChatClient::onDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::error), this, &ChatClient::onError);
    connect(m_socket, &QSslSocket::readyRead, this, &ChatClient::onReadyRead);
    connect(m_timeoutTimer, &QTimer::timeout, this, &ChatClient::onTimeout);
}

ChatClient::~ChatClient() {
    disconnectFromServer();
}

bool ChatClient::connectToServer(const QString &host, int port) {
    m_host = host;
    m_port = port;
    m_socket->connectToHostEncrypted(host, port);
    m_timeoutTimer->start(10000); // 10 seconds timeout
    return true;
}

void ChatClient::disconnectFromServer() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    m_loggedIn = false;
    m_nick.clear();
    m_token.clear();
}

void ChatClient::registerUser(const QString &nick) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QString command = QString("/register %1").arg(nick);
    m_socket->write(command.toUtf8() + "\n");
}

void ChatClient::loginUser(const QString &nick, const QString &token) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QString command = QString("/login %1 %2").arg(nick).arg(token);
    m_socket->write(command.toUtf8() + "\n");
}

void ChatClient::sendMessage(const QString &message) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    m_socket->write(message.toUtf8() + "\n");
}

void ChatClient::sendPrivateMessage(const QString &to, const QString &message) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QString command = QString("/msg %1 %2").arg(to).arg(message);
    m_socket->write(command.toUtf8() + "\n");
}

void ChatClient::sendEncryptedMessage(const QString &to, const QString &message) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QString command = QString("/send_enc %1 %2").arg(to).arg(message);
    m_socket->write(command.toUtf8() + "\n");
}

void ChatClient::requestOnlineUsers() {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    m_socket->write("/online\n");
}

void ChatClient::requestHelp() {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    m_socket->write("/help\n");
}

void ChatClient::p2pConnect(const QString &target) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server");
        return;
    }
    QString command = QString("/p2p_connect %1").arg(target);
    m_socket->write(command.toUtf8() + "\n");
}

void ChatClient::onConnected() {
    m_timeoutTimer->stop();
    emit connected();
}

void ChatClient::onDisconnected() {
    m_loggedIn = false;
    m_nick.clear();
    m_token.clear();
    emit disconnected();
}

void ChatClient::onError(QAbstractSocket::SocketError socketError) {
    m_timeoutTimer->stop();
    emit errorOccurred(m_socket->errorString());
    m_socket->disconnectFromHost();
}

void ChatClient::onReadyRead() {
    while (m_socket->canReadLine()) {
        QString line = QString::fromUtf8(m_socket->readLine()).trimmed();
        if (line.startsWith("{")) {
            QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
            if (doc.isNull()) continue;
            QJsonObject obj = doc.object();
            QString type = obj.value("type").toString();
            if (type == "register") {
                if (obj.value("status").toString() == "success") {
                    QString nick = obj.value("nick").toString();
                    QString token = obj.value("token").toString();
                    emit registrationSuccess(nick, token);
                } else {
                    QString error = obj.value("message").toString();
                    emit registrationFailed(error);
                }
            } else if (type == "login") {
                if (obj.value("status").toString() == "success") {
                    QString nick = obj.value("nick").toString();
                    m_nick = nick;
                    m_loggedIn = true;
                    emit loginSuccess(nick);
                } else {
                    QString error = obj.value("message").toString();
                    emit loginFailed(error);
                }
            } else if (type == "message") {
                QString from = obj.value("from").toString();
                QString text = obj.value("text").toString();
                emit messageReceived(from, text);
            } else if (type == "private") {
                QString from = obj.value("from").toString();
                QString to = obj.value("to").toString();
                QString text = obj.value("text").toString();
                emit privateMessageReceived(from, to, text);
            } else if (type == "encrypted") {
                QString from = obj.value("from").toString();
                QString to = obj.value("to").toString();
                QString data = obj.value("data").toString();
                emit encryptedMessageReceived(from, to, data);
            } else if (type == "online") {
                QJsonArray users = obj.value("users").toArray();
                QStringList userList;
                for (const QJsonValue &val : users) {
                    userList.append(val.toString());
                }
                emit onlineUsersReceived(userList);
            } else if (type == "help") {
                QJsonArray commands = obj.value("commands").toArray();
                QStringList cmdList;
                for (const QJsonValue &val : commands) {
                    cmdList.append(val.toString());
                }
                emit helpReceived(cmdList);
            } else if (type == "p2p_initiated") {
                QString target = obj.value("target").toString();
                bool success = obj.value("status").toString() == "success";
                emit p2pConnectionInitiated(target, success);
            }
        } else {
            // Plain text message
            emit messageReceived("System", line);
        }
    }
}

void ChatClient::onTimeout() {
    if (m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Connection timeout");
        m_socket->abort();
    }
}