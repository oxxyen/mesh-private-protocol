// src/assets/chatclient.cpp
#include "chatclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

ChatClient::ChatClient(QObject *parent)
    : QObject(parent), socket(new QSslSocket(this))
{
    connect(socket, &QSslSocket::readyRead, this, &ChatClient::onReadyRead);

    // Исправление перегрузки sslErrors
    connect(socket,
            static_cast<void (QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
            this,
            &ChatClient::onSslErrors);

    connect(socket, &QSslSocket::connected, this, &ChatClient::connected);
    connect(socket, &QSslSocket::disconnected, this, &ChatClient::disconnected);
    connect(socket, &QSslSocket::errorOccurred, this, [=](QAbstractSocket::SocketError) {
        emit error(socket->errorString());
    });
}

void ChatClient::connectToServer(const QString &host, quint16 port)
{
    socket->connectToHostEncrypted(host, port);
    socket->ignoreSslErrors(); // Только для dev!
}

void ChatClient::disconnectFromServer()
{
    socket->disconnectFromHost();
}

void ChatClient::registerUser(const QString &nick)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit error("Not connected to server");
        return;
    }
    QString cmd = "/register " + nick + "\n";
    socket->write(cmd.toUtf8());
}

void ChatClient::loginUser(const QString &nick, const QString &token)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit error("Not connected to server");
        return;
    }
    QString cmd = "/login " + nick + " " + token + "\n";
    socket->write(cmd.toUtf8());
}

void ChatClient::sendCommand(const QString &command)
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write((command + "\n").toUtf8());
    }
}

void ChatClient::onReadyRead()
{
    buffer += socket->readAll();
    while (buffer.contains('\n')) {
        int idx = buffer.indexOf('\n');
        QString line = buffer.left(idx).trimmed();
        buffer = buffer.mid(idx + 1);

        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
        if (!doc.isObject()) continue;

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();

        if (type == "register") {
            QString status = obj["status"].toString();
            if (status == "success") {
                QString nick = obj["nick"].toString();
                QString token = obj["token"].toString();
                emit registered(nick, token);
            } else {
                QString msg = obj["message"].toString("Registration failed");
                emit registrationFailed(msg);
            }
        } else if (type == "login") {
            QString status = obj["status"].toString();
            if (status == "success") {
                QString nick = obj["nick"].toString();
                emit loginSuccess(nick);
            } else {
                QString msg = obj["message"].toString("Login failed");
                emit loginFailed(msg);
            }
        } else if (type == "message" || type == "private") {
            QString from = obj["from"].toString();
            QString text = obj["text"].toString();
            emit messageReceived(from, text, type);
        } else if (type == "online") {
            QJsonArray users = obj["users"].toArray();
            QStringList userList;
            for (const QJsonValue &val : users) {
                userList << val.toString();
            }
            emit onlineUsersUpdated(userList);
        }
    }
}

void ChatClient::onSslErrors(const QList<QSslError> &errors)
{
    for (const QSslError &e : errors) {
        qDebug() << "SSL Error:" << e.errorString();
    }
    socket->ignoreSslErrors(errors);
}