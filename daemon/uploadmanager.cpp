#include "uploadmanager.h"
#include "unpacker.h"
#include "packer.h"
#include "stm32crc.h"

static const int CHUNK_SIZE = 2000;
using std::function;

UploadManager::UploadManager(WatchConnector *watch, QObject *parent) :
    QObject(parent), watch(watch), _lastUploadId(0), _state(StateNotStarted)
{
    watch->setEndpointHandler(WatchConnector::watchPUTBYTES,
                              [this](const QByteArray &msg) {
        if (_pending.empty()) {
            logger()->warn() << "putbytes message, but queue is empty!";
            return false;
        }
        handleMessage(msg);
        return true;
    });
}

uint UploadManager::upload(WatchConnector::UploadType type, int index, const QString &filename, QIODevice *device, int size,
                           SuccessCallback successCallback, ErrorCallback errorCallback, ProgressCallback progressCallback)
{
    PendingUpload upload;
    upload.id = ++_lastUploadId;
    upload.type = type;
    upload.index = index;
    upload.filename = filename;
    upload.device = device;
    if (size < 0) {
        upload.size = device->size();
    } else {
        upload.size = size;
    }
    upload.remaining = upload.size;
    upload.successCallback = successCallback;
    upload.errorCallback = errorCallback;
    upload.progressCallback = progressCallback;

    if (upload.remaining <= 0) {
        logger()->warn() << "upload is empty";
        if (errorCallback) {
            errorCallback(-1);
            return -1;
        }
    }

    _pending.enqueue(upload);

    if (_pending.size() == 1) {
        startNextUpload();
    }

    return upload.id;
}

uint UploadManager::uploadAppBinary(int slot, QIODevice *device, SuccessCallback successCallback, ErrorCallback errorCallback, ProgressCallback progressCallback)
{
    return upload(WatchConnector::uploadBINARY, slot, QString(), device, -1, successCallback, errorCallback, progressCallback);
}

uint UploadManager::uploadAppResources(int slot, QIODevice *device, SuccessCallback successCallback, ErrorCallback errorCallback, ProgressCallback progressCallback)
{
    return upload(WatchConnector::uploadRESOURCES, slot, QString(), device, -1, successCallback, errorCallback, progressCallback);
}

uint UploadManager::uploadFile(const QString &filename, QIODevice *device, SuccessCallback successCallback, ErrorCallback errorCallback, ProgressCallback progressCallback)
{
    Q_ASSERT(!filename.isEmpty());
    return upload(WatchConnector::uploadFILE, 0, filename, device, -1, successCallback, errorCallback, progressCallback);
}

void UploadManager::cancel(uint id, int code)
{
    if (_pending.empty()) {
        logger()->warn() << "cannot cancel, empty queue";
        return;
    }

    if (id == _pending.head().id) {
        PendingUpload upload = _pending.dequeue();
        logger()->debug() << "aborting current upload" << id << "(code:" << code << ")";

        if (_state != StateNotStarted && _state != StateWaitForToken && _state != StateComplete) {
            QByteArray msg;
            Packer p(&msg);
            p.write<quint8>(WatchConnector::putbytesABORT);
            p.write<quint32>(_token);

            logger()->debug() << "sending abort for upload" << id;

            watch->sendMessage(WatchConnector::watchPUTBYTES, msg);
        }

        _state = StateNotStarted;
        _token = 0;

        if (upload.errorCallback) {
            upload.errorCallback(code);
        }

        if (!_pending.empty()) {
            startNextUpload();
        }
    } else {
        for (int i = 1; i < _pending.size(); ++i) {
            if (_pending[i].id == id) {
                logger()->debug() << "cancelling upload" << id << "(code:" << code << ")";
                if (_pending[i].errorCallback) {
                    _pending[i].errorCallback(code);
                }
                _pending.removeAt(i);
                return;
            }
        }
        logger()->warn() << "cannot cancel, id" << id << "not found";
    }
}

void UploadManager::startNextUpload()
{
    Q_ASSERT(!_pending.empty());
    Q_ASSERT(_state == StateNotStarted);

    PendingUpload &upload = _pending.head();
    QByteArray msg;
    Packer p(&msg);
    p.write<quint8>(WatchConnector::putbytesINIT);
    p.write<quint32>(upload.remaining);
    p.write<quint8>(upload.type);
    p.write<quint8>(upload.index);
    if (!upload.filename.isEmpty()) {
        p.writeCString(upload.filename);
    }

    logger()->debug() << "starting new upload, size:" << upload.remaining << ", type:" << upload.type << ", slot:" << upload.index;

    _state = StateWaitForToken;
    watch->sendMessage(WatchConnector::watchPUTBYTES, msg);
}

