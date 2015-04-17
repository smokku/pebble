#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QAuthenticator>
#include <QBuffer>
#include <QDir>
#include <QCryptographicHash>
#include <limits>
#include "jskitobjects.h"

static const char *token_salt = "0feeb7416d3c4546a19b04bccd8419b1";

JSKitPebble::JSKitPebble(const AppInfo &info, JSKitManager *mgr, QObject *parent)
    : QObject(parent), l(metaObject()->className()), _appInfo(info), _mgr(mgr)
{
}

void JSKitPebble::addEventListener(const QString &type, QJSValue function)
{
    _callbacks[type].append(function);
}

void JSKitPebble::removeEventListener(const QString &type, QJSValue function)
{
    if (!_callbacks.contains(type)) return;
    QList<QJSValue> &callbacks = _callbacks[type];

    for (QList<QJSValue>::iterator it = callbacks.begin(); it != callbacks.end(); ) {
        if (it->strictlyEquals(function)) {
            it = callbacks.erase(it);
        } else {
            ++it;
        }
    }

    if (callbacks.empty()) {
        _callbacks.remove(type);
    }
}

uint JSKitPebble::sendAppMessage(QJSValue message, QJSValue callbackForAck, QJSValue callbackForNack)
{
    QVariantMap data = message.toVariant().toMap();
    QPointer<JSKitPebble> pebbObj = this;
    uint transactionId = _mgr->_appmsg->nextTransactionId();

    qCDebug(l) << "sendAppMessage" << data;

    _mgr->_appmsg->send(_appInfo.uuid(), data,
    [pebbObj, transactionId, callbackForAck]() mutable {
        if (pebbObj.isNull()) return;
        if (callbackForAck.isCallable()) {
            qCDebug(pebbObj->l) << "Invoking ack callback";
            QJSValue event = pebbObj->buildAckEventObject(transactionId);
            QJSValue result = callbackForAck.call(QJSValueList({event}));
            if (result.isError()) {
                qCWarning(pebbObj->l) << "error while invoking ACK callback" << callbackForAck.toString() << ":"
                                          << JSKitManager::describeError(result);
            }
        } else {
            qCDebug(pebbObj->l) << "Ack callback not callable";
        }
    },
    [pebbObj, transactionId, callbackForNack]() mutable {
        if (pebbObj.isNull()) return;
        if (callbackForNack.isCallable()) {
            qCDebug(pebbObj->l) << "Invoking nack callback";
            QJSValue event = pebbObj->buildAckEventObject(transactionId, "NACK from watch");
            QJSValue result = callbackForNack.call(QJSValueList({event}));
            if (result.isError()) {
                qCWarning(pebbObj->l) << "error while invoking NACK callback" << callbackForNack.toString() << ":"
                                          << JSKitManager::describeError(result);
            }
        } else {
            qCDebug(pebbObj->l) << "Nack callback not callable";
        }
    });

    return transactionId;
}

void JSKitPebble::showSimpleNotificationOnPebble(const QString &title, const QString &body)
{
    qCDebug(l) << "showSimpleNotificationOnPebble" << title << body;
    emit _mgr->appNotification(_appInfo.uuid(), title, body);
}

void JSKitPebble::openURL(const QUrl &url)
{
    qCDebug(l) << "opening url" << url.toString();
    emit _mgr->appOpenUrl(url);
}

QString JSKitPebble::getAccountToken() const
{
    // We do not have any account system, so we just fake something up.
    QCryptographicHash hasher(QCryptographicHash::Md5);

    hasher.addData(token_salt, strlen(token_salt));
    hasher.addData(_appInfo.uuid().toByteArray());

    QString token = _mgr->_settings->property("accountToken").toString();
    if (token.isEmpty()) {
        token = QUuid::createUuid().toString();
        qCDebug(l) << "created new account token" << token;
        _mgr->_settings->setProperty("accountToken", token);
    }
    hasher.addData(token.toLatin1());

    QString hash = hasher.result().toHex();
    qCDebug(l) << "returning account token" << hash;

    return hash;
}

