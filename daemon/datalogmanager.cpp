#include "datalogmanager.h"
#include "unpacker.h"

DataLogManager::DataLogManager(WatchConnector *watch, QObject *parent) :
    QObject(parent), watch(watch)
{
    watch->setEndpointHandler(WatchConnector::watchDATA_LOGGING, [this](const QByteArray& data) {
        if (data.size() < 2) {
            logger()->warn() << "small data_logging packet";
            return false;
        }

        const char command = data[0];
        const int session = data[1];

        switch (command) {
        case WatchConnector::datalogOPEN:
            logger()->debug() << "open datalog session" << session;
            return true;
        case WatchConnector::datalogCLOSE:
            logger()->debug() << "close datalog session" << session;
            return true;
        case WatchConnector::datalogTIMEOUT:
            logger()->debug() << "timeout datalog session" << session;
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

    logger()->debug() << "got datalog data" << session << data.size();
}
