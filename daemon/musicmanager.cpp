#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include "musicmanager.h"
#include "settings.h"

MusicManager::MusicManager(WatchConnector *watch, Settings *settings, QObject *parent)
    : QObject(parent), l(metaObject()->className()),
      watch(watch), _watcher(new QDBusServiceWatcher(this)), _pulseBus(NULL), settings(settings),_maxVolume(0)
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

    // Locate and connect to the PulseAudio DBus to control system volume
    // Look up PulseAudio P2P Bus address
    QDBusMessage call = QDBusMessage::createMethodCall("org.PulseAudio1", "/org/pulseaudio/server_lookup1", "org.freedesktop.DBus.Properties", "Get" );
    call << "org.PulseAudio.ServerLookup1" << "Address";
    QDBusReply<QDBusVariant> lookupReply = bus.call(call);
    if (lookupReply.isValid()) {
        //
        qCDebug(l) << "PulseAudio Bus address: " << lookupReply.value().variant().toString();
        _pulseBus = new QDBusConnection(QDBusConnection::connectToPeer(lookupReply.value().variant().toString(), "org.PulseAudio1"));
    }
    else {
        qCDebug(l) << "Cannot find PulseAudio Bus address, cannot control system volume.";
    }

    // Query max volume
    call = QDBusMessage::createMethodCall("com.Meego.MainVolume2", "/com/meego/mainvolume2",
                                                       "org.freedesktop.DBus.Properties", "Get");
    call << "com.Meego.MainVolume2" << "StepCount";
    QDBusReply<QDBusVariant> volumeMaxReply = _pulseBus->call(call);
    if (volumeMaxReply.isValid()) {
        _maxVolume = volumeMaxReply.value().variant().toUInt();
        qCDebug(l) << "Max volume: " << _maxVolume;
    }
    else {
        qCWarning(l) << "Could not read volume max, cannot adjust volume: " << volumeMaxReply.error().message();
    }

    // Now set up the Pebble endpoint handler for music control commands
    watch->setEndpointHandler(WatchConnector::watchMUSIC_CONTROL, [this](const QByteArray& data) {
        handleMusicControl(WatchConnector::MusicControl(data.at(0)));
        return true;
    });

    // If the watch disconnects, we will send the current metadata when it comes back.
    connect(watch, &WatchConnector::connectedChanged,
            this, &MusicManager::handleWatchConnected);
}

MusicManager::~MusicManager()
{
    if (_pulseBus != NULL) {
        qCDebug(l) << "Disconnecting from PulseAudio P2P DBus";
        QDBusConnection::disconnectFromBus("org.PulseAudio1");
        delete(_pulseBus);
    }
}

void MusicManager::switchToService(const QString &service)
{
    if (_curService != service) {
        qCDebug(l) << "switching to mpris service" << service;
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
            qCDebug(l) << "got mpris metadata from service" << _curService;
            _curMetadata = qdbus_cast<QVariantMap>(reply.value().variant().value<QDBusArgument>());
        } else {
            qCWarning(l) << reply.error().message();
        }
    }
}

void MusicManager::sendCurrentMprisMetadata()
{
    Q_ASSERT(watch->isConnected());

    QString artist = _curMetadata.value("xesam:artist").toString().left(30);
    QString album = _curMetadata.value("xesam:album").toString().left(30);
    QString track = _curMetadata.value("xesam:title").toString().left(30);

    qCDebug(l) << "sending mpris metadata:" << artist << album << track;

    watch->sendMusicNowPlaying(artist, album, track);
}

void MusicManager::callMprisMethod(const QString &method)
{
    Q_ASSERT(!method.isEmpty());
    Q_ASSERT(!_curService.isEmpty());

    qCDebug(l) << _curService << "->" << method;

    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusMessage call = QDBusMessage::createMethodCall(_curService,
                                                       "/org/mpris/MediaPlayer2",
                                                       "org.mpris.MediaPlayer2.Player",
                                                       method);

    QDBusError err = bus.call(call);

    if (err.isValid()) {
        qCWarning(l) << "while calling mpris method on" << _curService << ":" << err.message();
    }
}

void MusicManager::handleMusicControl(WatchConnector::MusicControl operation)
{
    qCDebug(l) << "operation from watch:" << operation;
    QVariant useSystemVolumeVar = settings->property("useSystemVolume");
    bool useSystemVolume = (useSystemVolumeVar.isValid() && useSystemVolumeVar.toBool());

    // System volume controls
    if (useSystemVolume && _pulseBus != NULL &&
            (operation == WatchConnector::musicVOLUME_UP || operation == WatchConnector::musicVOLUME_DOWN)) {
        // Query current volume
        QDBusMessage call = QDBusMessage::createMethodCall("com.Meego.MainVolume2", "/com/meego/mainvolume2",
                                                       "org.freedesktop.DBus.Properties", "Get");
        call << "com.Meego.MainVolume2" << "CurrentStep";

        QDBusReply<QDBusVariant> volumeReply = _pulseBus->call(call);
        if (volumeReply.isValid()) {
            // Decide the new value for volume, taking limits into account
            uint volume = volumeReply.value().variant().toUInt();
            uint newVolume;
            qCDebug(l) << "Current volume: " << volumeReply.value().variant().toUInt();
            if (operation == WatchConnector::musicVOLUME_UP && volume < _maxVolume-1 ) {
                newVolume = volume + 1;
            }
            else if (operation == WatchConnector::musicVOLUME_DOWN && volume > 0) {
                newVolume = volume - 1;
            }
            else {
            qCDebug(l) << "Volume already at limit";
            newVolume = volume;
            }

            // If we have a new volume level, change it
            if (newVolume != volume) {
                qCDebug(l) << "Setting volume: " << newVolume;

                call = QDBusMessage::createMethodCall("com.Meego.MainVolume2", "/com/meego/mainvolume2",
                                                      "org.freedesktop.DBus.Properties", "Set");
                call << "com.Meego.MainVolume2" << "CurrentStep" << QVariant::fromValue(QDBusVariant(newVolume));

                QDBusError err = _pulseBus->call(call);
                if (err.isValid()) {
                    qCWarning(l) << err.message();
                }
            }
        }
    }

    //Don't allow any music operations if there's no MPRIS player, unless we are allowed to control the system volume and it's a volume command
    else if (_curService.isEmpty()) {
        qCDebug(l) << "can't do any music operation, no mpris interface active";
        return;
    }
    else {
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
                qCDebug(l) << "Setting volume" << volume;

                call = QDBusMessage::createMethodCall(_curService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Set");
                call << "org.mpris.MediaPlayer2.Player" << "Volume" << QVariant::fromValue(QDBusVariant(volume));

                QDBusError err = QDBusConnection::sessionBus().call(call);
                if (err.isValid()) {
                    qCWarning(l) << err.message();
                }
             } else {
             qCWarning(l) << volumeReply.error().message();
            }
            break;
        }

        case WatchConnector::musicGET_NOW_PLAYING:
            sendCurrentMprisMetadata();
            break;

        default:
            qCWarning(l) << "Operation" << operation << "not supported";
            break;
        }
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
        qCDebug(l) << "received new metadata" << metadata;
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