QString JSKitPebble::getWatchToken() const
{
    QCryptographicHash hasher(QCryptographicHash::Md5);

    hasher.addData(token_salt, strlen(token_salt));
    hasher.addData(_appInfo.uuid().toByteArray());
    hasher.addData(_mgr->_watch->versions().serialNumber.toLatin1());

    QString hash = hasher.result().toHex();
    qCDebug(l) << "returning watch token" << hash;

    return hash;
}

QJSValue JSKitPebble::createXMLHttpRequest()
{
    JSKitXMLHttpRequest *xhr = new JSKitXMLHttpRequest(_mgr, 0);
    // Should be deleted by JS engine.
    return _mgr->engine()->newQObject(xhr);
}

QJSValue JSKitPebble::buildAckEventObject(uint transaction, const QString &message) const
{
    QJSEngine *engine = _mgr->engine();
    QJSValue eventObj = engine->newObject();
    QJSValue dataObj = engine->newObject();

    dataObj.setProperty("transactionId", engine->toScriptValue(transaction));
    eventObj.setProperty("data", dataObj);

    if (!message.isEmpty()) {
        QJSValue errorObj = engine->newObject();
        errorObj.setProperty("message", engine->toScriptValue(message));
        eventObj.setProperty("error", errorObj);
    }

    return eventObj;
}

void JSKitPebble::invokeCallbacks(const QString &type, const QJSValueList &args)
{
    if (!_callbacks.contains(type)) return;
    QList<QJSValue> &callbacks = _callbacks[type];

    for (QList<QJSValue>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
        qCDebug(l) << "invoking callback" << type << it->toString();
        QJSValue result = it->call(args);
        if (result.isError()) {
            qCWarning(l) << "error while invoking callback" << type << it->toString() << ":"
                             << JSKitManager::describeError(result);
        }
    }
}

JSKitConsole::JSKitConsole(QObject *parent)
    : QObject(parent), l(metaObject()->className())
{
}

void JSKitConsole::log(const QString &msg)
{
    qCDebug(l) << msg;
}

JSKitLocalStorage::JSKitLocalStorage(const QUuid &uuid, QObject *parent)
    : QObject(parent), _storage(new QSettings(getStorageFileFor(uuid), QSettings::IniFormat, this))
{
    _len = _storage->allKeys().size();
}

int JSKitLocalStorage::length() const
{
    return _len;
}

QJSValue JSKitLocalStorage::getItem(const QString &key) const
{
    QVariant value = _storage->value(key);
    if (value.isValid()) {
        return QJSValue(value.toString());
    } else {
        return QJSValue(QJSValue::NullValue);
    }
}

void JSKitLocalStorage::setItem(const QString &key, const QString &value)
{
    _storage->setValue(key, QVariant::fromValue(value));
    checkLengthChanged();
}

void JSKitLocalStorage::removeItem(const QString &key)
{
    _storage->remove(key);
    checkLengthChanged();
}

void JSKitLocalStorage::clear()
{
    _storage->clear();
    _len = 0;
    emit lengthChanged();
}

void JSKitLocalStorage::checkLengthChanged()
{
    int curLen = _storage->allKeys().size();
    if (_len != curLen) {
        _len = curLen;
        emit lengthChanged();
    }
}

QString JSKitLocalStorage::getStorageFileFor(const QUuid &uuid)
{
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    dataDir.mkpath("js-storage");
    QString fileName = uuid.toString();
    fileName.remove('{');
    fileName.remove('}');
    return dataDir.absoluteFilePath("js-storage/" + fileName + ".ini");
}

JSKitXMLHttpRequest::JSKitXMLHttpRequest(JSKitManager *mgr, QObject *parent)
    : QObject(parent), l(metaObject()->className()), _mgr(mgr),
      _net(new QNetworkAccessManager(this)), _timeout(0), _reply(0)
{
    qCDebug(l) << "constructed";
    connect(_net, &QNetworkAccessManager::authenticationRequired,
            this, &JSKitXMLHttpRequest::handleAuthenticationRequired);
}

JSKitXMLHttpRequest::~JSKitXMLHttpRequest()
{
    qCDebug(l) << "destructed";
}

void JSKitXMLHttpRequest::open(const QString &method, const QString &url, bool async, const QString &username, const QString &password)
{
    if (_reply) {
        _reply->deleteLater();
        _reply = 0;
    }

    _username = username;
    _password = password;
    _request = QNetworkRequest(QUrl(url));
    _verb = method;
    Q_UNUSED(async);

    qCDebug(l) << "opened to URL" << _request.url().toString();
}

