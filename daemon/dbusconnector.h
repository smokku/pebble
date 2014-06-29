#ifndef DBUSCONNECTOR_H
#define DBUSCONNECTOR_H

#include <QObject>
#include <QVariantMap>

class DBusConnector : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap pebble READ pebble NOTIFY pebbleChanged)

    QVariantMap pebbleProps;

public:
    explicit DBusConnector(QObject *parent = 0);

    QVariantMap pebble() { return pebbleProps; }

signals:
    void pebbleChanged();

public slots:
    bool findPebble();

};

#endif // DBUSCONNECTOR_H
