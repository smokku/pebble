#ifndef DBUSCONNECTOR_H
#define DBUSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <Log4Qt/Logger>

// TODO Remove this.

class DBusConnector : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    Q_PROPERTY(QVariantMap pebble READ pebble NOTIFY pebbleChanged)
    QVariantMap pebbleProps;

public:
    explicit DBusConnector(QObject *parent = 0);

    QVariantMap pebble() const { return pebbleProps; }

signals:
    void pebbleChanged();

public slots:
    bool findPebble();
};

#endif // DBUSCONNECTOR_H
