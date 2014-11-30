#ifndef APPMSGMANAGER_H
#define APPMSGMANAGER_H

#include "watchconnector.h"

class AppMsgManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit AppMsgManager(WatchConnector *watch, QObject *parent);

public slots:
    void send(const QUuid &uuid, const QVariantMap &data);

signals:
    void appStarted(const QUuid &uuid);
    void appStopped(const QUuid &uuid);
    void dataReceived(const QUuid &uuid, const QVariantMap &data);

private:
    static QByteArray buildAckMessage(uint transaction);
    static QByteArray buildNackMessage(uint transaction);

private:
    WatchConnector *watch;
};

#endif // APPMSGMANAGER_H
