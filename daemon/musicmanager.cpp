#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include "musicmanager.h"

MusicManager::MusicManager(WatchConnector *watch, QObject *parent)
    : QObject(parent), watch(watch), _watcher(new QDBusServiceWatcher(this))
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusConnectionInterface *bus_iface = bus.interface();

    // This watcher will be used to find when the current MPRIS service dies
    // (and thus we must clear the metadata)
    _watcher->setConnection(bus);
    connect(_watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, &MusicManager::handleMprisServiceOwnerChanged);

    // Try to find an active MPRIS service to initially connect to
    const QStringList &services = bus_iface->registeredServiceNames();
    foreach (QString service, services) {
        if (service.startsWith("org.mpris.MediaPlayer2.")) {
            switchToService(service);
            fetchMetadataFromService();
            // The watch is not connected by this point,
            // so we don't send the current metadata.
            break;
        }
    }

    // Even if we didn't find any service, we still listen for metadataChanged signals
    // from every MPRIS-compatible player
    // If such a signal comes in, we will connect to the source service for that signal
    bus.connect("", "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged",
                this, SLOT(handleMprisPropertiesChanged(QString,QMap<QString,QVariant>,QStringList)));

    // Now set up the Pebble endpoint handler for music control commands
    watch->setEndpointHandler(WatchConnector::watchMUSIC_CONTROL, [this](const QByteArray& data) {
        handleMusicControl(WatchConnector::MusicControl(data.at(0)));
        return true;
    });

    // If the watch disconnects, we will send the current metadata when it comes back.
    connect(watch, &WatchConnector::connectedChanged,
            this, &MusicManager::handleWatchConnected);
}

void MusicManager::switchToService(const QString &service)
{
    if (_curService != service) {
        logger()->debug() << "switching to mpris service" << service;
        _curService = service;

        if (_curService.isEmpty()) {
            _watcher->setWatchedServices(QStringList());
        } else {
            _watcher->setWatchedServices(QStringList(_curService));
        }
    }
}

void MusicManager::fetchMetadataFromService()
{
    _curMetadata.clear();

    if (!_curService.isEmpty()) {
        QDBusMessage call = QDBusMessage::createMethodCall(_curService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
        call << "org.mpris.MediaPlayer2.Player" << "Metadata";
        QDBusReply<QDBusVariant> reply = QDBusConnection::sessionBus().call(call);
        if (reply.isValid()) {
            logger()->debug() << "got mpris metadata from service" << _curService;
            _curMetadata = qdbus_cast<QVariantMap>(reply.value().variant().value<QDBusArgument>());
        } else {
            logger()->error() << reply.error().message();
        }
    }
}

void MusicManager::sendCurrentMprisMetadata()
{
    Q_ASSERT(watch->isConnected());

    QString track = _curMetadata.value("xesam:title").toString().left(30);
    QString album = _curMetadata.value("xesam:album").toString().left(30);
    QString artist = _curMetadata.value("xesam:artist").toString().left(30);

    logger()->debug() << "sending mpris metadata:" << track << album << artist;

    watch->sendMusicNowPlaying(track, album, artist);
}

void MusicManager::callMprisMethod(const QString &method)
{
    Q_ASSERT(!method.isEmpty());
    Q_ASSERT(!_curService.isEmpty());

    logger()->debug() << _curService << "->" << method;

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusMessage call = QDBusMessage::createMethodCall(_curService,
                                                       "/org/mpris/MediaPlayer2",
                                                       "org.mpris.MediaPlayer2.Player",
                                                       method);

    QDBusError err = bus.call(call);

    if (err.isValid()) {
        logger()->error() << "while calling mpris method on" << _curService << ":" << err.message();
    }
}

void MusicManager::handleMusicControl(WatchConnector::MusicControl operation)
{
    logger()->debug() << "operation from watch:" << operation;

    if (_curService.isEmpty()) {
        logger()->info() << "can't do any music operation, no mpris interface active";
        return;
    }

    switch (operation) {
    case WatchConnector::musicPLAY_PAUSE:
        callMprisMethod("PlayPause");
        break;
    case WatchConnector::musicPAUSE:
        callMprisMethod("Pause");
        break;
    case WatchConnector::musicPLAY:
        callMprisMethod("Play");
        break;
    case WatchConnector::musicNEXT:
        callMprisMethod("Next");
        break;
    case WatchConnector::musicPREVIOUS:
        callMprisMethod("Previous");
        break;

    case WatchConnector::musicVOLUME_UP:
    case WatchConnector::musicVOLUME_DOWN: {
        QDBusConnection bus = QDBusConnection::sessionBus();
        QDBusMessage call = QDBusMessage::createMethodCall(_curService, "/org/mpris/MediaPlayer2",
                                                           "org.freedesktop.DBus.Properties", "Get");
        call << "org.mpris.MediaPlayer2.Player" << "Volume";
        QDBusReply<QDBusVariant> volumeReply = bus.call(call);
        if (volumeReply.isValid()) {
            double volume = volumeReply.value().variant().toDouble();
            if (operation == WatchConnector::musicVOLUME_UP) {
                volume += 0.1;
            }
            else {
                volume -= 0.1;
            }
            logger()->debug() << "Setting volume" << volume;

            call = QDBusMessage::createMethodCall(_curService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Set");
            call << "org.mpris.MediaPlayer2.Player" << "Volume" << QVariant::fromValue(QDBusVariant(volume));

            QDBusError err = QDBusConnection::sessionBus().call(call);
            if (err.isValid()) {
                logger()->error() << err.message();
            }
        } else {
            logger()->error() << volumeReply.error().message();
        }
        }
        break;

    case WatchConnector::musicGET_NOW_PLAYING:
        sendCurrentMprisMetadata();
        break;

    default:
        logger()->warn() << "Operation" << operation << "not supported";
        break;
    }
}

void MusicManager::handleMprisServiceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner);
    if (name == _curService && newOwner.isEmpty()) {
        // Oops, current service is going away
        switchToService(QString());
        _curMetadata.clear();
        if (watch->isConnected()) {
            sendCurrentMprisMetadata();
        }
    }
}

void MusicManager::handleMprisPropertiesChanged(const QString &interface, const QMap<QString, QVariant> &changed, const QStringList &invalidated)
{
    Q_ASSERT(calledFromDBus());
    Q_UNUSED(interface);
    Q_UNUSED(invalidated);

    if (changed.contains("Metadata")) {
        QVariantMap metadata = qdbus_cast<QVariantMap>(changed.value("Metadata").value<QDBusArgument>());
        logger()->debug() << "received new metadata" << metadata;
        _curMetadata = metadata;
    }

    if (changed.contains("PlaybackStatus")) {
        QString status = changed.value("PlaybackStatus").toString();
        if (status == "Stopped") {
            _curMetadata.clear();
        }
    }

    if (watch->isConnected()) {
        sendCurrentMprisMetadata();
    }

    switchToService(message().service());
}

void MusicManager::handleWatchConnected()
{
    if (watch->isConnected()) {
        sendCurrentMprisMetadata();
    }
}
