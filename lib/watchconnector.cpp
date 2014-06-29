#include "watchconnector.h"
#include <QTimer>
#include <QDateTime>

using namespace watch;

static int __reconnect_timeout = 5000; //ms

WatchConnector::WatchConnector(QObject *parent) :
    QObject(parent), socket(nullptr), is_connected(false)
{}

WatchConnector::~WatchConnector()
{
}

void WatchConnector::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    //FIXME TODO: Configurable
    if (device.name().startsWith("Pebble")) {
        qDebug() << "Found Pebble:" << device.name() << '(' << device.address().toString() << ')';
        handleWatch(device.name(), device.address().toString());
    } else {
        qDebug() << "Found other device:" << device.name() << '(' << device.address().toString() << ')';
    }
}

void WatchConnector::deviceConnect(const QString &name, const QString &address)
{
    if (name.startsWith("Pebble")) handleWatch(name, address);
}

void WatchConnector::reconnect()
{
    qDebug() << "reconnect" << _last_name;
    if (!_last_name.isEmpty() && !_last_address.isEmpty()) {
        deviceConnect(_last_name, _last_address);
    }
}

void WatchConnector::handleWatch(const QString &name, const QString &address)
{
    qDebug() << "handleWatch" << name << (socket != nullptr);
    if (socket != nullptr && socket->isOpen()) {
        socket->close();
        socket->deleteLater();
    }

    bool emit_name = (_last_name != name);
    _last_name = name;
    _last_address = address;
    if (emit_name) emit nameChanged();

    qDebug() << "Creating socket";
    socket = new QBluetoothSocket(QBluetoothSocket::RfcommSocket);

    // FIXME: Assuming port 1 (with Pebble)
    socket->connectToService(QBluetoothAddress(address), 1);

    connect(socket, SIGNAL(readyRead()), SLOT(onReadSocket()));
    connect(socket, SIGNAL(connected()), SLOT(onConnected()));
    connect(socket, SIGNAL(disconnected()), SLOT(onDisconnected()));
}

QString WatchConnector::decodeEndpoint(unsigned int val)
{
    //FIXME: Create a map of these values
    switch(val) {
        case watchTIME:
            return "TIME";
        case watchVERSION:
            return "VERSION";
        case watchPHONE_VERSION:
            return "PHONE_VERSION";
        case watchSYSTEM_MESSAGE:
            return "SYSTEM_MESSAGE";
        case watchMUSIC_CONTROL:
            return "MUSIC_CONTROL";
        case watchPHONE_CONTROL:
            return "PHONE_CONTROL";
        case watchAPPLICATION_MESSAGE:
            return "APP_MSG";
        case watchLAUNCHER:
            return "LAUNCHER";
        case watchLOGS:
            return "LOGS";
        case watchPING:
            return "PING";
        case watchLOG_DUMP:
            return "DUMP";
        case watchRESET:
            return "RESET";
        case watchAPP:
            return "APP";
        case watchAPP_LOGS:
            return "APP_LOGS";
        case watchNOTIFICATION:
            return "NOTIFICATION";
        case watchRESOURCE:
            return "RESOURCE";
        case watchAPP_MANAGER:
            return "APP_MANAG";
        case watchSCREENSHOT:
            return "SCREENSHOT";
        case watchPUTBYTES:
            return "PUTBYTES";
        default:
            return "Unknown: "+ QString::number(val);
    }
}

void WatchConnector::decodeMsg(QByteArray data)
{
    unsigned int datalen = 0;
    int index = 0;
    datalen = (data.at(index) << 8) + data.at(index+1);
    index += 2;

    unsigned int endpoint = 0;
    endpoint = (data.at(index) << 8) + data.at(index+1);
    index += 2;

    qDebug() << "Length:" << datalen << " Endpoint:" << decodeEndpoint(endpoint);
    qDebug() << "Data:" << data.mid(index).toHex();
    if (endpoint == watchPHONE_CONTROL) {
        if (data.length() >= 5) {
            if (data.at(4) == callHANGUP) {
                emit hangup();
            }
        }
    }
}

void WatchConnector::onReadSocket()
{
    qDebug() << "read";

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
    qDebug() << "Connected!";
    bool was_connected = is_connected;
    is_connected = true;
    if (not was_connected) emit connectedChanged();
}

void WatchConnector::onDisconnected()
{
    qDebug() << "Disconnected!";

    bool was_connected = is_connected;
    is_connected = false;

    QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
    if (!socket) return;

    if (was_connected) emit connectedChanged();

    socket->deleteLater();

    // Try to connect again after a timeout
    QTimer::singleShot(__reconnect_timeout, this, SLOT(reconnect()));
}

void WatchConnector::sendData(const QByteArray &data)
{
    if (socket == nullptr) return;

    socket->write(data);
}

void WatchConnector::sendMessage(unsigned int endpoint, QByteArray data)
{
    qDebug() << "Sending message";
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
        QByteArray tmp = d.left(0xF0).toUtf8();
        res.append(tmp.length() & 0xFF);
        res.append(tmp);
    }
}

QByteArray WatchConnector::buildMessageData(unsigned int lead, QStringList data)
{
    QByteArray res;
    res.append(lead & 0xFF);
    buildData(res, data);

    return res;
}

void WatchConnector::ping(unsigned int val)
{
    QByteArray res;
    res.append((char)0);

    res.append((char)((val >> 24) & 0xff));
    res.append((char)((val >> 16) & 0xff));
    res.append((char)((val >> 8) & 0xff));
    res.append((char)(val & 0xff));

    sendMessage(watchPING, res);
}

QString WatchConnector::timeStamp()
{
    return QString::number(QDateTime::currentMSecsSinceEpoch());
}

void WatchConnector::sendNotification(unsigned int lead, QString sender, QString data, QString subject)
{
    QStringList tmp;
    tmp.append(sender);
    tmp.append(data);
    tmp.append(timeStamp());
    if (lead == 0) tmp.append(subject);

    QByteArray res = buildMessageData(lead, tmp);

    sendMessage(watchNOTIFICATION, res);
}

void WatchConnector::sendSMSNotification(QString sender, QString data)
{
    sendNotification(1, sender, data, "");
}

void WatchConnector::sendEmailNotification(QString sender, QString data, QString subject)
{
    sendNotification(0, sender, data, subject);
}

void WatchConnector::phoneControl(char act, unsigned int cookie, QStringList datas)
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

void WatchConnector::ring(QString number, QString name, bool incoming, unsigned int cookie)
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

void WatchConnector::startPhoneCall(unsigned int cookie)
{
    phoneControl(callSTART, cookie, QStringList());
}

void WatchConnector::endPhoneCall(unsigned int cookie)
{
    phoneControl(callEND, cookie, QStringList());
}
