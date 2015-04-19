#ifndef PEBBLESTOREVIEW_H
#define PEBBLESTOREVIEW_H

#include <private/qquickwebview_p.h>
#include <private/qwebnavigationrequest_p.h>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>

class PebbleStoreView : public QQuickWebView
{
    Q_OBJECT
public:
    PebbleStoreView();
    Q_PROPERTY(bool loggedin READ loggedin NOTIFY accessTokenChanged)
    Q_PROPERTY(bool downloadInProgress READ downloadInProgress NOTIFY downloadInProgressChanged)
    Q_PROPERTY(QString accessToken READ accessToken WRITE setAccessToken NOTIFY accessTokenChanged)

    bool loggedin();
    bool downloadInProgress();
    QString accessToken() const;
    void setAccessToken(const QString &accessToken);

public slots:
    void gotoWatchFaces();
    void gotoWatchApps();
    void logout();

private slots:
    void onNavigationRequested(QWebNavigationRequest* request);
    void onNetworkReplyFinished(QNetworkReply* reply);

signals:
    void accessTokenChanged(const QString & accessToken);
    void downloadPebbleApp(const QString & downloadTitle, const QString & downloadUrl);
    void downloadInProgressChanged();
    void call(const QString &, const QString &);

private:
    QNetworkAccessManager* m_networkManager;
    QUrl m_configUrl;
    QString m_accessToken;
    QJsonObject downloadObject;
    QJsonObject storeConfigObject;
    bool m_downloadInProgress;

    QUrl prepareUrl(QString baseUrl);
    void fetchData(QUrl url);
    void addToLocker(QJsonObject data);
    void removeFromLocker(QJsonObject data);
    void showLocker();
};

#endif // PEBBLESTOREVIEW_H
