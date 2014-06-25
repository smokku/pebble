#ifndef DBUSCONNECTOR_H
#define DBUSCONNECTOR_H

#include <QObject>

class DBusConnector : public QObject
{
    Q_OBJECT
public:
    explicit DBusConnector(QObject *parent = 0);

    QString pebbleName;
    QString pebbleAddress;

signals:

public slots:
    bool findPebble();

};

#endif // DBUSCONNECTOR_H
