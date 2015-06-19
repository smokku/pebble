#include <QDateTime>
#include <QMetaEnum>
#include <QDebugStateSaver>
#include <QBluetoothLocalDevice>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDBusObjectPath>

#include "unpacker.h"
#include "watchconnector.h"

static const int RECONNECT_TIMEOUT = 500; //ms
static const bool PROTOCOL_DEBUG = false;

QDebug operator<< (QDebug d, const WatchConnector::SoftwareVersion &ver) {
    QDebugStateSaver save(d);
    d.setAutoInsertSpaces(true);
    d << ver.version << ver.build.toString(Qt::ISODate) << ver.commit;
    d.nospace() << ver.hw_string << "(" << ver.hw_revision << ")";
    d.space() << ver.metadata_version;
    d.nospace() << (ver.is_recovery ? "recovery" : "standard");
    return d;
}

QDebug operator<< (QDebug d, const WatchConnector::WatchVersions &ver) {
    QDebugStateSaver save(d);
    d.nospace() << "bootloader: " << ver.bootLoaderBuild.toString(Qt::ISODate)
                << ", serial number: " << ver.serialNumber
                << ", bt address: " << ver.address.toHex()
                << ", firmware (main: " << ver.main
                << ") (safe: " << ver.safe << ")";
    return d;
}

QVariantMap WatchConnector::SoftwareVersion::toMap() const
{
    QVariantMap map;
    map.insert("version", this->version);
    map.insert("build", this->build.toTime_t());
    map.insert("commit", this->commit);
    map.insert("hardware", this->hw_string);
    map.insert("metadata", this->metadata_version);
    map.insert("recovery", this->is_recovery);
    return map;
}

QVariantMap WatchConnector::WatchVersions::toMap() const
{
    QVariantMap map;
    if (!isEmpty()) {
        map.insert("bootloader", this->bootLoaderBuild.toTime_t());
        map.insert("serial", this->serialNumber);
        map.insert("address", this->address.toHex());
        map.insertMulti("firmware", this->main.toMap());
        map.insertMulti("firmware", this->safe.toMap());
    }
    return map;
}
void WatchConnector::WatchVersions::clear()
{
    serialNumber.clear();
    address.clear();
}

bool WatchConnector::WatchVersions::isEmpty() const
{
    return serialNumber.isEmpty() || address.isEmpty();
}

