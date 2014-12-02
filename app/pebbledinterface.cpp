#include "pebbledinterface.h"
#include "watch_interface.h"

static const QString PEBBLED_SYSTEMD_UNIT("pebbled.service");
static const QString PEBBLED_DBUS_SERVICE("org.pebbled");
static const QString PEBBLED_DBUS_PATH("/org/pebbled/watch");
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
    qDebug() << __FUNCTION__ << interface << changed << invalidated;
    if (interface != "org.freedesktop.systemd1.Unit") return;
    if (invalidated.contains("UnitFileState") || invalidated.contains("ActiveState"))
        getUnitProperties();
}

bool PebbledInterface::enabled() const
{
    qDebug() << __FUNCTION__;
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
    qDebug() << __FUNCTION__;
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
    qDebug() << __FUNCTION__;
    return watch->connected();
}

QString PebbledInterface::name() const
{
    qDebug() << __FUNCTION__;
    return watch->name();
}

QString PebbledInterface::address() const
{
    qDebug() << __FUNCTION__;
    return watch->address();
}

void PebbledInterface::ping()
{
    qDebug() << __FUNCTION__;
    watch->Ping(66);
}

void PebbledInterface::time()
{
    qDebug() << __FUNCTION__;
    watch->SyncTime();
}

void PebbledInterface::disconnect()
{
    qDebug() << __FUNCTION__;
    watch->Disconnect();
}

void PebbledInterface::reconnect()
{
    qDebug() << __FUNCTION__;
    watch->Reconnect();
}

QUrl PebbledInterface::configureApp(const QUuid &uuid)
{
    qDebug() << __FUNCTION__ << uuid;
    QString url = watch->StartAppConfiguration(uuid.toString());
    return QUrl(url);
}

void PebbledInterface::setAppConfiguration(const QUuid &uuid, const QString &data)
{
    watch->SendAppConfigurationData(uuid.toString(), data);
}
