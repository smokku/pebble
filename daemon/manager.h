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
    Q_PROPERTY(QVariantMap pebble READ pebble NOTIFY pebbleChanged)
    Q_PROPERTY(QString name READ pebbleName NOTIFY pebbleChanged)
    Q_PROPERTY(QString address READ pebbleAddress NOTIFY pebbleChanged)
    Q_PROPERTY(bool connected READ pebbleConnected NOTIFY connectedChanged)

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

    QVariantMap pebble() { return dbus->pebble(); }
    QString pebbleName() { return dbus->pebble()["Name"].toString(); }
    QString pebbleAddress() { return dbus->pebble()["Address"].toString(); }
    bool pebbleConnected() { return watch->isConnected(); }

signals:
    void pebbleChanged();
    void connectedChanged();

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

#endif // MANAGER_H