WatchConnector::WatchConnector(QObject *parent) :
    QObject(parent), l(metaObject()->className()),
    socket(nullptr), is_connected(false),
    currentPebble(0), _last_address(0), platform(HP_UNKNOWN)
{
    reconnectTimer.setSingleShot(true);
    QObject::connect(&reconnectTimer, SIGNAL(timeout()), SLOT(connect()));
    timeSyncTimer.setSingleShot(true);
    QObject::connect(&timeSyncTimer, SIGNAL(timeout()), SLOT(time()));
    timeSyncTimer.setInterval(4 * 60 * 60 * 1000); // sync time every 4 hours

    hardwareMapping.insert(HR_UNKNOWN, HWMap(HP_UNKNOWN, "unknown"));
    hardwareMapping.insert(TINTIN_EV1, HWMap(APLITE, "ev1"));
    hardwareMapping.insert(TINTIN_EV2, HWMap(APLITE, "ev2"));
    hardwareMapping.insert(TINTIN_EV2_3, HWMap(APLITE, "ev2_3"));
    hardwareMapping.insert(TINTIN_EV2_4, HWMap(APLITE, "ev2_4"));
    hardwareMapping.insert(TINTIN_V1_5, HWMap(APLITE, "v1_5"));
    hardwareMapping.insert(BIANCA, HWMap(APLITE, "v2_0"));
    hardwareMapping.insert(SNOWY_EVT2, HWMap(BASALT, "snowy_evt2"));
    hardwareMapping.insert(SNOWY_DVT, HWMap(BASALT, "snowy_dvt"));
    hardwareMapping.insert(TINTIN_BB, HWMap(APLITE, "bigboard"));
    hardwareMapping.insert(TINTIN_BB2, HWMap(APLITE, "bb2"));
    hardwareMapping.insert(SNOWY_BB, HWMap(BASALT, "snowy_bb"));
    hardwareMapping.insert(SNOWY_BB2, HWMap(BASALT, "snowy_bb2"));

    setEndpointHandler(watchVERSION, [this](const QByteArray &data) {
        Unpacker u(data);

        u.skip(1);

        _versions.main.build = QDateTime::fromTime_t(u.read<quint32>());
        _versions.main.version = u.readFixedString(32);
        _versions.main.commit = u.readFixedString(8);
        _versions.main.is_recovery = u.read<quint8>();
        _versions.main.hw_revision = HardwareRevision(u.read<quint8>());
        _versions.main.hw_string = hardwareMapping.value(_versions.main.hw_revision).second;
        _versions.main.metadata_version = u.read<quint8>();

        _versions.safe.build = QDateTime::fromTime_t(u.read<quint32>());
        _versions.safe.version = u.readFixedString(32);
        _versions.safe.commit = u.readFixedString(8);
        _versions.safe.is_recovery = u.read<quint8>();
        _versions.safe.hw_revision = HardwareRevision(u.read<quint8>());
        _versions.safe.hw_string = hardwareMapping.value(_versions.safe.hw_revision).second;
        _versions.safe.metadata_version = u.read<quint8>();

        _versions.bootLoaderBuild = QDateTime::fromTime_t(u.read<quint32>());
        _versions.hardwareRevision = u.readFixedString(9);
        _versions.serialNumber = u.readFixedString(12);
        _versions.address = u.readBytes(6);

        platform = hardwareMapping.value(_versions.safe.hw_revision).first;

        if (u.bad()) {
            qCWarning(l) << "short read while reading firmware version";
        } else {
            emit versionsChanged();
        }

        qCDebug(l) << "hardware information:" << _versions;

        return true;
    });
}

WatchConnector::~WatchConnector()
{
}

//dbus-send --system --dest=org.bluez --print-reply / org.bluez.Manager.ListAdapters
//dbus-send --system --dest=org.bluez --print-reply $path org.bluez.Adapter.GetProperties
//dbus-send --system --dest=org.bluez --print-reply $devpath org.bluez.Device.GetProperties
//dbus-send --system --dest=org.bluez --print-reply $devpath org.bluez.Input.Connect
bool WatchConnector::findPebbles()
{
    pebbles.clear();
    currentPebble = 0;

    QDBusConnection system = QDBusConnection::systemBus();

    QDBusReply<QList<QDBusObjectPath>> ListAdaptersReply = system.call(
                QDBusMessage::createMethodCall("org.bluez", "/", "org.bluez.Manager",
                                               "ListAdapters"));
    if (not ListAdaptersReply.isValid()) {
        qCCritical(l) << ListAdaptersReply.error().message();
        return false;
    }

    QList<QDBusObjectPath> adapters = ListAdaptersReply.value();

    if (adapters.isEmpty()) {
        qCDebug(l) << "No BT adapters found";
        return false;
    }

    QDBusReply<QVariantMap> AdapterPropertiesReply = system.call(
                QDBusMessage::createMethodCall("org.bluez", adapters[0].path(), "org.bluez.Adapter",
                                               "GetProperties"));
    if (not AdapterPropertiesReply.isValid()) {
        qCCritical(l) << AdapterPropertiesReply.error().message();
        return false;
    }

    QList<QDBusObjectPath> devices;
    AdapterPropertiesReply.value()["Devices"].value<QDBusArgument>() >> devices;

    foreach (QDBusObjectPath path, devices) {
        QDBusReply<QVariantMap> DevicePropertiesReply = system.call(
                    QDBusMessage::createMethodCall("org.bluez", path.path(), "org.bluez.Device",
                                                   "GetProperties"));
        if (not DevicePropertiesReply.isValid()) {
            qCCritical(l) << DevicePropertiesReply.error().message();
            continue;
        }

        const QVariantMap &dict = DevicePropertiesReply.value();

        QString tmp = dict["Name"].toString();
        qCDebug(l) << "Found BT device:" << tmp;
        if (tmp.startsWith("Pebble")) {
            qCDebug(l) << "Found Pebble:" << tmp;
            QBluetoothAddress addr(dict["Address"].toString());
            QBluetoothLocalDevice dev;
            if (dev.pairingStatus(addr) == QBluetoothLocalDevice::AuthorizedPaired) {
                pebbles.insert(dict["Name"].toString(), addr);
            }
        }
    }

    if (pebbles.size()) {
        scheduleReconnect();
    }

    return true;
}

