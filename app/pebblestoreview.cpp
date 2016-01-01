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
}

void PebbleStoreView::fetchConfig()
{
    qDebug()<<this->m_hardwarePlatform;

    if (this->m_hardwarePlatform == "aplite") {
        this->m_configUrl = QUrl("https://boot.getpebble.com/api/config/android/v1/3");
    } else {
        this->m_configUrl = QUrl("https://boot.getpebble.com/api/config/android/v3/1?app_version=3.4.0");
    }

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

QString PebbleStoreView::hardwarePlatform() const
{
    return this->m_hardwarePlatform;
}

void PebbleStoreView::setHardwarePlatform(const QString &hardwarePlatform)
{
    this->m_hardwarePlatform = hardwarePlatform;
    emit hardwarePlatformChanged(hardwarePlatform);

    //We need to refetch the config after a platform change
    this->fetchConfig();
}


void PebbleStoreView::logout()
{
    setAccessToken("");
    setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("authentication/sign_in").toString()));
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

void PebbleStoreView::searchQuery(QString query)
{
    QString baseUrl = this->storeConfigObject.value("webviews").toObject().value("appstore/search/query").toString();
    baseUrl = baseUrl.replace("?q", "&query"); //fix wrong param name
    baseUrl = baseUrl.replace("$$query$$", query);
    setUrl(prepareUrl(baseUrl));
}

void PebbleStoreView::fetchData(QUrl url)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Cache-Control", "no-cache");
    this->m_networkManager->get(request);
}

void PebbleStoreView::addToLocker(QJsonObject data)
{
    QUrl url(data.value("links").toObject().value("add").toString());
    QString token("Bearer " + this->accessToken());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Authorization", token.toUtf8());
    this->m_networkManager->post(request, "");
}

void PebbleStoreView::removeFromLocker(QJsonObject data)
{
    QUrl url(data.value("links").toObject().value("remove").toString());
    QString token("Bearer " + this->accessToken());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Authorization", token.toUtf8());
    this->m_networkManager->post(request, "");
}

void PebbleStoreView::showLocker()
{
    QUrl url(this->storeConfigObject.value("links").toObject().value("users/app_locker").toString());
    QString token("Bearer " + this->accessToken());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Authorization", token.toUtf8());
    this->m_networkManager->get(request);
}

void PebbleStoreView::showMe()
{
    QUrl url(this->storeConfigObject.value("links").toObject().value("users/me").toString());
    QString token("Bearer " + this->accessToken());
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Cache-Control", "no-cache");
    request.setRawHeader("Authorization", token.toUtf8());
    this->m_networkManager->get(request);
}

void PebbleStoreView::onNetworkReplyFinished(QNetworkReply* reply)
{
    qDebug()<<"Download finished"<<reply->request().url();

    //Config url
    if (reply->request().url() == this->m_configUrl) {
        QJsonDocument jsonResponse = QJsonDocument::fromJson(reply->readAll());
        QJsonObject jsonObject = jsonResponse.object();
        this->storeConfigObject = jsonObject.value("config").toObject();

        if (this->m_accessToken.isEmpty()) {
            setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("authentication/sign_in").toString()));
        } else {
            setUrl(prepareUrl(this->storeConfigObject.value("webviews").toObject().value("onboarding/get_some_apps").toString()));
        }
    //Add download to locker
    } else if (!this->downloadObject.isEmpty() && reply->request().url() == this->downloadObject.value("links").toObject().value("add").toString()) {
        qDebug()<<reply->readAll();
        this->m_downloadInProgress = false;
        emit downloadInProgressChanged();
    //Remove from locker
    } else if (!this->downloadObject.isEmpty() && reply->request().url() == this->downloadObject.value("links").toObject().value("remove").toString()) {
        qDebug()<<reply->readAll();
    //PBW file
    } else if (!this->downloadObject.isEmpty() && reply->request().url() == this->downloadObject.value("pbw_file").toString()) {
        QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        QFile file(dataDir.absoluteFilePath("apps") + "/" + this->downloadObject.value("uuid").toString() + ".pbw");
        file.open(QIODevice::WriteOnly);
        file.write(reply->readAll());
        file.close();

        qDebug()<<this->downloadObject;
        this->addToLocker(this->downloadObject);
    //Locker
    } else if (reply->request().url() == this->storeConfigObject.value("links").toObject().value("users/app_locker").toString()) {
        qDebug()<<reply->readAll();
    //Me
    } else if (reply->request().url() == this->storeConfigObject.value("links").toObject().value("users/me").toString()) {
        qDebug()<<reply->readAll();
    } else {
        qDebug()<<"Unknown download finished!";
    }
}

QUrl PebbleStoreView::prepareUrl(QString baseUrl)
{
    baseUrl = baseUrl.replace("$$user_id$$", "ZZZ");
    baseUrl = baseUrl.replace("$$phone_id$$", "XXX"); //Unique phone id
    baseUrl = baseUrl.replace("$$pebble_id$$", "YYY"); //official APP puts serial here
    baseUrl = baseUrl.replace("$$pebble_color$$", "64");
    baseUrl = baseUrl.replace("$$hardware$$", this->m_hardwarePlatform);
    baseUrl = baseUrl.replace("$$access_token$$", this->m_accessToken);
    baseUrl = baseUrl.replace("$$extras$$", "");

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
                QJsonDocument jsonResponse = QJsonDocument::fromJson(argsStr.toUtf8());
                QJsonObject jsonObject = jsonResponse.object();
                qDebug()<<"Call"<<methodStr<<jsonObject;
                if (methodStr == "loadAppToDeviceAndLocker") {
                    QJsonObject data = jsonObject.value("data").toObject();
                    qDebug()<<"download"<<data.value("title").toString()<<data.value("pbw_file").toString();
                    this->downloadObject = data;;
                    this->m_downloadInProgress = true;
                    emit downloadInProgressChanged();
                    fetchData(QUrl(data.value("pbw_file").toString()));
                    emit downloadPebbleApp(data.value("title").toString(), data.value("pbw_file").toString());
                } else if (methodStr == "setNavBarTitle") {
                    QJsonObject data = jsonObject.value("data").toObject();
                    emit titleChanged(data.value("title").toString());
                }
            }
        }
    }
}




