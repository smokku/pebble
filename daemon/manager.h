#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "dbusconnector.h"
#include "voicecallmanager.h"
#include "notificationmanager.h"

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QtContacts/QContactManager>
#include <QtContacts/QContactDetailFilter>
#include <MNotification>
#include "Logger"

using namespace QtContacts;

class Manager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    friend class PebbledProxy;

    QBluetoothLocalDevice btDevice;

    watch::WatchConnector *watch;
    DBusConnector *dbus;
    VoiceCallManager *voice;
    NotificationManager *notifications;

    MNotification notification;

    QContactManager *contacts;
    QContactDetailFilter numberFilter;

public:
    explicit Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice, NotificationManager *notifications);

    Q_INVOKABLE QString findPersonByNumber(QString number);

signals:

public slots:
    void hangupAll();

protected slots:
    void onPebbleChanged();
    void onConnectedChanged();
    void onActiveVoiceCallChanged();
    void onVoiceError(const QString &message);
    void onActiveVoiceCallStatusChanged();
    void onNotifyError(const QString &message);
    void onSmsNotify(const QString &sender, const QString &data);
    void onEmailNotify(const QString &sender, const QString &data,const QString &subject);
};

class PebbledProxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap pebble READ pebble)
    Q_PROPERTY(QString name READ pebbleName)
    Q_PROPERTY(QString address READ pebbleAddress)
    Q_PROPERTY(bool connected READ pebbleConnected)

    QVariantMap pebble() { return static_cast<Manager*>(parent())->dbus->pebble(); }
    QString pebbleName() { return static_cast<Manager*>(parent())->dbus->pebble()["Name"].toString(); }
    QString pebbleAddress() { return static_cast<Manager*>(parent())->dbus->pebble()["Address"].toString(); }
    bool pebbleConnected() { return static_cast<Manager*>(parent())->watch->isConnected(); }

public:
    explicit PebbledProxy(QObject *parent) : QObject(parent) {}

public slots:
    void ping(int val) { static_cast<Manager*>(parent())->watch->ping((unsigned int)val); }
    void disconnect() { static_cast<Manager*>(parent())->watch->disconnect(); }
    void reconnect() { static_cast<Manager*>(parent())->watch->reconnect(); }

};

#endif // MANAGER_H
