/*
  Copyright (C) 2014 Jouni Roivas
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the authors nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef WATCHCONNECTOR_H
#define WATCHCONNECTOR_H

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTimer>
#include <QBluetoothDeviceInfo>
#include <QBluetoothSocket>
#include <QBluetoothServiceInfo>
#include <Log4Qt/Logger>

using namespace QtBluetooth;

namespace watch
{

class WatchConnector : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    Q_ENUMS(Endpoints)

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString connected READ isConnected NOTIFY connectedChanged)

public:
    enum Endpoints {
        watchTIME = 11,
        watchVERSION = 16,
        watchPHONE_VERSION = 17,
        watchSYSTEM_MESSAGE = 18,
        watchMUSIC_CONTROL = 32,
        watchPHONE_CONTROL = 33,
        watchAPPLICATION_MESSAGE = 48,
        watchLAUNCHER = 49,
        watchLOGS = 2000,
        watchPING = 2001,
        watchLOG_DUMP = 2002,
        watchRESET = 2003,
        watchAPP = 2004,
        watchAPP_LOGS = 2006,
        watchNOTIFICATION = 3000,
        watchRESOURCE = 4000,
        watchAPP_MANAGER = 6000,
        watchDATA_LOGGING = 6778,
        watchSCREENSHOT = 8000,
        watchPUTBYTES = 48879
    };
    enum {
        callANSWER = 1,
        callHANGUP = 2,
        callGET_STATE = 3,
        callINCOMING = 4,
        callOUTGOING = 5,
        callMISSED = 6,
        callRING = 7,
        callSTART = 8,
        callEND = 9
    };
    enum MusicControl {
        musicPLAY_PAUSE = 1,
        musicPAUSE = 2,
        musicPLAY = 3,
        musicNEXT = 4,
        musicPREVIOUS = 5,
        musicVOLUME_UP = 6,
        musicVOLUME_DOWN = 7,
        musicGET_NOW_PLAYING = 8,
        musicSEND_NOW_PLAYING = 9
    };
    enum {
        leadEMAIL = 0,
        leadSMS = 1,
        leadFACEBOOK = 2,
        leadTWITTER = 3,
        leadNOW_PLAYING_DATA = 16
    };
    enum {
         sessionCapGAMMA_RAY = 0x80000000
    };
    enum {
         remoteCapTELEPHONY = 16,
         remoteCapSMS = 32,
         remoteCapGPS = 64,
         remoteCapBTLE = 128,
         remoteCapCAMERA_REAR = 256,
         remoteCapACCEL = 512,
         remoteCapGYRO = 1024,
         remoteCapCOMPASS = 2048
    };
    enum {
         osUNKNOWN = 0,
         osIOS = 1,
         osANDROID = 2,
         osOSX = 3,
         osLINUX = 4,
         osWINDOWS = 5
    };


    explicit WatchConnector(QObject *parent = 0);
    virtual ~WatchConnector();
    bool isConnected() const { return is_connected; }
    QString name() const { return socket != nullptr ? socket->peerName() : ""; }

    QString timeStamp();
    QString decodeEndpoint(uint val);

signals:
    void messageReceived(QString peer, QString msg);
    void messageDecoded(uint endpoint, QByteArray data);
    void nameChanged();
    void connectedChanged();

public slots:
    void sendData(const QByteArray &data);
    void sendMessage(uint endpoint, QByteArray data);
    void ping(uint val);
    void time();
    void sendNotification(uint lead, QString sender, QString data, QString subject);
    void sendSMSNotification(QString sender, QString data);
    void sendEmailNotification(QString sender, QString data, QString subject);
    void sendFacebookNotification(QString sender, QString data);
    void sendTwitterNotification(QString sender, QString data);
    void sendMusicNowPlaying(QString track, QString album, QString artist);
    void sendPhoneVersion();

    void buildData(QByteArray &res, QStringList data);
    QByteArray buildMessageData(uint lead, QStringList data);

    void phoneControl(char act, uint cookie, QStringList datas);
    void ring(QString number, QString name, bool incoming=true, uint cookie=0);
    void startPhoneCall(uint cookie=0);
    void endPhoneCall(uint cookie=0);

    void deviceConnect(const QString &name, const QString &address);
    void disconnect();
    void deviceDiscovered(const QBluetoothDeviceInfo&);
    void handleWatch(const QString &name, const QString &address);
    void onReadSocket();
    void onConnected();
    void onDisconnected();
    void onError(QBluetoothSocket::SocketError error);
    void reconnect();

private:
    void decodeMsg(QByteArray data);

    QPointer<QBluetoothSocket> socket;
    bool is_connected;
    QTimer reconnectTimer;
    QString _last_name;
    QString _last_address;
};
}

#endif // WATCHCONNECTOR_H
