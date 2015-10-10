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
    Q_PROPERTY(QString hardwarePlatform READ hardwarePlatform WRITE setHardwarePlatform NOTIFY hardwarePlatformChanged)

    bool loggedin();
    bool downloadInProgress();
    QString accessToken() const;
    void setAccessToken(const QString &accessToken);
    QString hardwarePlatform() const;
    void setHardwarePlatform(const QString &hardwarePlatform);

public slots:
    void gotoWatchFaces();
    void gotoWatchApps();
    void searchQuery(QString query);
    void logout();

private slots:
    void onNavigationRequested(QWebNavigationRequest* request);
    void onNetworkReplyFinished(QNetworkReply* reply);

signals:
    void accessTokenChanged(const QString & accessToken);
    void hardwarePlatformChanged(const QString & hardwarePlatform);
    void downloadPebbleApp(const QString & downloadTitle, const QString & downloadUrl);
    void downloadInProgressChanged();
    void titleChanged(const QString & title);

private:
    QNetworkAccessManager* m_networkManager;
    QUrl m_configUrl;
    QString m_accessToken;
    QString m_hardwarePlatform;
    QJsonObject downloadObject;
    QJsonObject storeConfigObject;
    bool m_downloadInProgress;

    QUrl prepareUrl(QString baseUrl);
    void fetchConfig();
    void fetchData(QUrl url);
    void addToLocker(QJsonObject data);
    void removeFromLocker(QJsonObject data);
    void showLocker();
    void showMe();
};

#endif // PEBBLESTOREVIEW_H
