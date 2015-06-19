#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
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
#include <QDBusContext>
#include <QtContacts/QContactManager>
#include <QtContacts/QContactDetailFilter>
#include <MNotification>
#include <QLoggingCategory>

#include <unicode/translit.h>

using namespace QtContacts;

class PebbledProxy;

class Manager : public QObject, protected QDBusContext
{
    Q_OBJECT
    QLoggingCategory l;

    friend class PebbledProxy;

    Settings *settings;

    PebbledProxy *proxy;

    WatchConnector *watch;
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

    QUuid currentAppUuid;

    QScopedPointer<icu::Transliterator> transliterator;

public:
    explicit Manager(Settings *settings, QObject *parent = 0);
    ~Manager();

    QString findPersonByNumber(QString number);

    bool uploadFirmware(bool recovery, const QString &file);

protected:
    void transliterateMessage(const QString &text);

public slots:
    void applyProfile();
    void ping(uint val);

private slots:
    void onSettingChanged(const QString &key);
    void onSettingsChanged();
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
    QLoggingCategory l;

    Q_PROPERTY(QString Name READ Name NOTIFY NameChanged)
    Q_PROPERTY(QString Address READ Address NOTIFY AddressChanged)
    Q_PROPERTY(QVariantMap Info READ Info NOTIFY InfoChanged)
    Q_PROPERTY(bool Connected READ Connected NOTIFY ConnectedChanged)
    Q_PROPERTY(QString AppUuid READ AppUuid NOTIFY AppUuidChanged)
    Q_PROPERTY(QStringList AppSlots READ AppSlots NOTIFY AppSlotsChanged)
    Q_PROPERTY(QVariantList AllApps READ AllApps NOTIFY AllAppsChanged)

    inline Manager* manager() const { return static_cast<Manager*>(parent()); }

public:
    inline explicit PebbledProxy(QObject *parent)
        : QObject(parent), l(metaObject()->className()) {}

    inline QString Name() const { qCDebug(l) << manager()->watch->name(); return manager()->watch->name(); }
    inline QString Address() const { qCDebug(l) << manager()->watch->address().toString(); return manager()->watch->address().toString(); }
    inline QVariantMap Info() const { qCDebug(l) << manager()->watch->versions().toMap(); return manager()->watch->versions().toMap(); }
    inline bool Connected() const { qCDebug(l) << manager()->watch->isConnected(); return manager()->watch->isConnected(); }
    inline QString AppUuid() const { return manager()->currentAppUuid.toString(); }

    QStringList AppSlots() const;

    QVariantList AllApps() const;

public slots:
    inline void Disconnect() { manager()->watch->disconnect(); }
    inline void Reconnect() { manager()->watch->connect(); }
    inline void Ping(uint val) { manager()->ping(val); }
    inline void SyncTime() { manager()->watch->time(); }

    inline void LaunchApp(const QString &uuid) { manager()->appmsg->launchApp(uuid); }
    inline void CloseApp(const QString &uuid) { manager()->appmsg->closeApp(uuid); }

    bool SendAppMessage(const QString &uuid, const QVariantMap &data);
    QString StartAppConfiguration(const QString &uuid);
    void SendAppConfigurationData(const QString &uuid, const QString &data);

    void UnloadApp(int slot);
    void UploadApp(const QString &uuid, int slot);

    void NotifyFirmware(bool ok);
    void UploadFirmware(bool recovery, const QString &file);

signals:
    void NameChanged();
    void AddressChanged();
    void InfoChanged();
    void ConnectedChanged();
    void AppUuidChanged();
    void AppSlotsChanged();
    void AllAppsChanged();
    void AppOpened(const QString &uuid);
    void AppClosed(const QString &uuid);
};

#endif // MANAGER_H
