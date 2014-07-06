#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "dbusconnector.h"
#include "voicecallmanager.h"

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QtContacts/QContactManager>
#include <QtContacts/QContactDetailFilter>
#include <CommHistory/GroupModel>
#include <MNotification>

using namespace QtContacts;
using namespace CommHistory;

class Manager : public QObject
{
    Q_OBJECT

    friend class PebbledProxy;

    QBluetoothLocalDevice btDevice;

    watch::WatchConnector *watch;
    DBusConnector *dbus;
    VoiceCallManager *voice;

    MNotification notification;

    QContactManager *contacts;
    QContactDetailFilter numberFilter;
    GroupManager *conversations;

public:
    explicit Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice);

    Q_INVOKABLE QString findPersonByNumber(QString number);
    Q_INVOKABLE void processUnreadMessages(GroupObject *group);

signals:

public slots:
    void hangupAll();

protected slots:
    void onPebbleChanged();
    void onConnectedChanged();
    void onActiveVoiceCallChanged();
    void onVoiceError(const QString &message);
    void onActiveVoiceCallStatusChanged();
    void onConversationGroupAdded(GroupObject *group);
    void onUnreadMessagesChanged();

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

};

#endif // MANAGER_H
