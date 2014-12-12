#ifndef UPLOADMANAGER_H
#define UPLOADMANAGER_H

#include <functional>
#include <QQueue>
#include "watchconnector.h"
#include "stm32crc.h"

class UploadManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit UploadManager(WatchConnector *watch, QObject *parent = 0);

    typedef std::function<void()> SuccessCallback;
    typedef std::function<void(int)> ErrorCallback;
    typedef std::function<void(qreal)> ProgressCallback;

    uint upload(WatchConnector::UploadType type, int index, const QString &filename, QIODevice *device, int size = -1,
                SuccessCallback successCallback = SuccessCallback(), ErrorCallback errorCallback = ErrorCallback(), ProgressCallback progressCallback = ProgressCallback());

    uint uploadAppBinary(int slot, QIODevice *device, SuccessCallback successCallback = SuccessCallback(), ErrorCallback errorCallback = ErrorCallback(), ProgressCallback progressCallback = ProgressCallback());
    uint uploadAppResources(int slot, QIODevice *device, SuccessCallback successCallback = SuccessCallback(), ErrorCallback errorCallback = ErrorCallback(), ProgressCallback progressCallback = ProgressCallback());
    uint uploadFile(const QString &filename, QIODevice *device, SuccessCallback successCallback = SuccessCallback(), ErrorCallback errorCallback = ErrorCallback(), ProgressCallback progressCallback = ProgressCallback());

    void cancel(uint id, int code = 0);

signals:

public slots:


private:
    enum State {
        StateNotStarted,
        StateWaitForToken,
        StateInProgress,
        StateCommit,
        StateComplete
    };

    struct PendingUpload {
        uint id;

        WatchConnector::UploadType type;
        int index;
        QString filename;
        QIODevice *device;
        int size;
        int remaining;
        Stm32Crc crc;

        SuccessCallback successCallback;
        ErrorCallback errorCallback;
        ProgressCallback progressCallback;
    };

    void startNextUpload();
    void handleMessage(const QByteArray &msg);
    bool uploadNextChunk(PendingUpload &upload);
    bool commit(PendingUpload &upload);
    bool complete(PendingUpload &upload);

private:
    WatchConnector *watch;
    QQueue<PendingUpload> _pending;
    uint _lastUploadId;
    State _state;
    quint32 _token;
};

#endif // UPLOADMANAGER_H
