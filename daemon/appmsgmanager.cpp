#include <QTimer>

#include "appmsgmanager.h"
#include "unpacker.h"
#include "packer.h"

// TODO D-Bus server for non JS kit apps!!!!

AppMsgManager::AppMsgManager(AppManager *apps, WatchConnector *watch, QObject *parent)
    : QObject(parent), apps(apps), watch(watch), _lastTransactionId(0), _timeout(new QTimer(this))
{
    connect(watch, &WatchConnector::connectedChanged,
            this, &AppMsgManager::handleWatchConnectedChanged);

    _timeout->setSingleShot(true);
    _timeout->setInterval(3000);
    connect(_timeout, &QTimer::timeout,
            this, &AppMsgManager::handleTimeout);

    watch->setEndpointHandler(WatchConnector::watchLAUNCHER,
                              [this](const QByteArray &data) {
        switch (data.at(0)) {
        case WatchConnector::appmsgPUSH:
            handleLauncherPushMessage(data);
            break;
        case WatchConnector::appmsgACK:
        case WatchConnector::appmsgNACK:
            // TODO we ignore those for now.
            break;
        }

        return true;
    });

    watch->setEndpointHandler(WatchConnector::watchAPPLICATION_MESSAGE,
                              [this](const QByteArray &data) {
        switch (data.at(0)) {
        case WatchConnector::appmsgPUSH:
            handlePushMessage(data);
            break;
        case WatchConnector::appmsgACK:
            handleAckMessage(data, true);
            break;
        case WatchConnector::appmsgNACK:
            handleAckMessage(data, false);
            break;
        default:
            logger()->warn() << "Unknown application message type:" << int(data.at(0));
            break;
        }

        return true;
    });
}

void AppMsgManager::send(const QUuid &uuid, const QVariantMap &data, const std::function<void ()> &ackCallback, const std::function<void ()> &nackCallback)
{
    PendingTransaction trans;
    trans.uuid = uuid;
    trans.transactionId = ++_lastTransactionId;
    trans.dict = mapAppKeys(uuid, data);
    trans.ackCallback = ackCallback;
    trans.nackCallback = nackCallback;

    logger()->debug() << "Queueing appmsg" << trans.transactionId << "to" << trans.uuid
                      << "with dict" << trans.dict;

    _pending.enqueue(trans);
    if (_pending.size() == 1) {
        // This is the only transaction on the queue
        // Therefore, we were idle before: we can submit this transaction right now.
        transmitNextPendingTransaction();
    }
}

void AppMsgManager::setMessageHandler(const QUuid &uuid, MessageHandlerFunc func)
{
    _handlers.insert(uuid, func);
}

void AppMsgManager::clearMessageHandler(const QUuid &uuid)
{
    _handlers.remove(uuid);
}

uint AppMsgManager::lastTransactionId() const
{
    return _lastTransactionId;
}

uint AppMsgManager::nextTransactionId() const
{
    return _lastTransactionId + 1;
}

void AppMsgManager::send(const QUuid &uuid, const QVariantMap &data)
{
    std::function<void()> nullCallback;
    send(uuid, data, nullCallback, nullCallback);
}

void AppMsgManager::launchApp(const QUuid &uuid)
{
    WatchConnector::Dict dict;
    dict.insert(1, WatchConnector::launcherSTARTED);

    logger()->debug() << "Sending message to launcher" << uuid << dict;

    QByteArray msg = buildPushMessage(++_lastTransactionId, uuid, dict);
    watch->sendMessage(WatchConnector::watchLAUNCHER, msg);
}

void AppMsgManager::closeApp(const QUuid &uuid)
{
    WatchConnector::Dict dict;
    dict.insert(1, WatchConnector::launcherSTOPPED);

    logger()->debug() << "Sending message to launcher" << uuid << dict;

    QByteArray msg = buildPushMessage(++_lastTransactionId, uuid, dict);
    watch->sendMessage(WatchConnector::watchLAUNCHER, msg);
}

