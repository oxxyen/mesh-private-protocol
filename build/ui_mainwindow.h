/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.17
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionExit;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QLabel *statusLabel;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *hostEdit;
    QLabel *label_2;
    QLineEdit *portEdit;
    QPushButton *connectButton;
    QPushButton *disconnectButton;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_3;
    QLineEdit *nickEdit;
    QLabel *label_4;
    QLineEdit *tokenEdit;
    QPushButton *registerButton;
    QPushButton *loginButton;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_5;
    QLineEdit *toEdit;
    QLabel *label_6;
    QLineEdit *messageEdit;
    QPushButton *sendButton;
    QPushButton *privateSendButton;
    QPushButton *encryptedSendButton;
    QPushButton *onlineButton;
    QPushButton *helpButton;
    QPushButton *p2pConnectButton;
    QTabWidget *tabWidget;
    QWidget *chatTab;
    QVBoxLayout *verticalLayout_2;
    QTextEdit *chatArea;
    QWidget *onlineTab;
    QVBoxLayout *verticalLayout_3;
    QListWidget *onlineUsersList;
    QMenuBar *menubar;
    QMenu *menuFile;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(800, 600);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        statusLabel = new QLabel(centralwidget);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));

        verticalLayout->addWidget(statusLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        hostEdit = new QLineEdit(centralwidget);
        hostEdit->setObjectName(QString::fromUtf8("hostEdit"));

        horizontalLayout->addWidget(hostEdit);

        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout->addWidget(label_2);

        portEdit = new QLineEdit(centralwidget);
        portEdit->setObjectName(QString::fromUtf8("portEdit"));

        horizontalLayout->addWidget(portEdit);

        connectButton = new QPushButton(centralwidget);
        connectButton->setObjectName(QString::fromUtf8("connectButton"));

        horizontalLayout->addWidget(connectButton);

        disconnectButton = new QPushButton(centralwidget);
        disconnectButton->setObjectName(QString::fromUtf8("disconnectButton"));

        horizontalLayout->addWidget(disconnectButton);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_2->addWidget(label_3);

        nickEdit = new QLineEdit(centralwidget);
        nickEdit->setObjectName(QString::fromUtf8("nickEdit"));

        horizontalLayout_2->addWidget(nickEdit);

        label_4 = new QLabel(centralwidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        horizontalLayout_2->addWidget(label_4);

        tokenEdit = new QLineEdit(centralwidget);
        tokenEdit->setObjectName(QString::fromUtf8("tokenEdit"));

        horizontalLayout_2->addWidget(tokenEdit);

        registerButton = new QPushButton(centralwidget);
        registerButton->setObjectName(QString::fromUtf8("registerButton"));

        horizontalLayout_2->addWidget(registerButton);

        loginButton = new QPushButton(centralwidget);
        loginButton->setObjectName(QString::fromUtf8("loginButton"));

        horizontalLayout_2->addWidget(loginButton);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        horizontalLayout_3->addWidget(label_5);

        toEdit = new QLineEdit(centralwidget);
        toEdit->setObjectName(QString::fromUtf8("toEdit"));

        horizontalLayout_3->addWidget(toEdit);

        label_6 = new QLabel(centralwidget);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        horizontalLayout_3->addWidget(label_6);

        messageEdit = new QLineEdit(centralwidget);
        messageEdit->setObjectName(QString::fromUtf8("messageEdit"));

        horizontalLayout_3->addWidget(messageEdit);

        sendButton = new QPushButton(centralwidget);
        sendButton->setObjectName(QString::fromUtf8("sendButton"));

        horizontalLayout_3->addWidget(sendButton);

        privateSendButton = new QPushButton(centralwidget);
        privateSendButton->setObjectName(QString::fromUtf8("privateSendButton"));

        horizontalLayout_3->addWidget(privateSendButton);

        encryptedSendButton = new QPushButton(centralwidget);
        encryptedSendButton->setObjectName(QString::fromUtf8("encryptedSendButton"));

        horizontalLayout_3->addWidget(encryptedSendButton);

        onlineButton = new QPushButton(centralwidget);
        onlineButton->setObjectName(QString::fromUtf8("onlineButton"));

        horizontalLayout_3->addWidget(onlineButton);

        helpButton = new QPushButton(centralwidget);
        helpButton->setObjectName(QString::fromUtf8("helpButton"));

        horizontalLayout_3->addWidget(helpButton);

        p2pConnectButton = new QPushButton(centralwidget);
        p2pConnectButton->setObjectName(QString::fromUtf8("p2pConnectButton"));

        horizontalLayout_3->addWidget(p2pConnectButton);


        verticalLayout->addLayout(horizontalLayout_3);

        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        chatTab = new QWidget();
        chatTab->setObjectName(QString::fromUtf8("chatTab"));
        verticalLayout_2 = new QVBoxLayout(chatTab);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        chatArea = new QTextEdit(chatTab);
        chatArea->setObjectName(QString::fromUtf8("chatArea"));
        chatArea->setReadOnly(true);

        verticalLayout_2->addWidget(chatArea);

        tabWidget->addTab(chatTab, QString());
        onlineTab = new QWidget();
        onlineTab->setObjectName(QString::fromUtf8("onlineTab"));
        verticalLayout_3 = new QVBoxLayout(onlineTab);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        onlineUsersList = new QListWidget(onlineTab);
        onlineUsersList->setObjectName(QString::fromUtf8("onlineUsersList"));

        verticalLayout_3->addWidget(onlineUsersList);

        tabWidget->addTab(onlineTab, QString());

        verticalLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 21));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menuFile->addAction(actionExit);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MeshChat Client", nullptr));
        actionExit->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
        statusLabel->setText(QCoreApplication::translate("MainWindow", "Disconnected", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Host:", nullptr));
        hostEdit->setText(QCoreApplication::translate("MainWindow", "localhost", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Port:", nullptr));
        portEdit->setText(QCoreApplication::translate("MainWindow", "5555", nullptr));
        connectButton->setText(QCoreApplication::translate("MainWindow", "Connect", nullptr));
        disconnectButton->setText(QCoreApplication::translate("MainWindow", "Disconnect", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Nick:", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Token:", nullptr));
        registerButton->setText(QCoreApplication::translate("MainWindow", "Register", nullptr));
        loginButton->setText(QCoreApplication::translate("MainWindow", "Login", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "To:", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "Message:", nullptr));
        sendButton->setText(QCoreApplication::translate("MainWindow", "Send", nullptr));
        privateSendButton->setText(QCoreApplication::translate("MainWindow", "Send Private", nullptr));
        encryptedSendButton->setText(QCoreApplication::translate("MainWindow", "Send Encrypted", nullptr));
        onlineButton->setText(QCoreApplication::translate("MainWindow", "Online Users", nullptr));
        helpButton->setText(QCoreApplication::translate("MainWindow", "Help", nullptr));
        p2pConnectButton->setText(QCoreApplication::translate("MainWindow", "P2P Connect", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(chatTab), QCoreApplication::translate("MainWindow", "Chat", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(onlineTab), QCoreApplication::translate("MainWindow", "Online Users", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
