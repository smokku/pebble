#include <QtDBus>
#include "musicmanager.h"

MusicManager::MusicManager(WatchConnector *watch, QObject *parent)
    : QObject(parent), watch(watch)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusConnectionInterface *bus_iface = bus.interface();

    // Listen for MPRIS signals from every player
    bus.connect("", "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged",
                this, SLOT(handleMprisPropertiesChanged(QString,QMap<QString,QVariant>,QStringList)));

    // Listen for D-Bus name registered signals to see if a MPRIS service comes up
    connect(bus_iface, &QDBusConnectionInterface::serviceRegistered,
            this, &MusicManager::handleServiceRegistered);
    connect(bus_iface, &QDBusConnectionInterface::serviceUnregistered,
            this, &MusicManager::handleServiceUnregistered);
    connect(bus_iface, &QDBusConnectionInterface::serviceOwnerChanged,
            this, &MusicManager::handleServiceOwnerChanged);

    // But also try to find an already active MPRIS service
    const QStringList &services = bus_iface->registeredServiceNames();
    foreach (QString service, services) {
        if (service.startsWith("org.mpris.MediaPlayer2.")) {
            switchToService(service);
            break;
        }
    }

    // Set up watch endpoint handler
    watch->setEndpointHandler(WatchConnector::watchMUSIC_CONTROL, [this](const QByteArray& data) {
        musicControl(WatchConnector::MusicControl(data.at(0)));
        return true;
    });
    connect(watch, &WatchConnector::connectedChanged,
            this, &MusicManager::handleWatchConnected);
}

void MusicManager::musicControl(WatchConnector::MusicControl operation)
{
    logger()->debug() << "operation from watch:" << operation;

    if (_curService.isEmpty()) {
        logger()->info() << "No mpris interface active";
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
        QDBusConnection bus = QDBusConnection::sessionBus();
        QDBusMessage call = QDBusMessage::createMethodCall(_curService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
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
        return;
    case WatchConnector::musicGET_NOW_PLAYING:
        setMprisMetadata(_curMetadata);
        return;

    case WatchConnector::musicSEND_NOW_PLAYING:
    default:
        logger()->warn() << "Operation" << operation << "not supported";
        return;
    }

    if (method.isEmpty()) {
        logger()->error() << "Requested unsupported operation" << operation;
        return;
    }

    logger()->debug() << operation << "->" << method;

    QDBusMessage call = QDBusMessage::createMethodCall(_curService, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", method);
    QDBusError err = QDBusConnection::sessionBus().call(call);
    if (err.isValid()) {
        logger()->error() << err.message();
    }
}

void MusicManager::switchToService(const QString &service)
{
    if (_curService != service) {
        logger()->debug() << "switching to mpris service" << service;
        _curService = service;
    }
}

void MusicManager::setMprisMetadata(const QVariantMap &metadata)
{
    _curMetadata = metadata;
    QString track = metadata.value("xesam:title").toString();
    QString album = metadata.value("xesam:album").toString();
    QString artist = metadata.value("xesam:artist").toString();
    logger()->debug() << "new mpris metadata:" << track << album << artist;

    if (watch->isConnected()) {
        watch->sendMusicNowPlaying(track, album, artist);
    }
}

void MusicManager::handleServiceRegistered(const QString &service)
{
    if (service.startsWith("org.mpris.MediaPlayer2.")) {
        if (_curService.isEmpty()) {
            switchToService(service);
        }
    }
}

void MusicManager::handleServiceUnregistered(const QString &service)
{
    if (service == _curService) {
        // Oops!
        setMprisMetadata(QVariantMap());
        switchToService(QString());
    }
}

void MusicManager::handleServiceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner);
    if (newOwner.isEmpty()) {
        handleServiceUnregistered(name);
    } else {
        handleServiceRegistered(name);
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
        setMprisMetadata(metadata);
    }

    if (changed.contains("PlaybackStatus")) {
        QString status = changed.value("PlaybackStatus").toString();
        if (status == "Stopped") {
            setMprisMetadata(QVariantMap());
        }
    }

    switchToService(message().service());
}

void MusicManager::handleWatchConnected()
{
    if (watch->isConnected()) {
        if (!_curService.isEmpty()) {
            QDBusMessage call = QDBusMessage::createMethodCall(_curService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
            call << "org.mpris.MediaPlayer2.Player" << "Metadata";
            QDBusReply<QDBusVariant> metadata = QDBusConnection::sessionBus().call(call);
            if (metadata.isValid()) {
                setMprisMetadata(qdbus_cast<QVariantMap>(metadata.value().variant().value<QDBusArgument>()));
                //
            } else {
                logger()->error() << metadata.error().message();
                setMprisMetadata(QVariantMap());
            }
        }
    }
}