WatchConnector::Dict AppMsgManager::mapAppKeys(const QUuid &uuid, const QVariantMap &data)
{
    AppInfo info = apps->info(uuid);
    if (info.uuid() != uuid) {
        logger()->warn() << "Unknown app GUID while sending message:" << uuid;
    }

    WatchConnector::Dict d;

    for (QVariantMap::const_iterator it = data.constBegin(); it != data.constEnd(); ++it) {
        if (info.hasAppKey(it.key())) {
            d.insert(info.valueForAppKey(it.key()), it.value());
        } else {
            // Even if we do not know about this appkey, try to see if it's already a numeric key we
            // can send to the watch.
            bool ok = false;
            int num = it.key().toInt(&ok);
            if (ok) {
                d.insert(num, it.value());
            } else {
                logger()->warn() << "Unknown appKey" << it.key() << "for app with GUID" << uuid;
            }
        }
    }

    return d;
}

QVariantMap AppMsgManager::mapAppKeys(const QUuid &uuid, const WatchConnector::Dict &dict)
{
    AppInfo info = apps->info(uuid);
    if (info.uuid() != uuid) {
        logger()->warn() << "Unknown app GUID while sending message:" << uuid;
    }

    QVariantMap data;

    for (WatchConnector::Dict::const_iterator it = dict.constBegin(); it != dict.constEnd(); ++it) {
        if (info.hasAppKeyValue(it.key())) {
            data.insert(info.appKeyForValue(it.key()), it.value());
        } else {
            logger()->warn() << "Unknown appKey value" << it.key() << "for app with GUID" << uuid;
            data.insert(QString::number(it.key()), it.value());
        }
    }

    return data;
}

bool AppMsgManager::unpackPushMessage(const QByteArray &msg, quint8 *transaction, QUuid *uuid, WatchConnector::Dict *dict)
{
    Unpacker u(msg);
    quint8 code = u.read<quint8>();
    Q_ASSERT(code == WatchConnector::appmsgPUSH);

    *transaction = u.read<quint8>();
    *uuid = u.readUuid();
    *dict = u.readDict();

    if (u.bad()) {
        return false;
    }

    return true;
}

QByteArray AppMsgManager::buildPushMessage(quint8 transaction, const QUuid &uuid, const WatchConnector::Dict &dict)
{
    QByteArray ba;
    Packer p(&ba);
    p.write<quint8>(WatchConnector::appmsgPUSH);
    p.write<quint8>(transaction);
    p.writeUuid(uuid);
    p.writeDict(dict);

    return ba;
}

QByteArray AppMsgManager::buildAckMessage(quint8 transaction)
{
    QByteArray ba(2, Qt::Uninitialized);
    ba[0] = WatchConnector::appmsgACK;
    ba[1] = transaction;
    return ba;
}

QByteArray AppMsgManager::buildNackMessage(quint8 transaction)
{
    QByteArray ba(2, Qt::Uninitialized);
    ba[0] = WatchConnector::appmsgNACK;
    ba[1] = transaction;
    return ba;
}

void AppMsgManager::handleLauncherPushMessage(const QByteArray &data)
{
    quint8 transaction;
    QUuid uuid;
    WatchConnector::Dict dict;

    if (!unpackPushMessage(data, &transaction, &uuid, &dict)) {
        // Failed to parse!
        // Since we're the only one handling this endpoint,
        // all messages must be accepted
        logger()->warn() << "Failed to parser LAUNCHER PUSH message";
        return;
    }
    if (!dict.contains(1)) {
        logger()->warn() << "LAUNCHER message has no item in dict";
        return;
    }

    switch (dict.value(1).toInt()) {
    case WatchConnector::launcherSTARTED:
        logger()->debug() << "App starting in watch:" << uuid;
        this->watch->sendMessage(WatchConnector::watchLAUNCHER,
                                 buildAckMessage(transaction));
        emit appStarted(uuid);
        break;
    case WatchConnector::launcherSTOPPED:
        logger()->debug() << "App stopping in watch:" << uuid;
        this->watch->sendMessage(WatchConnector::watchLAUNCHER,
                                 buildAckMessage(transaction));
        emit appStopped(uuid);
        break;
    default:
        logger()->warn() << "LAUNCHER pushed unknown message:" << uuid << dict;
        this->watch->sendMessage(WatchConnector::watchLAUNCHER,
                                 buildNackMessage(transaction));
        break;
    }
}

