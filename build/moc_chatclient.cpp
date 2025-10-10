/****************************************************************************
** Meta object code from reading C++ file 'chatclient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.17)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../src/assets/chatclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chatclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.17. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChatClient_t {
    QByteArrayData data[33];
    char stringdata0[399];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ChatClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ChatClient_t qt_meta_stringdata_ChatClient = {
    {
QT_MOC_LITERAL(0, 0, 10), // "ChatClient"
QT_MOC_LITERAL(1, 11, 9), // "connected"
QT_MOC_LITERAL(2, 21, 0), // ""
QT_MOC_LITERAL(3, 22, 12), // "disconnected"
QT_MOC_LITERAL(4, 35, 12), // "loginSuccess"
QT_MOC_LITERAL(5, 48, 4), // "nick"
QT_MOC_LITERAL(6, 53, 11), // "loginFailed"
QT_MOC_LITERAL(7, 65, 5), // "error"
QT_MOC_LITERAL(8, 71, 19), // "registrationSuccess"
QT_MOC_LITERAL(9, 91, 5), // "token"
QT_MOC_LITERAL(10, 97, 18), // "registrationFailed"
QT_MOC_LITERAL(11, 116, 15), // "messageReceived"
QT_MOC_LITERAL(12, 132, 4), // "from"
QT_MOC_LITERAL(13, 137, 7), // "message"
QT_MOC_LITERAL(14, 145, 22), // "privateMessageReceived"
QT_MOC_LITERAL(15, 168, 2), // "to"
QT_MOC_LITERAL(16, 171, 24), // "encryptedMessageReceived"
QT_MOC_LITERAL(17, 196, 4), // "data"
QT_MOC_LITERAL(18, 201, 19), // "onlineUsersReceived"
QT_MOC_LITERAL(19, 221, 5), // "users"
QT_MOC_LITERAL(20, 227, 12), // "helpReceived"
QT_MOC_LITERAL(21, 240, 8), // "commands"
QT_MOC_LITERAL(22, 249, 22), // "p2pConnectionInitiated"
QT_MOC_LITERAL(23, 272, 6), // "target"
QT_MOC_LITERAL(24, 279, 7), // "success"
QT_MOC_LITERAL(25, 287, 13), // "errorOccurred"
QT_MOC_LITERAL(26, 301, 11), // "onConnected"
QT_MOC_LITERAL(27, 313, 14), // "onDisconnected"
QT_MOC_LITERAL(28, 328, 7), // "onError"
QT_MOC_LITERAL(29, 336, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(30, 365, 11), // "socketError"
QT_MOC_LITERAL(31, 377, 11), // "onReadyRead"
QT_MOC_LITERAL(32, 389, 9) // "onTimeout"

    },
    "ChatClient\0connected\0\0disconnected\0"
    "loginSuccess\0nick\0loginFailed\0error\0"
    "registrationSuccess\0token\0registrationFailed\0"
    "messageReceived\0from\0message\0"
    "privateMessageReceived\0to\0"
    "encryptedMessageReceived\0data\0"
    "onlineUsersReceived\0users\0helpReceived\0"
    "commands\0p2pConnectionInitiated\0target\0"
    "success\0errorOccurred\0onConnected\0"
    "onDisconnected\0onError\0"
    "QAbstractSocket::SocketError\0socketError\0"
    "onReadyRead\0onTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChatClient[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      13,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  104,    2, 0x06 /* Public */,
       3,    0,  105,    2, 0x06 /* Public */,
       4,    1,  106,    2, 0x06 /* Public */,
       6,    1,  109,    2, 0x06 /* Public */,
       8,    2,  112,    2, 0x06 /* Public */,
      10,    1,  117,    2, 0x06 /* Public */,
      11,    2,  120,    2, 0x06 /* Public */,
      14,    3,  125,    2, 0x06 /* Public */,
      16,    3,  132,    2, 0x06 /* Public */,
      18,    1,  139,    2, 0x06 /* Public */,
      20,    1,  142,    2, 0x06 /* Public */,
      22,    2,  145,    2, 0x06 /* Public */,
      25,    1,  150,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      26,    0,  153,    2, 0x08 /* Private */,
      27,    0,  154,    2, 0x08 /* Private */,
      28,    1,  155,    2, 0x08 /* Private */,
      31,    0,  158,    2, 0x08 /* Private */,
      32,    0,  159,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    5,    9,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   12,   13,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   12,   15,   13,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   12,   15,   17,
    QMetaType::Void, QMetaType::QStringList,   19,
    QMetaType::Void, QMetaType::QStringList,   21,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   23,   24,
    QMetaType::Void, QMetaType::QString,    7,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 29,   30,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ChatClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChatClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->loginSuccess((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->loginFailed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->registrationSuccess((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 5: _t->registrationFailed((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->messageReceived((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 7: _t->privateMessageReceived((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 8: _t->encryptedMessageReceived((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 9: _t->onlineUsersReceived((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 10: _t->helpReceived((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 11: _t->p2pConnectionInitiated((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 12: _t->errorOccurred((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 13: _t->onConnected(); break;
        case 14: _t->onDisconnected(); break;
        case 15: _t->onError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 16: _t->onReadyRead(); break;
        case 17: _t->onTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 15:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChatClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::connected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::disconnected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::loginSuccess)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::loginFailed)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::registrationSuccess)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::registrationFailed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::messageReceived)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & , const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::privateMessageReceived)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & , const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::encryptedMessageReceived)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QStringList & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::onlineUsersReceived)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QStringList & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::helpReceived)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & , bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::p2pConnectionInitiated)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (ChatClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::errorOccurred)) {
                *result = 12;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ChatClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ChatClient.data,
    qt_meta_data_ChatClient,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ChatClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChatClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChatClient.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChatClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    }
    return _id;
}

// SIGNAL 0
void ChatClient::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChatClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChatClient::loginSuccess(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ChatClient::loginFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChatClient::registrationSuccess(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ChatClient::registrationFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void ChatClient::messageReceived(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ChatClient::privateMessageReceived(const QString & _t1, const QString & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void ChatClient::encryptedMessageReceived(const QString & _t1, const QString & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void ChatClient::onlineUsersReceived(const QStringList & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void ChatClient::helpReceived(const QStringList & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void ChatClient::p2pConnectionInitiated(const QString & _t1, bool _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void ChatClient::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
