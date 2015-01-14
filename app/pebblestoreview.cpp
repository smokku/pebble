#include "pebblestoreview.h"
#include <QUrlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

PebbleStoreView::PebbleStoreView()
    : QQuickWebView()
{
    connect(this, SIGNAL(navigationRequested(QWebNavigationRequest*)), this, SLOT(onNavigationRequested(QWebNavigationRequest*)));

    this->m_networkManager = new QNetworkAccessManager(this);
    connect(this->m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onNetworkReplyFinished(QNetworkReply*)));

    this->m_configUrl = QUrl("https://boot.getpebble.com/api/config/android/v1/3");
    this->m_downloadInProgress = false;
    emit downloadInProgressChanged();

    //Fetching urls to use by the store
    fetchData(this->m_configUrl);
}

QString PebbleStoreView::accessToken() const
{
    return this->m_accessToken;
}

void PebbleStoreView::setAccessToken(const QString &accessToken)
{
    this->m_accessToken = accessToken;
    emit accessTokenChanged(accessToken);
}

void PebbleStoreView::logout()
{
    setAccessToken("");
    setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("authentication").toString()));
}

bool PebbleStoreView::loggedin()
{
    return (!this->m_accessToken.isEmpty());
}

bool PebbleStoreView::downloadInProgress()
{
    return this->m_downloadInProgress;
}

void PebbleStoreView::gotoWatchFaces()
{
    setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("appstore/watchfaces").toString()));
}

void PebbleStoreView::gotoWatchApps()
{
    setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("appstore/watchapps").toString()));
}

void PebbleStoreView::fetchData(QUrl url)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Cache-Control", "no-cache");
    this->m_networkManager->get(request);
}

void PebbleStoreView::onNetworkReplyFinished(QNetworkReply* reply)
{
    qDebug()<<"Download finished";
    if (reply->request().url() == this->m_configUrl) {
        QJsonDocument jsonResponse = QJsonDocument::fromJson(reply->readAll());
        QJsonObject jsonObject = jsonResponse.object();
        this->storeConfigObject = jsonObject.value("config").toObject();

        if (this->m_accessToken.isEmpty()) {
            setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("authentication").toString()));
        } else {
            setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("onboarding/get_some_apps").toString()));
        }
    } else {
        QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        QFile file(dataDir.absoluteFilePath("apps") + "/" + this->downloadObject.value("uuid").toString() + ".pbw");
        file.open(QIODevice::WriteOnly);
        file.write(reply->readAll());
        file.close();

        qDebug()<<this->downloadObject;

        this->m_downloadInProgress = false;
        emit downloadInProgressChanged();
    }
}

QUrl PebbleStoreView::prepareUrl(QString baseUrl)
{
    baseUrl = baseUrl.replace("$$user_id$$", "ZZZ");
    baseUrl = baseUrl.replace("$$phone_id$$", "XXX");
    baseUrl = baseUrl.replace("$$pebble_id$$", "YYY");
    baseUrl = baseUrl.replace("$$access_token$$", this->m_accessToken);

    qDebug()<<baseUrl;

    return QUrl(baseUrl);
}

void PebbleStoreView::onNavigationRequested(QWebNavigationRequest* request)
{
    if (request->url().scheme() == "pebble") {
        if (request->url().host() == "login") {
            QUrlQuery *accessTokenFragment = new QUrlQuery(request->url().fragment());
            this->m_accessToken = accessTokenFragment->queryItemValue("access_token");
            emit accessTokenChanged(accessTokenFragment->queryItemValue("access_token"));
            setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("onboarding/get_some_apps").toString()));
        }
    }
    if (request->url().scheme() == "pebble-method-call-js-frame") {
        QString urlStr = "";

        //Basic parse error string
        QRegExp reg(".*; source was \"(.*)\";.*");
        reg.setMinimal(true);
        if (reg.indexIn(request->url().errorString()) > -1) {
            urlStr = reg.cap(1);
            reg.setPattern("method=(.*)&args=(.*)$");
            reg.setMinimal(true);
            if (reg.indexIn(urlStr) > -1) {
                QString methodStr = reg.cap(1);
                QString argsStr = QUrl::fromPercentEncoding(reg.cap(2).toUtf8());
                emit call(methodStr, argsStr);
                if (methodStr == "loadAppToDeviceAndLocker") {
                    QJsonDocument jsonResponse = QJsonDocument::fromJson(argsStr.toUtf8());
                    QJsonObject jsonObject = jsonResponse.object();
                    QJsonObject data = jsonObject.value("data").toObject();
                    qDebug()<<"download"<<data.value("title").toString()<<data.value("pbw_file").toString();
                    this->downloadObject = data;;
                    this->m_downloadInProgress = true;
                    emit downloadInProgressChanged();
                    fetchData(QUrl(data.value("pbw_file").toString()));
                    emit downloadPebbleApp(data.value("title").toString(), data.value("pbw_file").toString());
                }
            }
        }
    }
}




