QT       += core gui network widgets
TARGET = MeshChatClient
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    chatclient.cpp

HEADERS += \
    mainwindow.h \
    chatclient.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    assets.qrc

LIBS += -lssl -lcrypto -lpthread -lsqlite3

INCLUDEPATH += .

# Для Windows
win32: LIBS += -lws2_32