void AppMsgManager::handlePushMessage(const QByteArray &data)
{
    quint8 transaction;
    QUuid uuid;
    WatchConnector::Dict dict;

    if (!unpackPushMessage(data, &transaction, &uuid, &dict)) {
        logger()->warn() << "Failed to parse APP_MSG PUSH";
        watch->sendMessage(WatchConnector::watchAPPLICATION_MESSAGE,
                           buildNackMessage(transaction));
        return;
    }

    logger()->debug() << "Received appmsg PUSH from" << uuid << "with" << dict;

    QVariantMap msg = mapAppKeys(uuid, dict);
    logger()->debug() << "Mapped dict" << msg;

    bool result;

    MessageHandlerFunc handler = _handlers.value(uuid);
    if (handler) {
        result = handler(msg);
    } else {
        // No handler? Let's just send an ACK.
        result = false;
    }

    if (result) {
        logger()->debug() << "ACKing transaction" << transaction;
        watch->sendMessage(WatchConnector::watchAPPLICATION_MESSAGE,
                           buildAckMessage(transaction));
    } else {
        logger()->info() << "NACKing transaction" << transaction;
        watch->sendMessage(WatchConnector::watchAPPLICATION_MESSAGE,
                           buildNackMessage(transaction));
    }
}

void AppMsgManager::handleAckMessage(const QByteArray &data, bool ack)
{
    if (data.size() < 2) {
        logger()->warn() << "invalid ack/nack message size";
        return;
    }

    const quint8 type = data[0];
    const quint8 recv_transaction = data[1];

    Q_ASSERT(type == WatchConnector::appmsgACK || type == WatchConnector::appmsgNACK);

    if (_pending.empty()) {
        logger()->warn() << "received an ack/nack for transaction" << recv_transaction << "but no transaction is pending";
        return;
    }

    PendingTransaction &trans = _pending.head();
    if (trans.transactionId != recv_transaction) {
        logger()->warn() << "received an ack/nack but for the wrong transaction";
    }

    logger()->debug() << "Got " << (ack ? "ACK" : "NACK") << " to transaction" << trans.transactionId;

    _timeout->stop();

    if (ack) {
        if (trans.ackCallback) {
            trans.ackCallback();
        }
    } else {
        if (trans.nackCallback) {
            trans.nackCallback();
        }
    }

    _pending.dequeue();

    if (!_pending.empty()) {
        transmitNextPendingTransaction();
    }
}

void AppMsgManager::handleWatchConnectedChanged()
{
    // If the watch is disconnected, everything breaks loose
    // TODO In the future we may want to avoid doing the following.
    if (!watch->isConnected()) {
        abortPendingTransactions();
    }
}

void AppMsgManager::handleTimeout()
{
    // Abort the first transaction
    Q_ASSERT(!_pending.empty());
    PendingTransaction trans = _pending.dequeue();

    logger()->warn() << "timeout on appmsg transaction" << trans.transactionId;

    if (trans.nackCallback) {
        trans.nackCallback();
    }

    if (!_pending.empty()) {
        transmitNextPendingTransaction();
    }
}

void AppMsgManager::transmitNextPendingTransaction()
{
    Q_ASSERT(!_pending.empty());
    PendingTransaction &trans = _pending.head();

    QByteArray msg = buildPushMessage(trans.transactionId, trans.uuid, trans.dict);

    watch->sendMessage(WatchConnector::watchAPPLICATION_MESSAGE, msg);

    _timeout->start();
}

void AppMsgManager::abortPendingTransactions()
{
    // Invoke all the NACK callbacks in the pending queue, then drop them.
    Q_FOREACH(const PendingTransaction &trans, _pending) {
        if (trans.nackCallback) {
            trans.nackCallback();
        }
    }

    _pending.clear();
}
