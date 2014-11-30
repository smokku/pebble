#include "appmsgmanager.h"
#include "unpacker.h"

// TODO D-Bus server for non JS kit apps!!!!

AppMsgManager::AppMsgManager(WatchConnector *watch, QObject *parent)
    : QObject(parent), watch(watch)
{
    watch->setEndpointHandler(WatchConnector::watchLAUNCHER,
                              [this](const QByteArray &data) {
        if (data.at(0) == WatchConnector::appmsgPUSH) {
            logger()->debug() << "LAUNCHER is PUSHing" << data.toHex();
            Unpacker u(data);
            u.skip(1); // skip data.at(0) which we just already checked above.
            uint transaction = u.read<quint8>();
            QUuid uuid = u.readUuid();
            WatchConnector::Dict dict = u.readDict();
            if (u.bad() || !dict.contains(1)) {
                logger()->warn() << "Failed to parse LAUNCHER message";
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
        case WatchConnector::appmsgPUSH:
            break;
        }

        return true;
    });
}

void AppMsgManager::send(const QUuid &uuid, const QVariantMap &data)
{
    // TODO
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