void WatchConnector::disconnect()
{
    qCDebug(l) << "disconnecting";
    socket->close();
    socket->deleteLater();
    reconnectTimer.stop();
    timeSyncTimer.stop();
    qCDebug(l) << "stopped timers";
}

void WatchConnector::connect()
{
    if (currentPebble >= pebbles.count()) {
        currentPebble = 0;
    }

    QString _name = pebbles.keys().at(currentPebble);
    QBluetoothAddress _address = pebbles.value(_name);

    qCDebug(l) << "connect watch" << currentPebble << _name << _address.toString();
    reconnectTimer.stop();

    if (_address.isNull()) {
        qCWarning(l) << "No known pebble";
        return;
    }

    // Check if bluetooth is on
    QBluetoothLocalDevice host;
    bool btOff = host.hostMode() == QBluetoothLocalDevice::HostPoweredOff;
    if (btOff) {
        qCDebug(l) << "Bluetooth switched off.";
        scheduleReconnect();
        return;
    }

    if (socket != nullptr) {
        socket->close();
        socket->deleteLater();
    }

    _versions.clear();

    qCDebug(l) << "Creating socket";
    socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
    QObject::connect(socket, SIGNAL(readyRead()), SLOT(onReadSocket()));
    QObject::connect(socket, SIGNAL(bytesWritten(qint64)), SLOT(onBytesWritten(qint64)));
    QObject::connect(socket, SIGNAL(connected()), SLOT(onConnected()));
    QObject::connect(socket, SIGNAL(disconnected()), SLOT(onDisconnected()));
    QObject::connect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(onError(QBluetoothSocket::SocketError)));

    // FIXME: Assuming port 1 (with Pebble)
    socket->connectToService(_address, 1);
}

QString WatchConnector::decodeEndpoint(uint val)
{
    QMetaEnum Endpoints = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("Endpoint"));
    const char *endpoint = Endpoints.valueToKey(val);
    return endpoint ? QString(endpoint) : QString("watchUNKNOWN_%1").arg(val);
}

void WatchConnector::setEndpointHandler(uint endpoint, const EndpointHandlerFunc &func)
{
    if (func) {
        handlers.insert(endpoint, func);
    } else {
        handlers.remove(endpoint);
    }
}

void WatchConnector::clearEndpointHandler(uint endpoint)
{
    handlers.remove(endpoint);
}

bool WatchConnector::dispatchMessage(uint endpoint, const QByteArray &data)
{
    auto tmp_it = tmpHandlers.find(endpoint);
    if (tmp_it != tmpHandlers.end()) {
        QList<EndpointHandlerFunc>& funcs = tmp_it.value();
        bool ok = false;
        for (int i = 0; i < funcs.size(); i++) {
            if (funcs[i](data)) {
                // This handler accepted this message
                ok = true;
                // Since it is a temporary handler, remove it.
                funcs.removeAt(i);
                break;
            }
        }
        if (funcs.empty()) {
            // "Garbage collect" the tmpHandlers entry.
            tmpHandlers.erase(tmp_it);
        }
        if (ok) {
            return true;
        }
    }

    auto it = handlers.find(endpoint);
    if (it != handlers.end()) {
        if (it.value() && it.value()(data)) {
            return true;
        }
    }

    qCDebug(l) << "message to endpoint" << decodeEndpoint(endpoint) << "was not dispatched";
    qCDebug(l) << data.toHex();
    return false;
}

