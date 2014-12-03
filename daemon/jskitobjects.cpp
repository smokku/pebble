#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QBuffer>
#include <QDir>
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
            QJSValue result = callbackForAck.call();
            if (result.isError()) {
                logger()->warn() << "error while invoking ACK callback" << callbackForAck.toString() << ":"
                                 << result.toString();
            }
        } else {
            logger()->debug() << "Ack callback not callable";
        }
    }, [this, callbackForNack]() mutable {
        if (callbackForNack.isCallable()) {
            logger()->debug() << "Invoking nack callback";
            QJSValue result = callbackForNack.call();
            if (result.isError()) {
                logger()->warn() << "error while invoking NACK callback" << callbackForNack.toString() << ":"
                                 << result.toString();
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

void JSKitPebble::invokeCallbacks(const QString &type, const QJSValueList &args)
{
    if (!_callbacks.contains(type)) return;
    QList<QJSValue> &callbacks = _callbacks[type];

    for (QList<QJSValue>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
        logger()->debug() << "invoking callback" << type << it->toString();
        QJSValue result = it->call(args);
        if (result.isError()) {
            logger()->warn() << "error while invoking callback" << type << it->toString() << ":"
                             << result.toString();
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
            logger()->warn() << "JS error on onload handler:" << result.toString();
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
            logger()->warn() << "JS error on onerror handler:" << result.toString();
        }
    }
}
