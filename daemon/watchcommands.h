#ifndef WATCHCOMMANDS_H
#define WATCHCOMMANDS_H

#include "watchconnector.h"
#include "Logger"

#include <QObject>

class WatchCommands : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    watch::WatchConnector *watch;

public:
    explicit WatchCommands(watch::WatchConnector *watch, QObject *parent = 0);

signals:
    void hangup();

public slots:
    void processMessage(uint endpoint, uint datalen, QByteArray data);

protected slots:
    void onMprisMetadataChanged(QVariantMap metadata);

};

#endif // WATCHCOMMANDS_H