void WatchConnector::onReadSocket()
{
    static const int header_length = 4;

    qCDebug(l) << "readyRead bytesAvailable =" << socket->bytesAvailable();

    QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
    Q_ASSERT(socket && socket == this->socket);

    // Keep attempting to read messages as long as at least a header is present
    while (socket->bytesAvailable() >= header_length) {
        // Take a look at the header, but do not remove it from the socket input buffer.
        // We will only remove it once we're sure the entire packet is in the buffer.
        uchar header[header_length];
        socket->peek(reinterpret_cast<char*>(header), header_length);

        quint16 message_length = qFromBigEndian<quint16>(&header[0]);
        quint16 endpoint = qFromBigEndian<quint16>(&header[2]);

        // Sanity checks on the message_length
        if (message_length == 0) {
            qCWarning(l) << "received empty message";
            socket->read(header_length); // skip this header
            continue; // check if there are additional headers.
        } else if (message_length > 8 * 1024) {
            // Protocol does not allow messages more than 8K long, seemingly.
            qCWarning(l) << "received message size too long: " << message_length;
            socket->readAll(); // drop entire input buffer
            return;
        }

        // Now wait for the entire message
        if (socket->bytesAvailable() < header_length + message_length) {
            qCDebug(l) << "incomplete msg body in read buffer";
            return; // try again once more data comes in
        }

        // We can now safely remove the header from the input buffer,
        // as we know the entire message is in the input buffer.
        socket->read(header_length);

        // Now read the rest of the message
        QByteArray data = socket->read(message_length);

        qCDebug(l) << "received message of length" << message_length << "to endpoint" << decodeEndpoint(endpoint);
        if (PROTOCOL_DEBUG) qCDebug(l) << data.toHex();

        dispatchMessage(endpoint, data);
    }
}

void WatchConnector::onConnected()
{
    qCDebug(l) << "Connected!";
    bool was_connected = is_connected;
    is_connected = true;
    reconnectTimer.stop();
    reconnectTimer.setInterval(0);
    if (!was_connected) {
        if (!writeData.isEmpty()) {
            qCDebug(l) << "Found" << writeData.length() << "bytes in write buffer - resending";
            sendData(writeData);
        }
        sendMessage(watchVERSION, QByteArray(1, 0));
        emit connectedChanged();
        quint64 new_address = address().toUInt64();
        if (new_address != _last_address) emit pebbleChanged();
        _last_address = new_address;
        time();
    }
}

void WatchConnector::onDisconnected()
{
    qCDebug(l) << "Disconnected!";

    bool was_connected = is_connected;
    is_connected = false;

    if (was_connected) emit connectedChanged();

    QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
    if (!socket) return;

    socket->deleteLater();

    if (not writeData.isEmpty() && reconnectTimer.interval() > RECONNECT_TIMEOUT) {
        writeData.clear(); // 3rd time around - user is not here, do not bother with resending last message
    }
    timeSyncTimer.stop();
    scheduleReconnect();

}

void WatchConnector::scheduleReconnect()
{
    if (reconnectTimer.interval() < 10 * RECONNECT_TIMEOUT) {
        reconnectTimer.setInterval(reconnectTimer.interval() + RECONNECT_TIMEOUT);
    }
    reconnectTimer.start();
    qCDebug(l) << "will reconnect in" << reconnectTimer.interval() << "ms";
}

