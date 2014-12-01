#include "appmsgmanager.h"
#include "unpacker.h"
#include "packer.h"

// TODO D-Bus server for non JS kit apps!!!!

AppMsgManager::AppMsgManager(AppManager *apps, WatchConnector *watch, QObject *parent)
    : QObject(parent), apps(apps), watch(watch), lastTransactionId(0)
{
    watch->setEndpointHandler(WatchConnector::watchLAUNCHER,
                              [this](const QByteArray &data) {
        if (data.at(0) == WatchConnector::appmsgPUSH) {
            uint transaction;
            QUuid uuid;
            WatchConnector::Dict dict;

            if (!unpackPushMessage(data, &transaction, &uuid, &dict)) {
                // Failed to parse!
                // Since we're the only one handling this endpoint,
                // all messages must be accepted
                logger()->warn() << "Failed to parser LAUNCHER PUSH message";
                return true;
            }
            if (!dict.contains(1)) {
                logger()->warn() << "LAUNCHER message has no item in dict";
                return true;
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

        return true;
    });

    watch->setEndpointHandler(WatchConnector::watchAPPLICATION_MESSAGE,
                              [this](const QByteArray &data) {
        switch (data.at(0)) {
        case WatchConnector::appmsgPUSH: {
            uint transaction;
            QUuid uuid;
            WatchConnector::Dict dict;

            if (!unpackPushMessage(data, &transaction, &uuid, &dict)) {
                logger()->warn() << "Failed to parse APP_MSG PUSH";
                return true;
            }

            logger()->debug() << "Received appmsg PUSH from" << uuid << "with" << dict;

            QVariantMap data = mapAppKeys(uuid, dict);
            logger()->debug() << "Mapped dict" << data;

            emit messageReceived(uuid, data);
            break;
        }
        default:
            logger()->warn() << "Unknown application message type:" << data.at(0);
            break;
        }

        return true;
    });
}

void AppMsgManager::send(const QUuid &uuid, const QVariantMap &data, const std::function<void ()> &ackCallback, const std::function<void ()> &nackCallback)
{
    WatchConnector::Dict dict = mapAppKeys(uuid, data);
    quint8 transaction = ++lastTransactionId;
    QByteArray msg = buildPushMessage(transaction, uuid, dict);

    logger()->debug() << "Sending appmsg" << transaction << "to" << uuid << "with" << dict;

    WatchConnector::Dict t_dict;
    QUuid t_uuid;
    uint t_trans;
    if (unpackPushMessage(msg, &t_trans, &t_uuid, &t_dict)) {
        logger()->debug() << t_trans << t_uuid << t_dict;
    } else {
        logger()->warn() << "not unpack my own";
    }


    watch->sendMessage(WatchConnector::watchAPPLICATION_MESSAGE, msg,
                       [this, ackCallback, nackCallback, transaction](const QByteArray &reply) {
        if (reply.size() < 2) return false;

        quint8 type = reply[0];
        quint8 recv_transaction = reply[1];

        logger()->debug() << "Got response to transaction" << transaction;

        if (recv_transaction != transaction) return false;

        switch (type) {
        case WatchConnector::appmsgACK:
            logger()->debug() << "Got ACK to transaction" << transaction;
            if (ackCallback) ackCallback();
            return true;
        case WatchConnector::appmsgNACK:
            logger()->info() << "Got NACK to transaction" << transaction;
            if (nackCallback) nackCallback();
            return true;
        default:
            return false;
        }
    });
}

void AppMsgManager::send(const QUuid &uuid, const QVariantMap &data)
{
    std::function<void()> nullCallback;
    send(uuid, data, nullCallback, nullCallback);
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

bool AppMsgManager::unpackPushMessage(const QByteArray &msg, uint *transaction, QUuid *uuid, WatchConnector::Dict *dict)
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

QByteArray AppMsgManager::buildPushMessage(uint transaction, const QUuid &uuid, const WatchConnector::Dict &dict)
{
    QByteArray ba;
    Packer p(&ba);
    p.write<quint8>(WatchConnector::appmsgPUSH);
    p.write<quint8>(transaction);
    p.writeUuid(uuid);
    p.writeDict(dict);

    return ba;
}

QByteArray AppMsgManager::buildAckMessage(uint transaction)
{
    QByteArray ba(2, Qt::Uninitialized);
    ba[0] = WatchConnector::appmsgACK;
    ba[1] = transaction;
    return ba;
}

QByteArray AppMsgManager::buildNackMessage(uint transaction)
{
    QByteArray ba(2, Qt::Uninitialized);
    ba[0] = WatchConnector::appmsgNACK;
    ba[1] = transaction;
    return ba;
}
