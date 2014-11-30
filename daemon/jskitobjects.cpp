#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include "jskitobjects.h"

JSKitPebble::JSKitPebble(JSKitManager *mgr)
    : QObject(mgr), _mgr(mgr)
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
    // TODO contact _mgr->appmsg->...
    logger()->debug() << "sendAppMessage" << message.toString();
}

void JSKitPebble::showSimpleNotificationOnPebble(const QString &title, const QString &body)
{
    logger()->debug() << "showSimpleNotificationOnPebble" << title << body;
}

void JSKitPebble::openUrl(const QUrl &url)
{
    if (!QDesktopServices::openUrl(url)) {
        logger()->warn() << "Failed to open URL:" << url;
    }
}

void JSKitPebble::invokeCallbacks(const QString &type, const QJSValueList &args)
{
    if (!_callbacks.contains(type)) return;
    QList<QJSValue> &callbacks = _callbacks[type];

    for (QList<QJSValue>::iterator it = callbacks.begin(); it != callbacks.end(); ++it) {
        QJSValue result = it->call(args);
        if (result.isError()) {
            logger()->warn() << "error while invoking callback" << type << it->toString() << ":"
                             << result.toString();
        }
    }
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
    return dataDir.absoluteFilePath("js-storage/" + uuid.toString() + ".ini");
}