void WatchConnector::onError(QBluetoothSocket::SocketError error)
{
    if (error == QBluetoothSocket::UnknownSocketError) {
        QString errorString = socket->errorString();
        qCDebug(l) << error << errorString;
        if (errorString.endsWith(" down")) {
            currentPebble++;
        }
    } else {
        qCCritical(l) << "error connecting Pebble:" << error << socket->errorString();
    }
    scheduleReconnect();
}

void WatchConnector::sendData(const QByteArray &data)
{
    writeData.append(data);
    if (socket == nullptr) {
        qCDebug(l) << "no socket - reconnecting";
        connect();
    } else if (is_connected) {
        qCDebug(l) << "writing" << data.length() << "bytes to socket";
        if (PROTOCOL_DEBUG) qCDebug(l) << data.toHex();
        socket->write(data);
    }
}

void WatchConnector::onBytesWritten(qint64 bytes)
{
    writeData.remove(0, bytes);
    qCDebug(l) << "socket written" << bytes << "bytes," << writeData.length() << "left";
}

void WatchConnector::sendMessage(uint endpoint, const QByteArray &data, const EndpointHandlerFunc &callback)
{
    qCDebug(l) << "sending message to endpoint" << decodeEndpoint(endpoint);
    QByteArray msg;

    // First send the length
    msg.append((data.length() & 0xFF00) >> 8);
    msg.append(data.length() & 0xFF);

    // Then the endpoint
    msg.append((endpoint & 0xFF00) >> 8);
    msg.append(endpoint & 0xFF);

    // Finally the data
    msg.append(data);

    sendData(msg);

    if (callback) {
        tmpHandlers[endpoint].append(callback);
    }
}

void WatchConnector::buildData(QByteArray &res, QStringList data)
{
    for (QString d : data)
    {
        QByteArray tmp = d.left(0xEF).toUtf8();
        res.append((tmp.length() + 1) & 0xFF);
        res.append(tmp);
        res.append('\0');
    }
}

QByteArray WatchConnector::buildMessageData(uint lead, QStringList data)
{
    QByteArray res;
    res.append(lead & 0xFF);
    buildData(res, data);

    return res;
}

void WatchConnector::sendPhoneVersion()
{
    unsigned int sessionCap = sessionCapGAMMA_RAY;
    unsigned int remoteCap = remoteCapTELEPHONY | remoteCapSMS | osANDROID;
    QByteArray res;

    //Prefix
    res.append(0x01);
    res.append(0xff);
    res.append(0xff);
    res.append(0xff);
    res.append(0xff);

    //Session Capabilities
    res.append((char)((sessionCap >> 24) & 0xff));
    res.append((char)((sessionCap >> 16) & 0xff));
    res.append((char)((sessionCap >> 8) & 0xff));
    res.append((char)(sessionCap & 0xff));

    //Remote Capabilities
    res.append((char)((remoteCap >> 24) & 0xff));
    res.append((char)((remoteCap >> 16) & 0xff));
    res.append((char)((remoteCap >> 8) & 0xff));
    res.append((char)(remoteCap & 0xff));

    //Version Magic
    res.append((char)0x02);

    //Append Version
    res.append((char)0x02); //Major
    res.append((char)0x00); //Minor
    res.append((char)0x00); //Bugfix

    sendMessage(watchPHONE_VERSION, res);
}

void WatchConnector::systemMessage(SystemMessage msg, const EndpointHandlerFunc &callback)
{
    QByteArray res;
    res.append((char)0);
    res.append((char)msg);
    sendMessage(watchSYSTEM_MESSAGE, res, callback);
}

void WatchConnector::ping(uint cookie)
{
    QByteArray res;
    res.append((char)0);

    res.append((char)((cookie >> 24) & 0xff));
    res.append((char)((cookie >> 16) & 0xff));
    res.append((char)((cookie >> 8) & 0xff));
    res.append((char)(cookie & 0xff));

    sendMessage(watchPING, res);
}

