#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QMessageBox>
#include <QFileDialog>
#include "chatclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onRegisterClicked();
    void onLoginClicked();
    void onSendClicked();
    void onPrivateSendClicked();
    void onEncryptedSendClicked();
    void onOnlineClicked();
    void onHelpClicked();
    void onP2PConnectClicked();
    void onMessageReceived(const QString &from, const QString &message);
    void onPrivateMessageReceived(const QString &from, const QString &to, const QString &message);
    void onEncryptedMessageReceived(const QString &from, const QString &to, const QString &data);
    void onOnlineUsersReceived(const QStringList &users);
    void onHelpReceived(const QStringList &commands);
    void onP2PConnectionInitiated(const QString &target, bool success);
    void onLoginSuccess(const QString &nick);
    void onLoginFailed(const QString &error);
    void onRegistrationSuccess(const QString &nick, const QString &token);
    void onRegistrationFailed(const QString &error);
    void onErrorOccurred(const QString &error);

private:
    Ui::MainWindow *ui;
    ChatClient *m_client;
    QString m_currentUser;
};

#endif // MAINWINDOW_H