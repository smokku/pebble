#include "dbusconnector.h"

#include <QDebug>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusConnectionInterface>

//dbus-send --system --dest=org.bluez --print-reply / org.bluez.Manager.ListAdapters
//dbus-send --system --dest=org.bluez --print-reply $path org.bluez.Adapter.GetProperties
//dbus-send --system --dest=org.bluez --print-reply $devpath org.bluez.Device.GetProperties
//dbus-send --system --dest=org.bluez --print-reply $devpath org.bluez.Input.Connect

DBusConnector::DBusConnector(QObject *parent) :
    QObject(parent)
{
    QDBusConnectionInterface *interface = QDBusConnection::sessionBus().interface();

    QDBusReply<QStringList> serviceNames = interface->registeredServiceNames();
    if (serviceNames.isValid()) {
        dbusServices = serviceNames.value();
    }
    else {
        logger()->error() << serviceNames.error().message();
    }
    connect(interface, SIGNAL(serviceRegistered(const QString &)), SLOT(onServiceRegistered(const QString &)));
    connect(interface, SIGNAL(serviceUnregistered(const QString &)), SLOT(onServiceUnregistered(const QString &)));
}

bool DBusConnector::findPebble()
{
    QDBusConnection system = QDBusConnection::systemBus();

    QDBusReply<QList<QDBusObjectPath>> ListAdaptersReply = system.call(
                QDBusMessage::createMethodCall("org.bluez", "/", "org.bluez.Manager",
                                               "ListAdapters"));
    if (not ListAdaptersReply.isValid()) {
        logger()->error() << ListAdaptersReply.error().message();
        return false;
    }

    QList<QDBusObjectPath> adapters = ListAdaptersReply.value();

    if (adapters.isEmpty()) {
        logger()->debug() << "No BT adapters found";
        return false;
    }

    QDBusReply<QVariantMap> AdapterPropertiesReply = system.call(
                QDBusMessage::createMethodCall("org.bluez", adapters[0].path(), "org.bluez.Adapter",
                                               "GetProperties"));
    if (not AdapterPropertiesReply.isValid()) {
        logger()->error() << AdapterPropertiesReply.error().message();
        return false;
    }

    QList<QDBusObjectPath> devices;
    AdapterPropertiesReply.value()["Devices"].value<QDBusArgument>() >> devices;

    foreach (QDBusObjectPath path, devices) {
        QDBusReply<QVariantMap> DevicePropertiesReply = system.call(
                    QDBusMessage::createMethodCall("org.bluez", path.path(), "org.bluez.Device",
                                                   "GetProperties"));
        if (not DevicePropertiesReply.isValid()) {
            logger()->error() << DevicePropertiesReply.error().message();
            continue;
        }

        const QVariantMap &dict = DevicePropertiesReply.value();

        QString tmp = dict["Name"].toString();
        logger()->debug() << "Found BT device:" << tmp;
        if (tmp.startsWith("Pebble")) {
            logger()->debug() << "Found Pebble:" << tmp;
            pebbleProps = dict;
            emit pebbleChanged();
            return true;
        }
    }

    return false;
}

void DBusConnector::onServiceRegistered(const QString &name)
{
    logger()->debug() << "DBus service online:" << name;
    if (!dbusServices.contains(name)) dbusServices.append(name);
}

void DBusConnector::onServiceUnregistered(const QString &name)
{
    logger()->debug() << "DBus service offline:" << name;
    if (dbusServices.contains(name)) dbusServices.removeAll(name);
}
