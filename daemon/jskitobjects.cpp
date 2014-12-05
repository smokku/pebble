#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QBuffer>
#include <QDir>
#include <limits>
#include "jskitobjects.h"

JSKitPebble::JSKitPebble(const AppInfo &info, JSKitManager *mgr)
    : QObject(mgr), _appInfo(info), _mgr(mgr)
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

void JSKitPebble::sendAppMessage(QJSValue message, QJSValue callbackForAck, QJSValue callbackForNack)
{
    QVariantMap data = message.toVariant().toMap();

    logger()->debug() << "sendAppMessage" << data;

    _mgr->_appmsg->send(_appInfo.uuid(), data, [this, callbackForAck]() mutable {
        if (callbackForAck.isCallable()) {
            logger()->debug() << "Invoking ack callback";
            QJSValue result = callbackForAck.call(QJSValueList({buildAckEventObject()}));
            if (result.isError()) {
                logger()->warn() << "error while invoking ACK callback" << callbackForAck.toString() << ":"
                                 << JSKitManager::describeError(result);
            }
        } else {
            logger()->debug() << "Ack callback not callable";
        }
    }, [this, callbackForNack]() mutable {
        if (callbackForNack.isCallable()) {
            logger()->debug() << "Invoking nack callback";
            QJSValue result = callbackForNack.call(QJSValueList({buildAckEventObject()}));
            if (result.isError()) {
                logger()->warn() << "error while invoking NACK callback" << callbackForNack.toString() << ":"
                                 << JSKitManager::describeError(result);
            }
        } else {
            logger()->debug() << "Nack callback not callable";
        }
    });
}

void JSKitPebble::showSimpleNotificationOnPebble(const QString &title, const QString &body)
{
    logger()->debug() << "showSimpleNotificationOnPebble" << title << body;
    emit _mgr->appNotification(_appInfo.uuid(), title, body);
}

void JSKitPebble::openURL(const QUrl &url)
{
    logger()->debug() << "opening url" << url.toString();
    emit _mgr->appOpenUrl(url);
}

QJSValue JSKitPebble::createXMLHttpRequest()
{
    JSKitXMLHttpRequest *xhr = new JSKitXMLHttpRequest(_mgr, 0);
    // Should be deleted by JS engine.
    return _mgr->engine()->newQObject(xhr);
}

QJSValue JSKitPebble::buildAckEventObject() const
{
    QJSEngine *engine = _mgr->engine();
    QJSValue eventObj = engine->newObject();
    QJSValue dataObj = engine->newObject();

    // Why do scripts need the real transactionId?
    // No idea. Just fake it.
    dataObj.setProperty("transactionId", engine->toScriptValue(0));
    eventObj.setProperty("data", dataObj);

    return eventObj;
}

void JSKitPebble::invokeCallbacks(const QString &type, const QJSValueList &args)
{
    if (!_callbacks.contains(type)) return;
    QList<QJSValue> &callbacks = _callbacks[type];

    for (QList<QJSValue>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
        logger()->debug() << "invoking callback" << type << it->toString();
        QJSValue result = it->call(args);
        if (result.isError()) {
            logger()->warn() << "error while invoking callback" << type << it->toString() << ":"
                             << JSKitManager::describeError(result);
        }
    }
}

JSKitConsole::JSKitConsole(JSKitManager *mgr)
    : QObject(mgr)
{
}

void JSKitConsole::log(const QString &msg)
{
    logger()->info() << msg;
}

JSKitLocalStorage::JSKitLocalStorage(const QUuid &uuid, JSKitManager *mgr)
    : QObject(mgr), _storage(new QSettings(getStorageFileFor(uuid), QSettings::IniFormat, this))
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
    dataDir.mkdir("js-storage");
    QString fileName = uuid.toString();
    fileName.remove('{');
    fileName.remove('}');
    return dataDir.absoluteFilePath("js-storage/" + fileName + ".ini");
}

JSKitXMLHttpRequest::JSKitXMLHttpRequest(JSKitManager *mgr, QObject *parent)
    : QObject(parent), _mgr(mgr),
      _net(new QNetworkAccessManager(this)), _reply(0)
{
    logger()->debug() << "constructed";
}

JSKitXMLHttpRequest::~JSKitXMLHttpRequest()
{
    logger()->debug() << "destructed";
}

void JSKitXMLHttpRequest::open(const QString &method, const QString &url, bool async)
{
    if (_reply) {
        _reply->deleteLater();
        _reply = 0;
    }

    _request = QNetworkRequest(QUrl(url));
    _verb = method;
    Q_UNUSED(async);
}

void JSKitXMLHttpRequest::setRequestHeader(const QString &header, const QString &value)
{
    logger()->debug() << "setRequestHeader" << header << value;
    _request.setRawHeader(header.toLatin1(), value.toLatin1());
}

