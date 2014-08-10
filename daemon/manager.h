#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "dbusconnector.h"
#include "voicecallmanager.h"
#include "notificationmanager.h"
#include "watchcommands.h"
#include "settings.h"

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QDBusContext>
#include <QtContacts/QContactManager>
#include <QtContacts/QContactDetailFilter>
#include <MNotification>
#include <Log4Qt/Logger>

#include <unicode/translit.h>

using namespace QtContacts;

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
    NotificationManager *notifications;

    WatchCommands *commands;

    Settings *settings;

    MNotification notification;

    QContactManager *contacts;
    QContactDetailFilter numberFilter;

    QString defaultProfile;

    QString lastSeenMpris;

    QScopedPointer<icu::Transliterator> transliterator;

public:
    explicit Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice, NotificationManager *notifications, Settings *settings);

    Q_INVOKABLE QString findPersonByNumber(QString number);
    Q_INVOKABLE QString getCurrentProfile();
    Q_INVOKABLE QString mpris();
    QVariantMap mprisMetadata;
    QVariantMap getMprisMetadata() { return mprisMetadata; }

protected:
    void transliterateMessage(const QString &text);

signals:
    void mprisMetadataChanged(QVariantMap);

public slots:
    void hangupAll();
    void applyProfile();

protected slots:
    void onSettingChanged(const QString &key);
    void onSettingsChanged();
    void onPebbleChanged();
    void onConnectedChanged();
    void onActiveVoiceCallChanged();
    void onVoiceError(const QString &message);
    void onActiveVoiceCallStatusChanged();
    void onNotifyError(const QString &message);
    void onSmsNotify(const QString &sender, const QString &data);
    void onTwitterNotify(const QString &sender, const QString &data);
    void onFacebookNotify(const QString &sender, const QString &data);
    void onEmailNotify(const QString &sender, const QString &data,const QString &subject);
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
    void time() { static_cast<Manager*>(parent())->watch->time(); }
    void disconnect() { static_cast<Manager*>(parent())->watch->disconnect(); }
    void reconnect() { static_cast<Manager*>(parent())->watch->reconnect(); }

};

#endif // MANAGER_H
