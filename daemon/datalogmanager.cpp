#include "datalogmanager.h"
#include "unpacker.h"

DataLogManager::DataLogManager(WatchConnector *watch, QObject *parent) :
    QObject(parent), l(metaObject()->className()), watch(watch)
{
    watch->setEndpointHandler(WatchConnector::watchDATA_LOGGING, [this](const QByteArray& data) {
        if (data.size() < 2) {
            qCWarning(l) << "small data_logging packet";
            return false;
        }

        const char command = data[0];
        const int session = data[1];

        switch (command) {
        case WatchConnector::datalogOPEN:
            qCDebug(l) << "open datalog session" << session;
            return true;
        case WatchConnector::datalogCLOSE:
            qCDebug(l) << "close datalog session" << session;
            return true;
        case WatchConnector::datalogTIMEOUT:
            qCDebug(l) << "timeout datalog session" << session;
            return true;
        case WatchConnector::datalogDATA:
            handleDataCommand(session, data.mid(2));
            return true;
        default:
            return false;
        }
    });
}

void DataLogManager::handleDataCommand(int session, const QByteArray &data)
{
    Unpacker u(data);

    // TODO Seemingly related to analytics, so not important.

    qCDebug(l) << "got datalog data" << session << data.size();
}
