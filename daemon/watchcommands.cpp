#include "watchcommands.h"

using namespace watch;

WatchCommands::WatchCommands(WatchConnector *watch, QObject *parent) :
    QObject(parent), watch(watch)
{}

void WatchCommands::processMessage(uint endpoint, uint datalen, QByteArray data)
{
    logger()->debug() << __FUNCTION__ << endpoint << "/" << data.length();
    switch (endpoint) {
    case WatchConnector::watchPHONE_VERSION:
        watch->sendPhoneVersion();
        break;
    case WatchConnector::watchPHONE_CONTROL:
        if (data.length() >= 5 && data.at(4) == WatchConnector::callHANGUP) {
            emit hangup();
        }
        break;
    case WatchConnector::watchMUSIC_CONTROL:
        logger()->debug() << "MUSIC_CONTROL" << data.toHex();
        break;

    default:
        logger()->info() << __FUNCTION__ << "endpoint" << endpoint << "not supported yet";
    }
}

void WatchCommands::onMprisMetadataChanged(QVariantMap metadata)
{
    QString track = metadata.value("xesam:title").toString();
    QString album = metadata.value("xesam:album").toString();
    QString artist = metadata.value("xesam:artist").toString();
    logger()->debug() << __FUNCTION__ << track << album << artist;
    watch->sendMusicNowPlaying(track, album, artist);
}