void JSKitXMLHttpRequest::setRequestHeader(const QString &header, const QString &value)
{
    qCDebug(l) << "setRequestHeader" << header << value;
    _request.setRawHeader(header.toLatin1(), value.toLatin1());
}

void JSKitXMLHttpRequest::send(const QJSValue &data)
{
    QByteArray byteData;

    if (data.isUndefined() || data.isNull()) {
        // Do nothing, byteData is empty.
    } else if (data.isString()) {
        byteData == data.toString().toUtf8();
    } else if (data.isObject()) {
        if (data.hasProperty("byteLength")) {
            // Looks like an ArrayView or an ArrayBufferView!
            QJSValue buffer = data.property("buffer");
            if (buffer.isUndefined()) {
                // We must assume we've been passed an ArrayBuffer directly
                buffer = data;
            }

            QJSValue array = data.property("_bytes");
            int byteLength = data.property("byteLength").toInt();

            if (array.isArray()) {
                byteData.reserve(byteLength);

                for (int i = 0; i < byteLength; i++) {
                    byteData.append(array.property(i).toInt());
                }

                qCDebug(l) << "passed an ArrayBufferView of" << byteData.length() << "bytes";
            } else {
                qCWarning(l) << "passed an unknown/invalid ArrayBuffer" << data.toString();
            }
        } else {
            qCWarning(l) << "passed an unknown object" << data.toString();
        }

    }

    QBuffer *buffer;
    if (!byteData.isEmpty()) {
        buffer = new QBuffer;
        buffer->setData(byteData);
    } else {
        buffer = 0;
    }

    qCDebug(l) << "sending" << _verb << "to" << _request.url() << "with" << QString::fromUtf8(byteData);
    _reply = _net->sendCustomRequest(_request, _verb.toLatin1(), buffer);

    connect(_reply, &QNetworkReply::finished,
            this, &JSKitXMLHttpRequest::handleReplyFinished);
    connect(_reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &JSKitXMLHttpRequest::handleReplyError);

    if (buffer) {
        // So that it gets deleted alongside the reply object.
        buffer->setParent(_reply);
    }
}

void JSKitXMLHttpRequest::abort()
{
    if (_reply) {
        _reply->deleteLater();
        _reply = 0;
    }
}

QJSValue JSKitXMLHttpRequest::onload() const
{
    return _onload;
}

void JSKitXMLHttpRequest::setOnload(const QJSValue &value)
{
    _onload = value;
}

QJSValue JSKitXMLHttpRequest::ontimeout() const
{
    return _ontimeout;
}

void JSKitXMLHttpRequest::setOntimeout(const QJSValue &value)
{
    _ontimeout = value;
}

QJSValue JSKitXMLHttpRequest::onerror() const
{
    return _onerror;
}

void JSKitXMLHttpRequest::setOnerror(const QJSValue &value)
{
    _onerror = value;
}

uint JSKitXMLHttpRequest::readyState() const
{
    if (!_reply) {
        return UNSENT;
    } else if (_reply->isFinished()) {
        return DONE;
    } else {
        return LOADING;
    }
}

uint JSKitXMLHttpRequest::timeout() const
{
    return _timeout;
}

void JSKitXMLHttpRequest::setTimeout(uint value)
{
    _timeout = value;
    // TODO Handle fetch in-progress.
}

uint JSKitXMLHttpRequest::status() const
{
    if (!_reply || !_reply->isFinished()) {
        return 0;
    } else {
        return _reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();
    }
}

QString JSKitXMLHttpRequest::statusText() const
{
    if (!_reply || !_reply->isFinished()) {
        return QString();
    } else {
        return _reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    }
}

QString JSKitXMLHttpRequest::responseType() const
{
    return _responseType;
}

void JSKitXMLHttpRequest::setResponseType(const QString &type)
{
    qCDebug(l) << "response type set to" << type;
    _responseType = type;
}

