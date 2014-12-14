#include "pebbledinterface.h"
#include "watch_interface.h"

static const QString PEBBLED_SYSTEMD_UNIT("pebbled.service");
static const QString PEBBLED_DBUS_SERVICE("org.pebbled");
static const QString PEBBLED_DBUS_PATH("/org/pebbled/Watch");
static const QString PEBBLED_DBUS_IFACE("org.pebbled.Watch");

PebbledInterface::PebbledInterface(QObject *parent) :
    QObject(parent),
    systemd(new QDBusInterface("org.freedesktop.systemd1",
                               "/org/freedesktop/systemd1",
                               "org.freedesktop.systemd1.Manager",
                               QDBusConnection::sessionBus(), this)),
    watch(new OrgPebbledWatchInterface(PEBBLED_DBUS_SERVICE,
                                       PEBBLED_DBUS_PATH,
                                       QDBusConnection::sessionBus(), this))
{
    connect(watch, &OrgPebbledWatchInterface::NameChanged,
            this, &PebbledInterface::nameChanged);
    connect(watch, &OrgPebbledWatchInterface::AddressChanged,
            this, &PebbledInterface::addressChanged);
    connect(watch, &OrgPebbledWatchInterface::ConnectedChanged,
            this, &PebbledInterface::connectedChanged);
    connect(watch, &OrgPebbledWatchInterface::AppUuidChanged,
            this, &PebbledInterface::appUuidChanged);
    connect(watch, &OrgPebbledWatchInterface::AppSlotsChanged,
            this, &PebbledInterface::refreshAppSlots);
    connect(watch, &OrgPebbledWatchInterface::AllAppsChanged,
            this, &PebbledInterface::refreshAllApps);

    connect(watch, &OrgPebbledWatchInterface::ConnectedChanged,
            this, &PebbledInterface::onWatchConnectedChanged);

    // simulate connected change on active changed
    // as the daemon might not had a chance to send 'connectedChanged'
    // when going down
    connect(this, &PebbledInterface::activeChanged,
            this, &PebbledInterface::connectedChanged);

    systemd->call("Subscribe");

    QDBusReply<QDBusObjectPath> unit = systemd->call("LoadUnit", PEBBLED_SYSTEMD_UNIT);
    if (unit.isValid()) {
        unitPath = unit.value();

        getUnitProperties();

        QDBusConnection::sessionBus().connect(
                    "org.freedesktop.systemd1", unitPath.path(),
                    "org.freedesktop.DBus.Properties", "PropertiesChanged",
                    this, SLOT(onPropertiesChanged(QString,QMap<QString,QVariant>,QStringList)));
    } else {
        qWarning() << unit.error().message();
    }

    if (watch->isValid()) {
        refreshAllApps();
        refreshAppSlots();
    }
}

void PebbledInterface::getUnitProperties()
{
    QDBusMessage request = QDBusMessage::createMethodCall(
                "org.freedesktop.systemd1", unitPath.path(),
                "org.freedesktop.DBus.Properties", "GetAll");
    request << "org.freedesktop.systemd1.Unit";
    QDBusReply<QVariantMap> reply = QDBusConnection::sessionBus().call(request);
    if (reply.isValid()) {
        QVariantMap newProperties = reply.value();
        bool emitEnabledChanged = (unitProperties["UnitFileState"] != newProperties["UnitFileState"]);
        bool emitActiveChanged = (unitProperties["ActiveState"] != newProperties["ActiveState"]);
        unitProperties = newProperties;
        if (emitEnabledChanged) emit enabledChanged();
        if (emitActiveChanged) emit activeChanged();
    } else {
        qWarning() << reply.error().message();
    }
}

void PebbledInterface::onPropertiesChanged(QString interface, QMap<QString,QVariant> changed, QStringList invalidated)
{
    qDebug() << Q_FUNC_INFO << interface << changed << invalidated;
    if (interface != "org.freedesktop.systemd1.Unit") return;
    if (invalidated.contains("UnitFileState") || invalidated.contains("ActiveState"))
        getUnitProperties();
}

bool PebbledInterface::enabled() const
{
    qDebug() << Q_FUNC_INFO;
    return unitProperties["UnitFileState"].toString() == "enabled";
}

void PebbledInterface::setEnabled(bool enabled)
{
    qDebug() << "setEnabled" << enabled;
    QDBusError reply;
    if (enabled) systemd->call("EnableUnitFiles", QStringList() << PEBBLED_SYSTEMD_UNIT, false, true);
    else systemd->call("DisableUnitFiles", QStringList() << PEBBLED_SYSTEMD_UNIT, false);
    if (reply.isValid()) {
        qWarning() << reply.message();
    } else {
        systemd->call("Reload");
        getUnitProperties();
    }
}

bool PebbledInterface::active() const
{
    qDebug() << Q_FUNC_INFO;
    return unitProperties["ActiveState"].toString() == "active";
}

void PebbledInterface::setActive(bool active)
{
    qDebug() << "setActive" << active;
    QDBusReply<QDBusObjectPath> reply = systemd->call(active?"StartUnit":"StopUnit", PEBBLED_SYSTEMD_UNIT, "replace");
    if (!reply.isValid()) {
        qWarning() << reply.error().message();
    }
}

bool PebbledInterface::connected() const
{
    qDebug() << Q_FUNC_INFO;
    return watch->connected();
}

