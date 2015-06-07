#ifndef WATCHCONNECTOR_H
#define WATCHCONNECTOR_H

#include <functional>
#include <QDebug>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QTimer>
#include <QDateTime>
#include <QBluetoothDeviceInfo>
#include <QBluetoothLocalDevice>
#include <QBluetoothSocket>
#include <QBluetoothServiceInfo>
#include <QLoggingCategory>

class WatchConnector : public QObject
{
    Q_OBJECT
    QLoggingCategory l;

    Q_ENUMS(Endpoint)

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString connected READ isConnected NOTIFY connectedChanged)

public:
    enum Endpoint {
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
        watchFILE_MANAGER = 8181,
        watchCORE_DUMP = 9000,
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
    enum SystemMessage {
        systemFIRMWARE_AVAILABLE = 0,
        systemFIRMWARE_START = 1,
        systemFIRMWARE_COMPLETE = 2,
        systemFIRMWARE_FAIL = 3,
        systemFIRMWARE_UP_TO_DATE = 4,
        systemFIRMWARE_OUT_OF_DATE = 5,
        systemBLUETOOTH_START_DISCOVERABLE = 6,
        systemBLUETOOTH_END_DISCOVERABLE = 7
    };
    enum AppManager {
        appmgrGET_APPBANK_STATUS = 1,
        appmgrREMOVE_APP = 2,
        appmgrREFRESH_APP = 3,
        appmgrGET_APPBANK_UUIDS = 5
    };
    enum AppMessage {
        appmsgPUSH = 1,
        appmsgREQUEST = 2,
        appmsgACK = 0xFF,
        appmsgNACK = 0x7F
    };
    enum DataLogMessage {
        datalogOPEN = 1,
        datalogDATA = 2,
        datalogCLOSE = 3,
        datalogTIMEOUT = 7
    };
    enum {
        launcherSTARTED = 1,
        launcherSTOPPED = 0
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
    enum UploadType {
        uploadFIRMWARE = 1,
        uploadRECOVERY = 2,
        uploadSYS_RESOURCES = 3,
        uploadRESOURCES = 4,
        uploadBINARY = 5,
        uploadFILE = 6,
        uploadWORKER = 7
    };
    enum PutBytesCommand {
        putbytesINIT = 1,
        putbytesSEND = 2,
        putbytesCOMMIT = 3,
        putbytesABORT = 4,
        putbytesCOMPLETE = 5
    };

    enum HardwareRevision {
        UNKNOWN = 0,
        TINTIN_EV1 = 1,
        TINTIN_EV2 = 2,
        TINTIN_EV2_3 = 3,
        TINTIN_EV2_4 = 4,
        TINTIN_V1_5 = 5,
        BIANCA = 6,
        SNOWY_EVT2 = 7,
        SNOWY_DVT = 8,

        TINTIN_BB = 0xFF,
        TINTIN_BB2 = 0xFE,
        SNOWY_BB = 0xFD,
        SNOWY_BB2 = 0xFC
    };
    QMap<HardwareRevision, QString> firmwareMapping;

    struct SoftwareVersion {
        QDateTime build;
        QString version;
        QString commit;
        bool is_recovery;
        HardwareRevision hw_revision;
        QString hw_string;
        quint8 metadata_version;

        QVariantMap toMap() const;
    };

    struct WatchVersions {
        SoftwareVersion main;
        SoftwareVersion safe;
        QDateTime bootLoaderBuild;
        QString hardwareRevision;
        QString serialNumber;
        QByteArray address;

        QVariantMap toMap() const;
        void clear();
        bool isEmpty() const;
    };

    typedef QMap<int, QVariant> Dict;
    enum DictItemType {
        typeBYTES,
        typeSTRING,
        typeUINT,
        typeINT
    };

    typedef std::function<bool(const QByteArray &)> EndpointHandlerFunc;

    explicit WatchConnector(QObject *parent = 0);
    virtual ~WatchConnector();

    inline bool isConnected() const { return is_connected; }
    inline QString name() const { return socket != nullptr ? socket->peerName() : ""; }
    inline WatchVersions versions() const { return _versions; }

    void setEndpointHandler(uint endpoint, const EndpointHandlerFunc &func);
    void clearEndpointHandler(uint endpoint);

    static QString timeStamp();
    static QString decodeEndpoint(uint val);

signals:
    void nameChanged();
    void versionsChanged();
    void connectedChanged();

public slots:
    void deviceConnect(const QString &name, const QString &address);
    void scheduleReconnect();
    void disconnect();
    void reconnect();

    void sendMessage(uint endpoint, const QByteArray &data, const EndpointHandlerFunc &callback = EndpointHandlerFunc());
    void ping(uint cookie);
    void systemMessage(SystemMessage msg, const EndpointHandlerFunc &callback = EndpointHandlerFunc());
    void time();

    void sendNotification(uint lead, QString sender, QString data, QString subject);
    void sendSMSNotification(QString sender, QString data);
    void sendEmailNotification(QString sender, QString data, QString subject);
    void sendFacebookNotification(QString sender, QString data);
    void sendTwitterNotification(QString sender, QString data);
    void sendMusicNowPlaying(QString artist, QString album, QString track);
    void sendPhoneVersion();
    void sendFirmwareState(bool ok);

    void buildData(QByteArray &res, QStringList data);
    QByteArray buildMessageData(uint lead, QStringList data);

    void phoneControl(char act, uint cookie, QStringList datas);
    void ring(QString number, QString name, bool incoming=true, uint cookie=0);
    void startPhoneCall(uint cookie=0);
    void endPhoneCall(uint cookie=0);

private slots:
    void deviceDiscovered(const QBluetoothDeviceInfo&);
    void handleWatch(const QString &name, const QString &address);
    void onReadSocket();
    void onBytesWritten(qint64);
    void onConnected();
    void onDisconnected();
    void onError(QBluetoothSocket::SocketError error);

private:
    void sendData(const QByteArray &data);
    bool dispatchMessage(uint endpoint, const QByteArray &data);

    QPointer<QBluetoothSocket> socket;
    QHash<uint, QList<EndpointHandlerFunc>> tmpHandlers;
    QHash<uint, EndpointHandlerFunc> handlers;
    bool is_connected;
    QByteArray writeData;
    QTimer reconnectTimer;
    QTimer timeSyncTimer;
    QString _last_name;
    QString _last_address;
    WatchVersions _versions;
};

QDebug operator<< (QDebug d, const WatchConnector::SoftwareVersion &ver);
QDebug operator<< (QDebug d, const WatchConnector::WatchVersions &ver);

#endif // WATCHCONNECTOR_H