QJSValue JSKitXMLHttpRequest::response() const
{
    QJSEngine *engine = _mgr->engine();
    if (_responseType.isEmpty() || _responseType == "text") {
        return engine->toScriptValue(QString::fromUtf8(_response));
    } else if (_responseType == "arraybuffer") {
        QJSValue arrayBufferProto = engine->globalObject().property("ArrayBuffer").property("prototype");
        QJSValue arrayBuf = engine->newObject();
        if (!arrayBufferProto.isUndefined()) {
            arrayBuf.setPrototype(arrayBufferProto);
            arrayBuf.setProperty("byteLength", engine->toScriptValue<uint>(_response.size()));
            QJSValue array = engine->newArray(_response.size());
            for (int i = 0; i < _response.size(); i++) {
                array.setProperty(i, engine->toScriptValue<int>(_response[i]));
            }
            arrayBuf.setProperty("_bytes", array);
            qCDebug(l) << "returning ArrayBuffer of" << _response.size() << "bytes";
        } else {
            qCWarning(l) << "Cannot find proto of ArrayBuffer";
        }
        return arrayBuf;
    } else {
        qCWarning(l) << "unsupported responseType:" << _responseType;
        return engine->toScriptValue<void*>(0);
    }
}

QString JSKitXMLHttpRequest::responseText() const
{
    return QString::fromUtf8(_response);
}

void JSKitXMLHttpRequest::handleReplyFinished()
{
    if (!_reply) {
        qCDebug(l) << "reply finished too late";
        return;
    }

    _response = _reply->readAll();
    qCDebug(l) << "reply finished, reply text:" << QString::fromUtf8(_response);

    emit readyStateChanged();
    emit statusChanged();
    emit statusTextChanged();
    emit responseChanged();
    emit responseTextChanged();

    if (_onload.isCallable()) {
        qCDebug(l) << "going to call onload handler:" << _onload.toString();
        QJSValue result = _onload.callWithInstance(_mgr->engine()->newQObject(this));
        if (result.isError()) {
            qCWarning(l) << "JS error on onload handler:" << JSKitManager::describeError(result);
        }
    } else {
        qCDebug(l) << "No onload set";
    }
}

void JSKitXMLHttpRequest::handleReplyError(QNetworkReply::NetworkError code)
{
    if (!_reply) {
        qCDebug(l) << "reply error too late";
        return;
    }

    qCDebug(l) << "reply error" << code;

    emit readyStateChanged();
    emit statusChanged();
    emit statusTextChanged();

    if (_onerror.isCallable()) {
        qCDebug(l) << "going to call onerror handler:" << _onload.toString();
        QJSValue result = _onerror.callWithInstance(_mgr->engine()->newQObject(this));
        if (result.isError()) {
            qCWarning(l) << "JS error on onerror handler:" << JSKitManager::describeError(result);
        }
    }
}

void JSKitXMLHttpRequest::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    if (_reply == reply) {
        qCDebug(l) << "authentication required";

        if (!_username.isEmpty() || !_password.isEmpty()) {
            qCDebug(l) << "using provided authorization:" << _username;

            auth->setUser(_username);
            auth->setPassword(_password);
        } else {
            qCDebug(l) << "no username or password provided";
        }
    }
}

JSKitGeolocation::JSKitGeolocation(JSKitManager *mgr, QObject *parent)
    : QObject(parent), l(metaObject()->className()),
      _mgr(mgr), _source(0), _lastWatchId(0)
{
}

void JSKitGeolocation::getCurrentPosition(const QJSValue &successCallback, const QJSValue &errorCallback, const QVariantMap &options)
{
    setupWatcher(successCallback, errorCallback, options, true);
}

int JSKitGeolocation::watchPosition(const QJSValue &successCallback, const QJSValue &errorCallback, const QVariantMap &options)
{
    return setupWatcher(successCallback, errorCallback, options, false);
}

void JSKitGeolocation::clearWatch(int watchId)
{
    removeWatcher(watchId);
}

void JSKitGeolocation::handleError(QGeoPositionInfoSource::Error error)
{
    qCWarning(l) << "positioning error: " << error;
    // TODO
}

