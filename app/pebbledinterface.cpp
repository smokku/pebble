#include "pebbledinterface.h"

QString PebbledInterface::PEBBLED_SYSTEMD_UNIT("pebbled.service");
QString PebbledInterface::PEBBLED_DBUS_SERVICE("org.pebbled");
QString PebbledInterface::PEBBLED_DBUS_PATH("/");
QString PebbledInterface::PEBBLED_DBUS_IFACE("org.pebbled");


PebbledInterface::PebbledInterface(QObject *parent) :
    QObject(parent), pebbled(0), systemd(0)
{
    QDBusConnection::sessionBus().connect(
                PEBBLED_DBUS_SERVICE, PEBBLED_DBUS_PATH, PEBBLED_DBUS_IFACE,
                "connectedChanged", this, SIGNAL(connectedChanged()));

    QDBusConnection::sessionBus().connect(
                PEBBLED_DBUS_SERVICE, PEBBLED_DBUS_PATH, PEBBLED_DBUS_IFACE,
                "pebbleChanged", this, SLOT(onPebbleChanged()));

    // simulate connected change on active changed
    // as the daemon might not had a chance to send connectedChanged()
    connect(this, SIGNAL(activeChanged()), SIGNAL(connectedChanged()));

    pebbled = new QDBusInterface(PEBBLED_DBUS_SERVICE, PEBBLED_DBUS_PATH, PEBBLED_DBUS_IFACE,
                                 QDBusConnection::sessionBus(), this);

    systemd = new QDBusInterface("org.freedesktop.systemd1",
                                 "/org/freedesktop/systemd1",
                                 "org.freedesktop.systemd1.Manager",
                                 QDBusConnection::sessionBus(), this);

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
        bool emitEnabledChanged = (properties["UnitFileState"] != newProperties["UnitFileState"]);
        bool emitActiveChanged = (properties["ActiveState"] != newProperties["ActiveState"]);
        properties = newProperties;
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
    if (invalidated.contains("UnitFileState") or invalidated.contains("ActiveState"))
        getUnitProperties();
}

void PebbledInterface::onPebbleChanged()
{
    qDebug() << __FUNCTION__;
    emit nameChanged();
    emit addressChanged();
    emit pebbleChanged();
}

bool PebbledInterface::enabled() const
{
    qDebug() << "enabled()";
    return properties["UnitFileState"].toString() == "enabled";
}

void PebbledInterface::setEnabled(bool enabled)
{
    if (systemd) {
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
}

bool PebbledInterface::active() const
{
    qDebug() << "active()";
    return properties["ActiveState"].toString() == "active";
}

void PebbledInterface::setActive(bool active)
{
    if (systemd) {
        qDebug() << "setActive" << active;
        QDBusReply<QDBusObjectPath> reply = systemd->call(active?"StartUnit":"StopUnit", PEBBLED_SYSTEMD_UNIT, "replace");
        if (!reply.isValid()) {
            qWarning() << reply.error().message();
        }
    }
}

bool PebbledInterface::connected() const
{
    qDebug() << __FUNCTION__;
    return pebbled->property(__FUNCTION__).toBool();
}

QVariantMap PebbledInterface::pebble() const
{
    qDebug() << __FUNCTION__;
    return pebbled->property(__FUNCTION__).toMap();
}

QString PebbledInterface::name() const
{
    qDebug() << __FUNCTION__;
    return pebbled->property(__FUNCTION__).toString();
}

QString PebbledInterface::address() const
{
    qDebug() << __FUNCTION__;
    return pebbled->property(__FUNCTION__).toString();
}

void PebbledInterface::ping()
{
    pebbled->call("ping", 66);
}

void PebbledInterface::time()
{
    pebbled->call("time");
}

void PebbledInterface::disconnect()
{
    pebbled->call("disconnect");
}

void PebbledInterface::reconnect()
{
    pebbled->call("reconnect");
}