void JSKitXMLHttpRequest::send(const QString &body)
{
    QBuffer *buffer = new QBuffer;
    buffer->setData(body.toUtf8());
    logger()->debug() << "sending" << _verb << "to" << _request.url() << "with" << body;
    _reply = _net->sendCustomRequest(_request, _verb.toLatin1(), buffer);
    connect(_reply, &QNetworkReply::finished,
            this, &JSKitXMLHttpRequest::handleReplyFinished);
    connect(_reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &JSKitXMLHttpRequest::handleReplyError);
    buffer->setParent(_reply); // So that it gets deleted alongside the reply object.
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

unsigned short JSKitXMLHttpRequest::readyState() const
{
    if (!_reply) {
        return UNSENT;
    } else if (_reply->isFinished()) {
        return DONE;
    } else {
        return LOADING;
    }
}

unsigned short JSKitXMLHttpRequest::status() const
{
    if (!_reply || !_reply->isFinished()) {
        return 0;
    } else {
        return _reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt();
    }
}

QString JSKitXMLHttpRequest::responseText() const
{
    return QString::fromUtf8(_response);
}

void JSKitXMLHttpRequest::handleReplyFinished()
{
    if (!_reply) {
        logger()->info() << "reply finished too late";
        return;
    }

    _response = _reply->readAll();
    logger()->debug() << "reply finished, reply text:" << QString::fromUtf8(_response);

    emit readyStateChanged();
    emit statusChanged();
    emit responseTextChanged();

    if (_onload.isCallable()) {
        logger()->debug() << "going to call onload handler:" << _onload.toString();
        QJSValue result = _onload.callWithInstance(_mgr->engine()->newQObject(this));
        if (result.isError()) {
            logger()->warn() << "JS error on onload handler:" << JSKitManager::describeError(result);
        }
    } else {
        logger()->debug() << "No onload set";
    }
}

void JSKitXMLHttpRequest::handleReplyError(QNetworkReply::NetworkError code)
{
    if (!_reply) {
        logger()->info() << "reply error too late";
        return;
    }

    logger()->info() << "reply error" << code;

    if (_onerror.isCallable()) {
        logger()->debug() << "going to call onerror handler:" << _onload.toString();
        QJSValue result = _onerror.callWithInstance(_mgr->engine()->newQObject(this));
        if (result.isError()) {
            logger()->warn() << "JS error on onerror handler:" << JSKitManager::describeError(result);
        }
    }
}

JSKitGeolocation::JSKitGeolocation(JSKitManager *mgr)
    : QObject(mgr), _mgr(mgr), _source(0), _lastWatchId(0)
{
}

void JSKitGeolocation::getCurrentPosition(const QJSValue &successCallback, const QJSValue &errorCallback, const QVariantMap &options)
{
    logger()->debug() << Q_FUNC_INFO;
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
    logger()->warn() << "positioning error: " << error;
    // TODO
}

void JSKitGeolocation::handlePosition(const QGeoPositionInfo &pos)
{
    if (_watches.empty()) {
        logger()->warn() << "got position update but no one is watching";
    }

    logger()->debug() << "got position at" << pos.timestamp() << "type" << pos.coordinate().type();

    QJSValue obj = buildPositionObject(pos);

    for (auto it = _watches.begin(); it != _watches.end(); /*no adv*/) {
        invokeCallback(it->successCallback, obj);

        if (it->once) {
            it = _watches.erase(it);
        } else {
            ++it;
        }
    }

    if (_watches.empty()) {
        _source->stopUpdates();
    }
}

void JSKitGeolocation::handleTimeout()
{
    logger()->debug() << Q_FUNC_INFO;
    // TODO
}

uint JSKitGeolocation::minimumTimeout() const
{
    uint minimum = std::numeric_limits<uint>::max();
    Q_FOREACH(const Watcher &watcher, _watches) {
        if (!watcher.once) {
            minimum = qMin<uint>(watcher.timeout, minimum);
        }
    }
    return minimum;
}

int JSKitGeolocation::setupWatcher(const QJSValue &successCallback, const QJSValue &errorCallback, const QVariantMap &options, bool once)
{
    Watcher watcher;
    watcher.successCallback = successCallback;
    watcher.errorCallback = errorCallback;
    watcher.highAccuracy = options.value("enableHighAccuracy").toBool();
    watcher.timeout = options.value("timeout", 0xFFFFFFFFU).toUInt();
    watcher.once = once;
    watcher.watchId = ++_lastWatchId;

    uint maximumAge = options.value("maximumAge", 0).toUInt();

    logger()->debug() << "setting up watcher, gps=" << watcher.highAccuracy << "timeout=" << watcher.timeout << "maximumAge=" << maximumAge << "once=" << once;

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
        logger()->debug() << "got pos timestamp" << pos.timestamp() << " but we want" << threshold;
        if (pos.isValid() && pos.timestamp() >= threshold) {
            invokeCallback(watcher.successCallback, buildPositionObject(pos));
            if (once) {
                return -1;
            }
        } else if (watcher.timeout == 0 && once) {
            invokeCallback(watcher.errorCallback, buildPositionErrorObject(TIMEOUT, "no cached position"));
            return -1;
        }
    }

    if (once) {
        _source->requestUpdate(watcher.timeout);
    } else {
        uint timeout = minimumTimeout();
        logger()->debug() << "setting location update interval to" << timeout;
        _source->setUpdateInterval(timeout);
        logger()->debug() << "starting location updates";
        _source->startUpdates();
    }

    _watches.append(watcher);

    logger()->debug() << "added new watch" << watcher.watchId;

    return watcher.watchId;
}

void JSKitGeolocation::removeWatcher(int watchId)
{
    Watcher watcher;

    logger()->debug() << "removing watchId" << watcher.watchId;

    for (int i = 0; i < _watches.size(); i++) {
        if (_watches[i].watchId == watchId) {
            watcher = _watches.takeAt(i);
            break;
        }
    }

    if (watcher.watchId != watchId) {
        logger()->warn() << "watchId not found";
        return;
    }

    if (_watches.empty()) {
        logger()->debug() << "stopping updates";
        _source->stopUpdates();
    } else {
        uint timeout = minimumTimeout();
        logger()->debug() << "setting location update interval to" << timeout;
        _source->setUpdateInterval(timeout);
    }
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
        logger()->debug() << "invoking callback" << callback.toString();
        QJSValue result = callback.call(QJSValueList({event}));
        if (result.isError()) {
            logger()->warn() << "while invoking callback: " << JSKitManager::describeError(result);
        }
    } else {
        logger()->warn() << "callback is not callable";
    }
}
