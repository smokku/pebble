#include "watchcommands.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

using namespace watch;

WatchCommands::WatchCommands(WatchConnector *watch, QObject *parent) :
    QObject(parent), watch(watch)
{}

void WatchCommands::processMessage(uint endpoint, QByteArray data)
{
    logger()->debug() << __FUNCTION__ << endpoint << "/" << data.toHex() << data.length();
    switch (endpoint) {
    case WatchConnector::watchPHONE_VERSION:
        watch->sendPhoneVersion();
        break;
    case WatchConnector::watchPHONE_CONTROL:
        if (data.at(0) == WatchConnector::callHANGUP) {
            emit hangup();
        }
        break;
    case WatchConnector::watchMUSIC_CONTROL:
        musicControl(WatchConnector::MusicControl(data.at(0)));
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

void WatchCommands::musicControl(WatchConnector::MusicControl operation)
{
    logger()->debug() << "Operation:" << operation;

    QString mpris = parent()->property("mpris").toString();
    if (mpris.isEmpty()) {
        logger()->debug() << "No mpris interface active";
        return;
    }

    QString method;

    switch(operation) {
    case WatchConnector::musicPLAY_PAUSE:
        method = "PlayPause";
        break;
    case WatchConnector::musicPAUSE:
        method = "Pause";
        break;
    case WatchConnector::musicPLAY:
        method = "Play";
        break;
    case WatchConnector::musicNEXT:
        method = "Next";
        break;
    case WatchConnector::musicPREVIOUS:
        method = "Previous";
        break;
    case WatchConnector::musicVOLUME_UP:
    case WatchConnector::musicVOLUME_DOWN: {
            QDBusReply<QDBusVariant> VolumeReply = QDBusConnection::sessionBus().call(
                        QDBusMessage::createMethodCall(mpris, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get")
                        << "org.mpris.MediaPlayer2.Player" << "Volume");
            if (VolumeReply.isValid()) {
                double volume = VolumeReply.value().variant().toDouble();
                if (operation == WatchConnector::musicVOLUME_UP) {
                    volume += 0.1;
                }
                else {
                    volume -= 0.1;
                }
                logger()->debug() << "Setting volume" << volume;
                QDBusError err = QDBusConnection::sessionBus().call(
                            QDBusMessage::createMethodCall(mpris, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Set")
                            << "org.mpris.MediaPlayer2.Player" << "Volume" << QVariant::fromValue(QDBusVariant(volume)));
                if (err.isValid()) {
                    logger()->error() << err.message();
                }
            }
            else {
                logger()->error() << VolumeReply.error().message();
            }
        }
        return;
    case WatchConnector::musicGET_NOW_PLAYING:
        onMprisMetadataChanged(parent()->property("mprisMetadata").toMap());
        return;

    case WatchConnector::musicSEND_NOW_PLAYING:
        logger()->warn() << "Operation" << operation << "not supported";
        return;
    }

    if (method.isEmpty()) {
        logger()->error() << "Requested unsupported operation" << operation;
        return;
    }

    logger()->debug() << operation << "->" << method;

    QDBusError err = QDBusConnection::sessionBus().call(
                QDBusMessage::createMethodCall(mpris, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", method));
    if (err.isValid()) {
        logger()->error() << err.message();
    }
}