void JSKitGeolocation::handlePosition(const QGeoPositionInfo &pos)
{
    qCDebug(l) << "got position at" << pos.timestamp() << "type" << pos.coordinate().type();

    if (_watches.empty()) {
        qCWarning(l) << "got position update but no one is watching";
        _source->stopUpdates(); // Just in case.
        return;
    }

    QJSValue obj = buildPositionObject(pos);

    for (auto it = _watches.begin(); it != _watches.end(); /*no adv*/) {
        invokeCallback(it->successCallback, obj);

        if (it->once) {
            it = _watches.erase(it);
        } else {
            it->timer.restart();
            ++it;
        }
    }
}

void JSKitGeolocation::handleTimeout()
{
    qCDebug(l) << "positioning timeout";

    if (_watches.empty()) {
        qCWarning(l) << "got position timeout but no one is watching";
        _source->stopUpdates();
        return;
    }

    QJSValue obj = buildPositionErrorObject(TIMEOUT, "timeout");

    for (auto it = _watches.begin(); it != _watches.end(); /*no adv*/) {
        if (it->timer.hasExpired(it->timeout)) {
            qCDebug(l) << "positioning timeout for watch" << it->watchId
                             << ", watch is" << it->timer.elapsed() << "ms old, timeout is" << it->timeout;
            invokeCallback(it->errorCallback, obj);

            if (it->once) {
                it = _watches.erase(it);
            } else {
                it->timer.restart();
                ++it;
            }
        } else {
            ++it;
        }
    }

    QMetaObject::invokeMethod(this, "updateTimeouts", Qt::QueuedConnection);
}

void JSKitGeolocation::updateTimeouts()
{
    int once_timeout = -1, updates_timeout = -1;

    qCDebug(l) << Q_FUNC_INFO;

    Q_FOREACH(const Watcher &watcher, _watches) {
        qint64 rem_timeout = watcher.timeout - watcher.timer.elapsed();
        qCDebug(l) << "watch" << watcher.watchId << "rem timeout" << rem_timeout;
        if (rem_timeout >= 0) {
            // In case it is too large...
            rem_timeout = qMin<qint64>(rem_timeout, std::numeric_limits<int>::max());
            if (watcher.once) {
                once_timeout = once_timeout >= 0 ? qMin<int>(once_timeout, rem_timeout) : rem_timeout;
            } else {
                updates_timeout = updates_timeout >= 0 ? qMin<int>(updates_timeout, rem_timeout) : rem_timeout;
            }
        }
    }

    if (updates_timeout >= 0) {
        qCDebug(l) << "setting location update interval to" << updates_timeout;
        _source->setUpdateInterval(updates_timeout);
        _source->startUpdates();
    } else {
        qCDebug(l) << "stopping updates";
        _source->stopUpdates();
    }

    if (once_timeout >= 0) {
        qCDebug(l) << "requesting single location update with timeout" << once_timeout;
        _source->requestUpdate(once_timeout);
    }
}

int JSKitGeolocation::setupWatcher(const QJSValue &successCallback, const QJSValue &errorCallback, const QVariantMap &options, bool once)
{
    Watcher watcher;
    watcher.successCallback = successCallback;
    watcher.errorCallback = errorCallback;
    watcher.highAccuracy = options.value("enableHighAccuracy").toBool();
    watcher.timeout = options.value("timeout", std::numeric_limits<int>::max() - 1).toInt();
    watcher.once = once;
    watcher.watchId = ++_lastWatchId;

    qlonglong maximumAge = options.value("maximumAge", 0).toLongLong();

    qCDebug(l) << "setting up watcher, gps=" << watcher.highAccuracy << "timeout=" << watcher.timeout << "maximumAge=" << maximumAge << "once=" << once;

    if (!_source) {
        _source = QGeoPositionInfoSource::createDefaultSource(this);
        connect(_source, static_cast<void (QGeoPositionInfoSource::*)(QGeoPositionInfoSource::Error)>(&QGeoPositionInfoSource::error),
                this, &JSKitGeolocation::handleError);
        connect(_source, &QGeoPositionInfoSource::positionUpdated,
                this, &JSKitGeolocation::handlePosition);
        connect(_source, &QGeoPositionInfoSource::updateTimeout,
                this, &JSKitGeolocation::handleTimeout);
    }

    if (maximumAge > 0) {
        QDateTime threshold = QDateTime::currentDateTime().addMSecs(-qint64(maximumAge));
        QGeoPositionInfo pos = _source->lastKnownPosition(watcher.highAccuracy);
        qCDebug(l) << "got pos timestamp" << pos.timestamp() << " but we want" << threshold;
        if (pos.isValid() && pos.timestamp() >= threshold) {
            invokeCallback(watcher.successCallback, buildPositionObject(pos));
            if (once) {
                return -1;
            }
        } else if (watcher.timeout == 0 && once) {
            // If the timeout has already expired, and we have no cached data
            // Do not even bother to turn on the GPS; return error object now.
            invokeCallback(watcher.errorCallback, buildPositionErrorObject(TIMEOUT, "no cached position"));
            return -1;
        }
    }

    watcher.timer.start();
    _watches.append(watcher);

    qCDebug(l) << "added new watch" << watcher.watchId;

    QMetaObject::invokeMethod(this, "updateTimeouts", Qt::QueuedConnection);

    return watcher.watchId;
}

