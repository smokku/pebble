#include "dbusconnector.h"

#include <QDebug>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusObjectPath>

DBusConnector::DBusConnector(QObject *parent) :
    QObject(parent)
{
//dbus-send --system --dest=org.bluez --print-reply / org.bluez.Manager.ListAdapters
//dbus-send --system --dest=org.bluez --print-reply $path org.bluez.Adapter.GetProperties
//dbus-send --system --dest=org.bluez --print-reply $devpath org.bluez.Device.GetProperties
//dbus-send --system --dest=org.bluez --print-reply $devpath org.bluez.Input.Connect
    QDBusConnection session = QDBusConnection::sessionBus();

    findPebble();

}

bool DBusConnector::findPebble()
{
    QDBusConnection system = QDBusConnection::systemBus();

    QDBusReply<QList<QDBusObjectPath>> ListAdaptersReply = system.call(QDBusMessage::createMethodCall("org.bluez",
                                                                                                      "/",
                                                                                                      "org.bluez.Manager",
                                                                                                      "ListAdapters"));
    if (not ListAdaptersReply.isValid()) {
        qWarning() << ListAdaptersReply.error().message();
        return false;
    }

    QList<QDBusObjectPath> adapters = ListAdaptersReply.value();

    if (adapters.isEmpty()) {
        qWarning() << "No BT adapters found";
        return false;
    }

    QDBusReply<QHash<QString,QVariant>> AdapterPropertiesReply = system.call(QDBusMessage::createMethodCall("org.bluez",
                                                                                                            adapters.at(0).path(),
                                                                                                            "org.bluez.Adapter",
                                                                                                            "GetProperties"));
    if (not AdapterPropertiesReply.isValid()) {
        qWarning() << AdapterPropertiesReply.error().message();
        return false;
    }

    QList<QDBusObjectPath> devices;
    AdapterPropertiesReply.value().value("Devices").value<QDBusArgument>() >> devices;

    QString name;
    QString address;

    foreach (QDBusObjectPath path, devices) {
        QDBusReply<QHash<QString,QVariant>> DevicePropertiesReply = system.call(QDBusMessage::createMethodCall("org.bluez",
                                                                                                               path.path(),
                                                                                                               "org.bluez.Device",
                                                                                                               "GetProperties"));
        if (not DevicePropertiesReply.isValid()) {
            qWarning() << DevicePropertiesReply.error().message();
            continue;
        }

        const QHash<QString,QVariant> &dict = DevicePropertiesReply.value();

        QString tmp = dict.value("Name").toString();
        qDebug() << "Found BT device:" << tmp;
        if (tmp.startsWith("Pebble")) {
            name = tmp;
            address = dict.value("Address").toString();
            qDebug() << "Found Pebble:" << name << address;
        }
    }

    if (name.isEmpty() or address.isEmpty()) return false;

    pebbleName = name;
    pebbleAddress = address;
    return true;
}
