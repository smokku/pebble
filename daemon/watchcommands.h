#ifndef WATCHCOMMANDS_H
#define WATCHCOMMANDS_H

#include "watchconnector.h"
#include <QLoggingCategory>

#include <QObject>

class WatchCommands : public QObject
{
    Q_OBJECT
    QLoggingCategory l;

    watch::WatchConnector *watch;

public:
    explicit WatchCommands(watch::WatchConnector *watch, QObject *parent = 0);

signals:
    void hangup();

public slots:
    void processMessage(uint endpoint, QByteArray data);

protected slots:
    void onMprisMetadataChanged(QVariantMap metadata);
    void musicControl(watch::WatchConnector::MusicControl operation);

};

#endif // WATCHCOMMANDS_H