void JSKitGeolocation::removeWatcher(int watchId)
{
    Watcher watcher;

    qCDebug(l) << "removing watchId" << watcher.watchId;

    for (int i = 0; i < _watches.size(); i++) {
        if (_watches[i].watchId == watchId) {
            watcher = _watches.takeAt(i);
            break;
        }
    }

    if (watcher.watchId != watchId) {
        qCWarning(l) << "watchId not found";
        return;
    }

    QMetaObject::invokeMethod(this, "updateTimeouts", Qt::QueuedConnection);
}

QJSValue JSKitGeolocation::buildPositionObject(const QGeoPositionInfo &pos)
{
    QJSEngine *engine = _mgr->engine();
    QJSValue obj = engine->newObject();
    QJSValue coords = engine->newObject();
    QJSValue timestamp = engine->toScriptValue<quint64>(pos.timestamp().toMSecsSinceEpoch());

    coords.setProperty("latitude", engine->toScriptValue(pos.coordinate().latitude()));
    coords.setProperty("longitude", engine->toScriptValue(pos.coordinate().longitude()));
    if (pos.coordinate().type() == QGeoCoordinate::Coordinate3D) {
        coords.setProperty("altitude", engine->toScriptValue(pos.coordinate().altitude()));
    } else {
        coords.setProperty("altitude", engine->toScriptValue<void*>(0));
    }

    coords.setProperty("accuracy", engine->toScriptValue(pos.attribute(QGeoPositionInfo::HorizontalAccuracy)));

    if (pos.hasAttribute(QGeoPositionInfo::VerticalAccuracy)) {
        coords.setProperty("altitudeAccuracy", engine->toScriptValue(pos.attribute(QGeoPositionInfo::VerticalAccuracy)));
    } else {
        coords.setProperty("altitudeAccuracy", engine->toScriptValue<void*>(0));
    }

    if (pos.hasAttribute(QGeoPositionInfo::Direction)) {
        coords.setProperty("heading", engine->toScriptValue(pos.attribute(QGeoPositionInfo::Direction)));
    } else {
        coords.setProperty("heading", engine->toScriptValue<void*>(0));
    }

    if (pos.hasAttribute(QGeoPositionInfo::GroundSpeed)) {
        coords.setProperty("speed", engine->toScriptValue(pos.attribute(QGeoPositionInfo::GroundSpeed)));
    } else {
        coords.setProperty("speed", engine->toScriptValue<void*>(0));
    }

    obj.setProperty("coords", coords);
    obj.setProperty("timestamp", timestamp);

    return obj;
}

QJSValue JSKitGeolocation::buildPositionErrorObject(PositionError error, const QString &message)
{
    QJSEngine *engine = _mgr->engine();
    QJSValue obj = engine->newObject();

    obj.setProperty("code", engine->toScriptValue<unsigned short>(error));
    obj.setProperty("message", engine->toScriptValue(message));

    return obj;
}

void JSKitGeolocation::invokeCallback(QJSValue callback, QJSValue event)
{
    if (callback.isCallable()) {
        qCDebug(l) << "invoking callback" << callback.toString();
        QJSValue result = callback.call(QJSValueList({event}));
        if (result.isError()) {
            qCWarning(l) << "while invoking callback: " << JSKitManager::describeError(result);
        }
    } else {
        qCWarning(l) << "callback is not callable";
    }
}
