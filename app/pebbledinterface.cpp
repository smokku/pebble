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

    systemd = new QDBusInterface("org.freedesktop.systemd1",
                                 "/org/freedesktop/systemd1",
                                 "org.freedesktop.systemd1.Manager",
                                 QDBusConnection::sessionBus(), this);

    QDBusReply<QDBusObjectPath> unit = systemd->call("LoadUnit", PEBBLED_SYSTEMD_UNIT);
    if (not unit.isValid()) {
        qWarning() << unit.error().message();
    } else {
        unitprops = new QDBusInterface("org.freedesktop.systemd1",
                                        unit.value().path(),
                                        "org.freedesktop.DBus.Properties",
                                        QDBusConnection::sessionBus(), this);
        //connect(unitprops, SIGNAL(PropertiesChanged(QString,QMap<QString,QVariant>,QStringList)), SLOT(onPropertiesChanged(QString,QMap<QString,QVariant>,QStringList)));
        unitprops->connection().connect("org.freedesktop.systemd1",
                                        unitprops->path(),
                          SYSTEMD_UNIT_IFACE,
                          "PropertyChanged",
                          this,
                          SLOT(propertyChanged(QString,QDBusVariant))
                          );




        QDBusReply<QVariantMap> reply = unitprops->call("GetAll", SYSTEMD_UNIT_IFACE);
        if (reply.isValid()) {
            QMap<QString,QVariant> map = reply.value();
            //onPropertiesChanged(SYSTEMD_UNIT_IFACE, map, QStringList());
        } else {
            qWarning() << reply.error().message();
        }

    }
}

void PebbledInterface::onPropertyChanged(QString string,QDBusVariant dbv)
{
    qDebug() << string << dbv.variant();
}


bool PebbledInterface::enabled() const
{
    qDebug() << "enabled()";
    //
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
