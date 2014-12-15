#include "watchconnector.h"
#include <QTimer>
#include <QDateTime>
#include <QMetaEnum>

using namespace watch;

static int RECONNECT_TIMEOUT = 500; //ms

WatchConnector::WatchConnector(QObject *parent) :
    QObject(parent), l(metaObject()->className()), socket(nullptr), is_connected(false)
{
    reconnectTimer.setSingleShot(true);
    connect(&reconnectTimer, SIGNAL(timeout()), SLOT(reconnect()));
}

WatchConnector::~WatchConnector()
{
}

void WatchConnector::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    //FIXME TODO: Configurable
    if (device.name().startsWith("Pebble")) {
        qCDebug(l) << "Found Pebble:" << device.name() << '(' << device.address().toString() << ')';
        handleWatch(device.name(), device.address().toString());
    } else {
        qCDebug(l) << "Found other device:" << device.name() << '(' << device.address().toString() << ')';
    }
}

void WatchConnector::deviceConnect(const QString &name, const QString &address)
{
    if (name.startsWith("Pebble")) handleWatch(name, address);
}

void WatchConnector::reconnect()
{
    qCDebug(l) << "reconnect" << _last_name;
    if (!_last_name.isEmpty() && !_last_address.isEmpty()) {
        deviceConnect(_last_name, _last_address);
    }
}

void WatchConnector::disconnect()
{
    qCDebug(l) << __FUNCTION__;
    socket->close();
    socket->deleteLater();
    reconnectTimer.stop();
    qCDebug(l) << "Stopped reconnect timer";
}

void WatchConnector::handleWatch(const QString &name, const QString &address)
{
    qCDebug(l) << "handleWatch" << name << address;
    reconnectTimer.stop();
    if (socket != nullptr && socket->isOpen()) {
        socket->close();
        socket->deleteLater();
    }

    bool emit_name = (_last_name != name);
    _last_name = name;
    _last_address = address;
    if (emit_name) emit nameChanged();

    qCDebug(l) << "Creating socket";
    socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol);
    connect(socket, SIGNAL(readyRead()), SLOT(onReadSocket()));
    connect(socket, SIGNAL(bytesWritten(qint64)), SLOT(onBytesWritten(qint64)));
    connect(socket, SIGNAL(connected()), SLOT(onConnected()));
    connect(socket, SIGNAL(disconnected()), SLOT(onDisconnected()));
    connect(socket, SIGNAL(error(QBluetoothSocket::SocketError)), this, SLOT(onError(QBluetoothSocket::SocketError)));

    // FIXME: Assuming port 1 (with Pebble)
    socket->connectToService(QBluetoothAddress(address), 1);
}

QString WatchConnector::decodeEndpoint(uint val)
{
    QMetaEnum Endpoints = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("Endpoints"));
    const char *endpoint = Endpoints.valueToKey(val);
    return endpoint ? QString(endpoint) : QString("watchUNKNOWN_%1").arg(val);
}

void WatchConnector::decodeMsg(QByteArray data)
{
    //Sometimes pebble sends a "00", we ignore it without future action
    if (data.length() == 1 && data.at(0) == 0) {
        return;
    }

    if (data.length() < 4) {
        qCCritical(l) << "Can not decode message data length invalid: " << data.toHex();
        return;
    }

    unsigned int datalen = 0;
    int index = 0;
    datalen = (data.at(index) << 8) + data.at(index+1);
    index += 2;

    unsigned int endpoint = 0;
    endpoint = (data.at(index) << 8) + data.at(index+1);
    index += 2;

    qCDebug(l) << "Length:" << datalen << "Endpoint:" << decodeEndpoint(endpoint);
    qCDebug(l) << "Data:" << data.mid(index).toHex();

    emit messageDecoded(endpoint, data.mid(index, datalen));
}

void WatchConnector::onReadSocket()
{
    qCDebug(l) << "read";

    QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
    if (!socket) return;

    while (socket->bytesAvailable()) {
        QByteArray line = socket->readAll();
        emit messageReceived(socket->peerName(), QString::fromUtf8(line.constData(), line.length()));
        decodeMsg(line);
    }
}

void WatchConnector::onConnected()
{
    qCDebug(l) << "Connected!";
    bool was_connected = is_connected;
    is_connected = true;
    reconnectTimer.stop();
    reconnectTimer.setInterval(0);
    if (not was_connected) {
        if (not writeData.isEmpty()) {
            qCDebug(l) << "Found" << writeData.length() << "bytes in write buffer - resending";
            sendData(writeData);
        }
        emit connectedChanged();
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

    if (reconnectTimer.interval() < 10 * RECONNECT_TIMEOUT) {
        reconnectTimer.setInterval(reconnectTimer.interval() + RECONNECT_TIMEOUT);
    }
    reconnectTimer.start();
    qCDebug(l) << "Will reconnect in" << reconnectTimer.interval() << "ms";
}

void WatchConnector::onError(QBluetoothSocket::SocketError error)
{
    if (error == QBluetoothSocket::UnknownSocketError) {
        qCDebug(l) << error << socket->errorString();
    } else {
        qCCritical(l) << "Error connecting Pebble:" << error << socket->errorString();
    }
}

void WatchConnector::sendData(const QByteArray &data)
{
    writeData = data;
    if (socket == nullptr) {
        qCDebug(l) << "No socket - reconnecting";
        reconnect();
        return;
    }
    if (is_connected) {
        qCDebug(l) << "Writing" << data.length() << "bytes to socket";
        socket->write(data);
    }
}

void WatchConnector::onBytesWritten(qint64 bytes)
{
    writeData = writeData.mid(bytes);
    qCDebug(l) << "Socket written" << bytes << "bytes," << writeData.length() << "left";
}

void WatchConnector::sendMessage(uint endpoint, QByteArray data)
{
    qCDebug(l) << "Sending message";
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
    unsigned int remoteCap = remoteCapTELEPHONY | remoteCapSMS | osIOS;
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

void WatchConnector::ping(uint val)
{
    QByteArray res;
    res.append((char)0);

    res.append((char)((val >> 24) & 0xff));
    res.append((char)((val >> 16) & 0xff));
    res.append((char)((val >> 8) & 0xff));
    res.append((char)(val & 0xff));

    sendMessage(watchPING, res);
}

void WatchConnector::time()
{
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
}

QString WatchConnector::timeStamp()
{
    return QString::number(QDateTime::currentMSecsSinceEpoch());
}

void WatchConnector::sendNotification(uint lead, QString sender, QString data, QString subject)
{
    QStringList tmp;
    tmp.append(sender);
    tmp.append(data);
    tmp.append(timeStamp());
    if (lead == leadEMAIL) tmp.append(subject);

    QByteArray res = buildMessageData(lead, tmp);

    sendMessage(watchNOTIFICATION, res);
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

void WatchConnector::sendMusicNowPlaying(QString track, QString album, QString artist)
{
    QStringList tmp;
    tmp.append(track.left(30));
    tmp.append(album.left(30));
    tmp.append(artist.left(30));

    QByteArray res = buildMessageData(leadNOW_PLAYING_DATA, tmp);

    sendMessage(watchMUSIC_CONTROL, res);
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
