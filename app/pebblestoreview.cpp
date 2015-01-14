#include "pebblestoreview.h"
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

PebbleStoreView::PebbleStoreView()
    : QQuickWebView()
{
    connect(this, SIGNAL(navigationRequested(QWebNavigationRequest*)), this, SLOT(onNavigationRequested(QWebNavigationRequest*)));
}


void PebbleStoreView::onNavigationRequested(QWebNavigationRequest* request)
{
    if (request->url().scheme() == "pebble") {
        if (request->url().host() == "login") {
            QUrlQuery *accessTokenFragment = new QUrlQuery(request->url().fragment());
            qDebug()<<"login"<<accessTokenFragment->queryItemValue("access_token");
            emit loginSuccess(accessTokenFragment->queryItemValue("access_token"));
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
                    emit downloadPebbleApp(data.value("title").toString(), data.value("pbw_file").toString());
                }
            }
        }
    }
}




