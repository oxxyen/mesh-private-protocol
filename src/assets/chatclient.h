// src/assets/chatclient.h
#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QByteArray>
#include <QStringList>

class ChatClient : public QObject
{
    Q_OBJECT

public:
    explicit ChatClient(QObject *parent = nullptr);
    void connectToServer(const QString &host = "127.0.0.1", quint16 port = 5555);
    void disconnectFromServer();
    void registerUser(const QString &nick);
    void loginUser(const QString &nick, const QString &token);
    void sendCommand(const QString &command);
    void sendPrivateMessage(const QString &to, const QString &text);
    QByteArray encryptMessage(const QString &recipient, const QString &message);

signals:
    void connected();
    void disconnected();
    void error(const QString &message);
    void registered(const QString &nick, const QString &token);
    void registrationFailed(const QString &reason);
    void loginSuccess(const QString &nick);
    void loginFailed(const QString &reason);
    void messageReceived(const QString &from, const QString &text, const QString &type = "public");
    void onlineUsersUpdated(const QStringList &users);

private slots:
    void onReadyRead();
    void onSslErrors(const QList<QSslError> &errors);

private:
    QSslSocket *socket;
    QString buffer;
};

#endif // CHATCLIENT_H