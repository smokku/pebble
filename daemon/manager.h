#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "dbusconnector.h"
#include "voicecallmanager.h"
#include "watchcommands.h"

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QDBusContext>
#include <QtContacts/QContactManager>
#include <QtContacts/QContactDetailFilter>
#include <CommHistory/GroupModel>
#include <MNotification>
#include "Logger"

using namespace QtContacts;
using namespace CommHistory;

class Manager :
        public QObject,
        protected QDBusContext
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    friend class PebbledProxy;

    Q_PROPERTY(QString mpris READ mpris)
    Q_PROPERTY(QVariantMap mprisMetadata READ getMprisMetadata WRITE setMprisMetadata NOTIFY mprisMetadataChanged)

    QBluetoothLocalDevice btDevice;

    watch::WatchConnector *watch;
    DBusConnector *dbus;
    VoiceCallManager *voice;

    WatchCommands *commands;

    MNotification notification;

    QContactManager *contacts;
    QContactDetailFilter numberFilter;
    GroupManager *conversations;

    QString lastSeenMpris;

public:
    explicit Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice);

    Q_INVOKABLE QString findPersonByNumber(QString number);
    Q_INVOKABLE void processUnreadMessages(GroupObject *group);
    Q_INVOKABLE QString mpris();
    QVariantMap mprisMetadata;
    QVariantMap getMprisMetadata() { return mprisMetadata; }

signals:
    void mprisMetadataChanged(QVariantMap);

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
    void onMprisPropertiesChanged(QString,QMap<QString,QVariant>,QStringList);
    void setMprisMetadata(QDBusArgument metadata);
    void setMprisMetadata(QVariantMap metadata);

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
