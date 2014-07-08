#include "pebbledinterface.h"

QString PebbledInterface::PEBBLED_SYSTEMD_UNIT("pebbled.service");
QString PebbledInterface::SYSTEMD_UNIT_IFACE("org.freedesktop.systemd1.Unit");


PebbledInterface::PebbledInterface(QObject *parent) :
    QObject(parent), pebbled(0), systemd(0), unitprops(0)
{
    pebbled = new QDBusInterface("org.pebbled",
                                 "/",
                                 "org.pebbled",
                                 QDBusConnection::sessionBus(), this);
    pebbled->connection()
            .connect(pebbled->service(), pebbled->path(), pebbled->interface(),
                     "connectedChanged", this, SIGNAL(connectedChanged()));
    pebbled->connection()
            .connect(pebbled->service(), pebbled->path(), pebbled->interface(),
                     "pebbleChanged", this, SLOT(onPebbleChanged()));

    // simulate connected change on active changed
    connect(this, SIGNAL(activeChanged()), this, SIGNAL(connectedChanged()));

    systemd = new QDBusInterface("org.freedesktop.systemd1",
                                 "/org/freedesktop/systemd1",
                                 "org.freedesktop.systemd1.Manager",
                                 QDBusConnection::sessionBus(), this);

    systemd->call("Subscribe");

    QDBusReply<QDBusObjectPath> unit = systemd->call("LoadUnit", PEBBLED_SYSTEMD_UNIT);
    if (not unit.isValid()) {
        qWarning() << unit.error().message();
    } else {
        unitprops = new QDBusInterface("org.freedesktop.systemd1",
                                        unit.value().path(),
                                        "org.freedesktop.DBus.Properties",
                                        QDBusConnection::sessionBus(), this);
        getUnitProperties();

        unitprops->connection()
                .connect("org.freedesktop.systemd1",
                         unitprops->path(),
                         "org.freedesktop.DBus.Properties",
                         "PropertiesChanged",
                         this,
                         SLOT(onPropertiesChanged(QString,QMap<QString,QVariant>,QStringList))
                        );
    }
}

void PebbledInterface::getUnitProperties()
{
    QDBusReply<QVariantMap> reply = unitprops->call("GetAll", SYSTEMD_UNIT_IFACE);
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
