#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <QObject>
#include "watchconnector.h"

class MusicManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit MusicManager(WatchConnector *watch, QObject *parent = 0);

private:
    void musicControl(WatchConnector::MusicControl operation);

private slots:
    void onMprisMetadataChanged(QVariantMap metadata);

private:
    WatchConnector *watch;
};

#endif // MUSICMANAGER_H