void UploadManager::handleMessage(const QByteArray &msg)
{
    Q_ASSERT(!_pending.empty());
    PendingUpload &upload = _pending.head();

    Unpacker u(msg);
    int status = u.read<quint8>();

    if (u.bad() || status != 1) {
        logger()->warn() << "upload" << upload.id << "got error code=" << status;
        cancel(upload.id, status);
        return;
    }

    quint32 recv_token = u.read<quint32>();

    if (u.bad()) {
        logger()->warn() << "upload" << upload.id << ": could not read the token";
        cancel(upload.id, -1);
        return;
    }

    if (_state != StateNotStarted && _state != StateWaitForToken && _state != StateComplete) {
        if (recv_token != _token) {
            logger()->warn() << "upload" << upload.id << ": invalid token";
            cancel(upload.id, -1);
            return;
        }
    }

    switch (_state) {
    case StateNotStarted:
        logger()->warn() << "got packet when upload is not started";
        break;
    case StateWaitForToken:
        logger()->debug() << "token received";
        _token = recv_token;
        _state = StateInProgress;

        /* fallthrough */
    case StateInProgress:
        logger()->debug() << "moving to the next chunk";
        if (upload.progressCallback) {
            // Report that the previous chunk has been succesfully uploaded
            upload.progressCallback(1.0 - (qreal(upload.remaining) / upload.size));
        }
        if (upload.remaining > 0) {
            if (!uploadNextChunk(upload)) {
                cancel(upload.id, -1);
                return;
            }
        } else {
            logger()->debug() << "no additional chunks, commit";
            _state = StateCommit;
            if (!commit(upload)) {
                cancel(upload.id, -1);
                return;
            }
        }
        break;
    case StateCommit:
        logger()->debug() << "commited succesfully";
        if (upload.progressCallback) {
            // Report that all chunks have been succesfully uploaded
            upload.progressCallback(1.0);
        }
        _state = StateComplete;
        if (!complete(upload)) {
            cancel(upload.id, -1);
            return;
        }
        break;
    case StateComplete:
        logger()->debug() << "upload" << upload.id << "succesful, invoking callback";
        if (upload.successCallback) {
            upload.successCallback();
        }
        _pending.dequeue();
        _token = 0;
        _state = StateNotStarted;
        if (!_pending.empty()) {
            startNextUpload();
        }
        break;
    default:
        logger()->warn() << "received message in wrong state";
        break;
    }
}

bool UploadManager::uploadNextChunk(PendingUpload &upload)
{
    QByteArray chunk = upload.device->read(qMin<int>(upload.remaining, CHUNK_SIZE));

    if (upload.remaining < CHUNK_SIZE && chunk.size() < upload.remaining) {
        // Short read!
        logger()->warn() << "short read during upload" << upload.id;
        return false;
    }

    Q_ASSERT(!chunk.isEmpty());
    Q_ASSERT(_state = StateInProgress);

    QByteArray msg;
    Packer p(&msg);
    p.write<quint8>(WatchConnector::putbytesSEND);
    p.write<quint32>(_token);
    p.write<quint32>(chunk.size());
    msg.append(chunk);

    logger()->debug() << "sending a chunk of" << chunk.size() << "bytes";

    watch->sendMessage(WatchConnector::watchPUTBYTES, msg);

    upload.remaining -= chunk.size();
    upload.crc.addData(chunk);

    logger()->debug() << "remaining" << upload.remaining << "/" << upload.size << "bytes";

    return true;
}

bool UploadManager::commit(PendingUpload &upload)
{
    Q_ASSERT(_state == StateCommit);
    Q_ASSERT(upload.remaining == 0);

    QByteArray msg;
    Packer p(&msg);
    p.write<quint8>(WatchConnector::putbytesCOMMIT);
    p.write<quint32>(_token);
    p.write<quint32>(upload.crc.result());

    logger()->debug() << "commiting upload" << upload.id
                      << "with crc" << qPrintable(QString("0x%1").arg(upload.crc.result(), 0, 16));

    watch->sendMessage(WatchConnector::watchPUTBYTES, msg);

    return true;
}

bool UploadManager::complete(PendingUpload &upload)
{
    Q_ASSERT(_state == StateComplete);

    QByteArray msg;
    Packer p(&msg);
    p.write<quint8>(WatchConnector::putbytesCOMPLETE);
    p.write<quint32>(_token);

    logger()->debug() << "completing upload" << upload.id;

    watch->sendMessage(WatchConnector::watchPUTBYTES, msg);

    return true;
}
