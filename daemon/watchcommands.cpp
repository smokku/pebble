#include "watchcommands.h"

using namespace watch;

WatchCommands::WatchCommands(WatchConnector *watch, QObject *parent) :
    QObject(parent), watch(watch)
{
    connect(watch, SIGNAL(messageDecoded(uint,uint,QByteArray)), SLOT(processMessage(uint,uint,QByteArray)));
}

void WatchCommands::processMessage(uint endpoint, uint datalen, QByteArray data)
{
    if (endpoint == WatchConnector::watchPHONE_CONTROL) {
        if (data.length() >= 5) {
            if (data.at(4) == WatchConnector::callHANGUP) {
                emit hangup();
            }
        }
    } else if (endpoint == WatchConnector::watchPHONE_VERSION) {
        watch->sendPhoneVersion();
    }
}
