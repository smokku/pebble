#ifndef WATCHCOMMANDS_H
#define WATCHCOMMANDS_H

#include "watchconnector.h"

#include <QObject>

class WatchCommands : public QObject
{
    Q_OBJECT

    watch::WatchConnector *watch;

public:
    explicit WatchCommands(watch::WatchConnector *watch, QObject *parent = 0);

signals:
    void hangup();

public slots:
    void processMessage(uint endpoint, uint datalen, QByteArray data);

};

#endif // WATCHCOMMANDS_H
