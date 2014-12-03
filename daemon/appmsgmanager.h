#ifndef APPMSGMANAGER_H
#define APPMSGMANAGER_H

#include <functional>
#include <QUuid>
#include <QQueue>

#include "watchconnector.h"
#include "appmanager.h"

class AppMsgManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit AppMsgManager(AppManager *apps, WatchConnector *watch, QObject *parent);

    void send(const QUuid &uuid, const QVariantMap &data,
              const std::function<void()> &ackCallback,
              const std::function<void()> &nackCallback);

public slots:
    void send(const QUuid &uuid, const QVariantMap &data);
    void launchApp(const QUuid &uuid);
    void closeApp(const QUuid &uuid);

signals:
    void appStarted(const QUuid &uuid);
    void appStopped(const QUuid &uuid);
    void messageReceived(const QUuid &uuid, const QVariantMap &data);

private:
    WatchConnector::Dict mapAppKeys(const QUuid &uuid, const QVariantMap &data);
    QVariantMap mapAppKeys(const QUuid &uuid, const WatchConnector::Dict &dict);

    static bool unpackPushMessage(const QByteArray &msg, quint8 *transaction, QUuid *uuid, WatchConnector::Dict *dict);

    static QByteArray buildPushMessage(quint8 transaction, const QUuid &uuid, const WatchConnector::Dict &dict);
    static QByteArray buildAckMessage(quint8 transaction);
    static QByteArray buildNackMessage(quint8 transaction);

    void handleLauncherPushMessage(const QByteArray &data);
    void handlePushMessage(const QByteArray &data);
    void handleAckMessage(const QByteArray &data, bool ack);

    void transmitNextPendingTransaction();
    void abortPendingTransactions();

private slots:
    void handleWatchConnectedChanged();
    void handleTimeout();

private:
    AppManager *apps;
    WatchConnector *watch;
    quint8 lastTransactionId;

    struct PendingTransaction {
        quint8 transactionId;
        QUuid uuid;
        WatchConnector::Dict dict;
        std::function<void()> ackCallback;
        std::function<void()> nackCallback;
    };
    QQueue<PendingTransaction> pending;
    QTimer *timeout;
};

#endif // APPMSGMANAGER_H
