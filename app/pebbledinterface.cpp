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
    QString url = watch->StartAppConfiguration(uuid);
    return QUrl(url);
}

void PebbledInterface::setAppConfiguration(const QString &uuid, const QString &data)
{
    qDebug() << Q_FUNC_INFO << uuid << data;
    watch->SendAppConfigurationData(uuid, data);
}
