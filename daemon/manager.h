#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "dbusconnector.h"
#include "uploadmanager.h"
#include "voicecallmanager.h"
#include "notificationmanager.h"
#include "musicmanager.h"
#include "datalogmanager.h"
#include "appmsgmanager.h"
#include "jskitmanager.h"
#include "appmanager.h"
#include "bankmanager.h"
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

class PebbledProxy;

class Manager : public QObject, protected QDBusContext
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    friend class PebbledProxy;

    QBluetoothLocalDevice btDevice;

    Settings *settings;

    PebbledProxy *proxy;

    WatchConnector *watch;
    DBusConnector *dbus;
    UploadManager *upload;
    AppManager *apps;
    BankManager *bank;
    VoiceCallManager *voice;
    NotificationManager *notifications;
    MusicManager *music;
    DataLogManager *datalog;
    AppMsgManager *appmsg;
    JSKitManager *js;

    MNotification notification;

    QContactManager *contacts;
    QContactDetailFilter numberFilter;

    QString defaultProfile;

    QUuid currentAppUuid;

    QScopedPointer<icu::Transliterator> transliterator;

public:
    explicit Manager(Settings *settings, QObject *parent = 0);
    ~Manager();

    QString findPersonByNumber(QString number);
    QString getCurrentProfile() const;

protected:
    void transliterateMessage(const QString &text);

public slots:
    void applyProfile();

private slots:
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

    void onAppNotification(const QUuid &uuid, const QString &title, const QString &body);
    void onAppOpened(const QUuid &uuid);
    void onAppClosed(const QUuid &uuid);
};

/** This class is what's actually exported over D-Bus,
 *  so the names of the slots and properties must match with org.pebbled.Watch D-Bus interface.
 *  Case sensitive. Otherwise, _runtime_ failures will occur. */
// Some of the methods are marked inline so that they may be inlined inside qt_metacall
class PebbledProxy : public QObject, protected QDBusContext
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    Q_PROPERTY(QString Name READ Name NOTIFY NameChanged)
    Q_PROPERTY(QString Address READ Address NOTIFY AddressChanged)
    Q_PROPERTY(bool Connected READ Connected NOTIFY ConnectedChanged)
    Q_PROPERTY(QString AppUuid READ AppUuid NOTIFY AppUuidChanged)
    Q_PROPERTY(QStringList AppSlots READ AppSlots NOTIFY AppSlotsChanged)
    Q_PROPERTY(QVariantList AllApps READ AllApps NOTIFY AllAppsChanged)

    inline Manager* manager() const { return static_cast<Manager*>(parent()); }
    inline QVariantMap pebble() const { return manager()->dbus->pebble(); }

public:
    inline explicit PebbledProxy(QObject *parent) : QObject(parent) {}

    inline QString Name() const { return pebble()["Name"].toString(); }
    inline QString Address() const { return pebble()["Address"].toString(); }
    inline bool Connected() const { return manager()->watch->isConnected(); }
    inline QString AppUuid() const { return manager()->currentAppUuid.toString(); }

    QStringList AppSlots() const;

    QVariantList AllApps() const;

public slots:
    inline void Disconnect() { manager()->watch->disconnect(); }
    inline void Reconnect() { manager()->watch->reconnect(); }
    inline void Ping(uint val) { manager()->watch->ping(val); }
    inline void SyncTime() { manager()->watch->time(); }

    inline void LaunchApp(const QString &uuid) { manager()->appmsg->launchApp(uuid); }
    inline void CloseApp(const QString &uuid) { manager()->appmsg->closeApp(uuid); }

    bool SendAppMessage(const QString &uuid, const QVariantMap &data);
    QString StartAppConfiguration(const QString &uuid);
    void SendAppConfigurationData(const QString &uuid, const QString &data);

    void UnloadApp(int slot);
    void UploadApp(const QString &uuid, int slot);

signals:
    void NameChanged();
    void AddressChanged();
    void ConnectedChanged();
    void AppUuidChanged();
    void AppSlotsChanged();
    void AllAppsChanged();
    void AppOpened(const QString &uuid);
    void AppClosed(const QString &uuid);
};

#endif // MANAGER_H
