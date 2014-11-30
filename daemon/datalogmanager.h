#ifndef DATALOGMANAGER_H
#define DATALOGMANAGER_H

#include "watchconnector.h"

class DataLogManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit DataLogManager(WatchConnector *watch, QObject *parent = 0);

signals:

public slots:

private:
    void handleDataCommand(int session, const QByteArray &data);

private:
    WatchConnector *watch;
};

#endif // DATALOGMANAGER_H
