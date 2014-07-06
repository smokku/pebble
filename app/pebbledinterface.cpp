#include "pebbledinterface.h"

PebbledInterface::PebbledInterface(QObject *parent) :
    QObject(parent),
    systemd()
{
    systemd = new QDBusInterface("org.freedesktop.systemd1",
                                 "/org/freedesktop/systemd1",
                                 "org.freedesktop.systemd1.Manager",
                                 QDBusConnection::systemBus(), this);

    pebbled = new QDBusInterface("org.pebbled",
                                 "/",
                                 "org.pebbled",
                                 QDBusConnection::sessionBus(), this);

    QDBusReply<QDBusObjectPath> unit = systemd->call("LoadUnit", "pebbled.service");
    if (not unit.isValid()) {
        qWarning() << unit.error().message();
    } else {
        systemdUnit = unit.value().path();
    }
    qDebug() << "pebbled.service unit:" << systemdUnit;
}

bool PebbledInterface::enabled() const
{
    qDebug() << "enabled()";
    // FIXME: implement
    return true;
}

void PebbledInterface::setEnabled(bool enabled)
{
    bool doEmit = (this->enabled() != enabled);
    qDebug() << "setEnabled" << this->enabled() << enabled;
    // FIXME: implement
    if (doEmit) emit enabledChanged();
}

bool PebbledInterface::active() const
{
    qDebug() << "active()";
    // FIXME: implement
    return true;
}

void PebbledInterface::setActive(bool active)
{
    bool doEmit = (this->active() != active);
    qDebug() << "setActive" << this->active() << active;
    // FIXME: implement
    if (doEmit) emit activeChanged();
}

bool PebbledInterface::connected() const
{
    qDebug() << "connected()";
    // FIXME: implement
    return true;
}

QVariantMap PebbledInterface::pebble() const
{
    qDebug() << "pebble()";
    // FIXME: implement
    return QVariantMap();
}

QString PebbledInterface::name() const
{
    qDebug() << "name()";
    // FIXME: implement
    return QString("Pebble XXX");
}

QString PebbledInterface::address() const
{
    qDebug() << "address()";
    // FIXME: implement
    return QString("");
}
