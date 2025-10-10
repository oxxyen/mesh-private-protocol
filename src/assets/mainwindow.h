#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chatclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRegisterClicked();
    void onRegistered(const QString &nick, const QString &token);
    void onRegistrationFailed(const QString &reason);
    void onConnected();
    void onDisconnected();
    void onError(const QString &msg);

private:
    Ui::MainWindow *ui;
    ChatClient *client;
};

#endif // MAINWINDOW_H