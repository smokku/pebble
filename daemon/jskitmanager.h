#ifndef JSKITMANAGER_H
#define JSKITMANAGER_H

#include <QJSEngine>
#include "appmanager.h"
#include "appmsgmanager.h"
#include "settings.h"

class JSKitPebble;
class JSKitConsole;
class JSKitLocalStorage;
class JSKitGeolocation;

class JSKitManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit JSKitManager(WatchConnector *watch, AppManager *apps, AppMsgManager *appmsg, Settings *settings, QObject *parent = 0);
    ~JSKitManager();

    QJSEngine * engine();
    bool isJSKitAppRunning() const;

    static QString describeError(QJSValue error);

    void showConfiguration();
    void handleWebviewClosed(const QString &result);

signals:
    void appNotification(const QUuid &uuid, const QString &title, const QString &body);
    void appOpenUrl(const QUrl &url);

private slots:
    void handleAppStarted(const QUuid &uuid);
    void handleAppStopped(const QUuid &uuid);
    void handleAppMessage(const QUuid &uuid, const QVariantMap &msg);

private:
    bool loadJsFile(const QString &filename);
    void startJsApp();
    void stopJsApp();

private:
    friend class JSKitPebble;

    WatchConnector *_watch;
    AppManager *_apps;
    AppMsgManager *_appmsg;
    Settings *_settings;
    AppInfo _curApp;
    QJSEngine *_engine;
    QPointer<JSKitPebble> _jspebble;
    QPointer<JSKitConsole> _jsconsole;
    QPointer<JSKitLocalStorage> _jsstorage;
    QPointer<JSKitGeolocation> _jsgeo;
};

#endif // JSKITMANAGER_H