QString PebbledInterface::name() const
{
    qDebug() << Q_FUNC_INFO;
    return watch->name();
}

QString PebbledInterface::address() const
{
    qDebug() << Q_FUNC_INFO;
    return watch->address();
}

QString PebbledInterface::appUuid() const
{
    qDebug() << Q_FUNC_INFO;
    return watch->appUuid();
}

void PebbledInterface::ping()
{
    qDebug() << Q_FUNC_INFO;
    watch->Ping(66);
}

void PebbledInterface::time()
{
    qDebug() << Q_FUNC_INFO;
    watch->SyncTime();
}

void PebbledInterface::disconnect()
{
    qDebug() << Q_FUNC_INFO;
    watch->Disconnect();
}

void PebbledInterface::reconnect()
{
    qDebug() << Q_FUNC_INFO;
    watch->Reconnect();
}

QUrl PebbledInterface::configureApp(const QString &uuid)
{
    qDebug() << Q_FUNC_INFO << uuid;
    QDBusPendingReply<QString> reply = watch->StartAppConfiguration(uuid);
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Received error:" << reply.error().message();
        return QUrl();
    } else {
        return QUrl(reply.value());
    }
}

bool PebbledInterface::isAppInstalled(const QString &uuid) const
{
    QUuid u(uuid);

    foreach (const QString &s, _appSlots) {
        if (QUuid(s) == u) {
            return true;
        }
    }

    return false;
}

QImage PebbledInterface::menuIconForApp(const QUuid &uuid) const
{
    return _appMenuIcons.value(uuid);
}

void PebbledInterface::setAppConfiguration(const QString &uuid, const QString &data)
{
    qDebug() << Q_FUNC_INFO << uuid << data;
    watch->SendAppConfigurationData(uuid, data);
}

void PebbledInterface::launchApp(const QString &uuid)
{
    qDebug() << Q_FUNC_INFO << uuid;
    QDBusPendingReply<> reply = watch->LaunchApp(uuid);
    reply.waitForFinished();

    // TODO Terrible hack; need to give time for the watch to open the app
    // A better solution would be to wait until AppUuidChanged is generated.
    QUuid u(uuid);
    if (u.isNull()) return;
    int sleep_count = 0;
    while (QUuid(watch->appUuid()) != u && sleep_count < 5) {
        qDebug() << "Waiting for" << u.toString() << "to launch";
        QThread::sleep(1);
        sleep_count++;
    }
}

void PebbledInterface::uploadApp(const QString &uuid, int slot)
{
    qDebug() << Q_FUNC_INFO << uuid << slot;
    QDBusPendingReply<> reply = watch->UploadApp(uuid, slot);
    reply.waitForFinished();
}

void PebbledInterface::unloadApp(int slot)
{
    qDebug() << Q_FUNC_INFO << slot;
    QDBusPendingReply<> reply = watch->UnloadApp(slot);
    reply.waitForFinished();
}

QStringList PebbledInterface::appSlots() const
{
    return _appSlots;
}

QVariantList PebbledInterface::allApps() const
{
    return _apps;
}

QVariantMap PebbledInterface::appInfoByUuid(const QString &uuid) const
{
    int index = _appsByUuid.value(QUuid(uuid), -1);
    if (index >= 0) {
        return _apps[index].toMap();
    } else {
        return QVariantMap();
    }
}

void PebbledInterface::onWatchConnectedChanged()
{
    qDebug() << Q_FUNC_INFO;
    if (watch->connected()) {
        refreshAllApps();
        refreshAppSlots();
    }
}

void PebbledInterface::refreshAppSlots()
{
    qDebug() << "refreshing app slots list";
    _appSlots = watch->appSlots();
    emit appSlotsChanged();
}

void PebbledInterface::refreshAllApps()
{
    _apps.clear();
    _appsByUuid.clear();
    _appMenuIcons.clear();

    qDebug() << "refreshing all apps list";

    const QVariantList l = watch->allApps();
    foreach (const QVariant &v, l) {
        QVariantMap orig = qdbus_cast<QVariantMap>(v.value<QDBusArgument>());
        QUuid uuid = orig.value("uuid").toUuid();
        if (uuid.isNull()) {
            qWarning() << "Invalid app uuid received" << orig;
            continue;
        }

        QVariantMap m;
        m.insert("uuid", uuid.toString());
        m.insert("shortName", orig.value("short-name"));
        m.insert("longName", orig.value("long-name"));

        QByteArray pngIcon = orig.value("menu-icon").toByteArray();
        if (!pngIcon.isEmpty()) {
            _appMenuIcons.insert(uuid, QImage::fromData(pngIcon, "PNG"));
        }

        _apps.append(QVariant::fromValue(m));
    }

    std::sort(_apps.begin(), _apps.end(), [](const QVariant &v1, const QVariant &v2) {
        const QVariantMap &a = v1.toMap();
        const QVariantMap &b = v2.toMap();
        return a.value("shortName").toString() < b.value("shortName").toString();
    });

    for (int i = 0; i < _apps.size(); ++i) {
        QUuid uuid = _apps[i].toMap().value("uuid").toUuid();
        _appsByUuid.insert(uuid, i);
    }

    qDebug() << _appsByUuid.size() << "different app uuids known";

    emit allAppsChanged();
}
