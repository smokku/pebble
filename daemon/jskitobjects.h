#ifndef JSKITMANAGER_P_H
#define JSKITMANAGER_P_H

#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "jskitmanager.h"

class JSKitPebble : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit JSKitPebble(const AppInfo &appInfo, JSKitManager *mgr);

    Q_INVOKABLE void addEventListener(const QString &type, QJSValue function);
    Q_INVOKABLE void removeEventListener(const QString &type, QJSValue function);

    Q_INVOKABLE void sendAppMessage(QJSValue message, QJSValue callbackForAck, QJSValue callbackForNack);

    Q_INVOKABLE void showSimpleNotificationOnPebble(const QString &title, const QString &body);

    Q_INVOKABLE void openUrl(const QUrl &url);

    Q_INVOKABLE QJSValue createXMLHttpRequest();

    void invokeCallbacks(const QString &type, const QJSValueList &args = QJSValueList());

private:
    AppInfo _appInfo;
    JSKitManager *_mgr;
    QHash<QString, QList<QJSValue>> _callbacks;
};

class JSKitConsole : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit JSKitConsole(JSKitManager *mgr);

    Q_INVOKABLE void log(const QString &msg);
};

class JSKitLocalStorage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int length READ length NOTIFY lengthChanged)

public:
    explicit JSKitLocalStorage(const QUuid &uuid, JSKitManager *mgr);

    int length() const;

    Q_INVOKABLE QJSValue getItem(const QString &key) const;
    Q_INVOKABLE void setItem(const QString &key, const QString &value);
    Q_INVOKABLE void removeItem(const QString &key);

    Q_INVOKABLE void clear();

signals:
    void lengthChanged();

private:
    void checkLengthChanged();
    static QString getStorageFileFor(const QUuid &uuid);

private:
    QSettings *_storage;
    int _len;
};

class JSKitXMLHttpRequest : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

    Q_PROPERTY(QJSValue onload READ onload WRITE setOnload)
    Q_PROPERTY(unsigned short readyState READ readyState NOTIFY readyStateChanged)
    Q_PROPERTY(unsigned short status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString responseText READ responseText NOTIFY responseTextChanged)

public:
    explicit JSKitXMLHttpRequest(JSKitManager *mgr, QObject *parent = 0);
    ~JSKitXMLHttpRequest();

    enum ReadyStates {
        UNSENT,
        OPENED,
        HEADERS_RECEIVED,
        LOADING,
        DONE
    };

    Q_INVOKABLE void open(const QString &method, const QString &url, bool async);
    Q_INVOKABLE void setRequestHeader(const QString &header, const QString &value);
    Q_INVOKABLE void send(const QString &body);
    Q_INVOKABLE void abort();

    QJSValue onload() const;
    void setOnload(const QJSValue &value);

    unsigned short readyState() const;
    unsigned short status() const;
    QString responseText() const;

signals:
    void readyStateChanged();
    void statusChanged();
    void responseTextChanged();

private slots:
    void handleReplyFinished();

private:
    JSKitManager *_mgr;
    QNetworkAccessManager *_net;
    QString _verb;
    QNetworkRequest _request;
    QNetworkReply *_reply;
    QByteArray _response;
    QJSValue _onload;
};

#endif // JSKITMANAGER_P_H
