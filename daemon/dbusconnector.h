#ifndef DBUSCONNECTOR_H
#define DBUSCONNECTOR_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QLoggingCategory>

// TODO Remove this.

class DBusConnector : public QObject
{
    Q_OBJECT
    QLoggingCategory l;

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