void WatchConnector::time()
{
    qCDebug(l) << "Synchronizing time";
    timeSyncTimer.stop();
    QByteArray res;
    QDateTime UTC(QDateTime::currentDateTimeUtc());
    QDateTime local(UTC.toLocalTime());
    local.setTimeSpec(Qt::UTC);
    int offset = UTC.secsTo(local);
    uint val = (local.toMSecsSinceEpoch() + offset) / 1000;

    res.append(0x02); //SET_TIME_REQ
    res.append((char)((val >> 24) & 0xff));
    res.append((char)((val >> 16) & 0xff));
    res.append((char)((val >> 8) & 0xff));
    res.append((char)(val & 0xff));
    sendMessage(watchTIME, res);
    timeSyncTimer.start();
}

QString WatchConnector::timeStamp()
{
    return QString::number(QDateTime::currentMSecsSinceEpoch());
}

void WatchConnector::sendNotification(uint lead, QString sender, QString data, QString subject)
{
    switch (platform) {
    case HP_UNKNOWN:
        qCWarning(l) << "Tried sending notification to UNKNOWN watch platform" << lead << sender << data << subject;
        break;
    case APLITE: {
        QStringList tmp;
        tmp.append(sender);
        tmp.append(data);
        tmp.append(timeStamp());
        if (lead == leadEMAIL) tmp.append(subject);

        QByteArray res = buildMessageData(lead, tmp);

        sendMessage(watchNOTIFICATION, res);
        }
        break;
    case BASALT: {
        qCWarning(l) << "Tried sending notification to unsupported watch platform" << lead << sender << data << subject;
        }
        break;
    default:
        qCWarning(l) << "Tried sending notification to unsupported watch platform" << platform << ":" << lead << sender << data << subject;
        break;
    }

}

void WatchConnector::sendSMSNotification(QString sender, QString data)
{
    sendNotification(leadSMS, sender, data, "");
}

void WatchConnector::sendFacebookNotification(QString sender, QString data)
{
    sendNotification(leadFACEBOOK, sender, data, "");
}

void WatchConnector::sendTwitterNotification(QString sender, QString data)
{
    sendNotification(leadTWITTER, sender, data, "");
}

void WatchConnector::sendEmailNotification(QString sender, QString data, QString subject)
{
    sendNotification(leadEMAIL, sender, data, subject);
}

void WatchConnector::sendMusicNowPlaying(QString artist, QString album, QString track)
{
    QStringList tmp;
    tmp.append(artist.left(30));
    tmp.append(album.left(30));
    tmp.append(track.left(30));
    QByteArray res = buildMessageData(leadNOW_PLAYING_DATA, tmp);

    sendMessage(watchMUSIC_CONTROL, res);
}

void WatchConnector::sendFirmwareState(bool ok)
{
    systemMessage(ok ? systemFIRMWARE_UP_TO_DATE : systemFIRMWARE_OUT_OF_DATE);
}

void WatchConnector::phoneControl(char act, uint cookie, QStringList datas)
{
    QByteArray head;
    head.append((char)act);
    head.append((cookie >> 24)& 0xFF);
    head.append((cookie >> 16)& 0xFF);
    head.append((cookie >> 8)& 0xFF);
    head.append(cookie & 0xFF);
    if (datas.length()>0) buildData(head, datas);

    sendMessage(watchPHONE_CONTROL, head);
}

void WatchConnector::ring(QString number, QString name, bool incoming, uint cookie)
{
    QStringList tmp;
    tmp.append(number);
    tmp.append(name);

    char act = callINCOMING;
    if (!incoming) {
        act = callOUTGOING;
    }

    phoneControl(act, cookie, tmp);
}

void WatchConnector::startPhoneCall(uint cookie)
{
    phoneControl(callSTART, cookie, QStringList());
}

void WatchConnector::endPhoneCall(uint cookie)
{
    phoneControl(callEND, cookie, QStringList());
}
