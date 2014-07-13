#ifndef DBUSCONNECTOR_H
#define DBUSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include "Logger"

class DBusConnector : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    Q_PROPERTY(QVariantMap pebble READ pebble NOTIFY pebbleChanged)
    Q_PROPERTY(QStringList services READ services NOTIFY servicesChanged)

    QVariantMap pebbleProps;
    QStringList dbusServices;

public:
    explicit DBusConnector(QObject *parent = 0);

    QVariantMap pebble() { return pebbleProps; }
    QStringList services() { return dbusServices; }

signals:
    void pebbleChanged();
    void servicesChanged();

public slots:
    bool findPebble();

protected slots:
    void onServiceRegistered(QString&);
    void onServiceUnregistered(QString&);

};

#endif // DBUSCONNECTOR_H
