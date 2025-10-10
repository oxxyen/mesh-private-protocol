#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class ChatClient : public QObject {
    Q_OBJECT

public:
    explicit ChatClient(QObject *parent = nullptr);
    ~ChatClient();

    bool connectToServer(const QString &host, int port);
    void disconnectFromServer();
    bool isLoggedIn() const { return m_loggedIn; }
    QString getNick() const { return m_nick; }

    void registerUser(const QString &nick);
    void loginUser(const QString &nick, const QString &token);
    void sendMessage(const QString &message);
    void sendPrivateMessage(const QString &to, const QString &message);
    void sendEncryptedMessage(const QString &to, const QString &message);
    void requestOnlineUsers();
    void requestHelp();
    void p2pConnect(const QString &target);

signals:
    void connected();
    void disconnected();
    void loginSuccess(const QString &nick);
    void loginFailed(const QString &error);
    void registrationSuccess(const QString &nick, const QString &token);
    void registrationFailed(const QString &error);
    void messageReceived(const QString &from, const QString &message);
    void privateMessageReceived(const QString &from, const QString &to, const QString &message);
    void encryptedMessageReceived(const QString &from, const QString &to, const QString &data);
    void onlineUsersReceived(const QStringList &users);
    void helpReceived(const QStringList &commands);
    void p2pConnectionInitiated(const QString &target, bool success);
    void errorOccurred(const QString &error);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();
    void onTimeout();

private:
    QSslSocket *m_socket;
    QTimer *m_timeoutTimer;
    bool m_loggedIn;
    QString m_nick;
    QString m_token;
    QString m_host;
    int m_port;
};

#endif // CHATCLIENT_H