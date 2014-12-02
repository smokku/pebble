#ifndef JSKITMANAGER_H
#define JSKITMANAGER_H

#include <QJSEngine>
#include "appmanager.h"
#include "appmsgmanager.h"

class JSKitPebble;
class JSKitConsole;
class JSKitLocalStorage;

class JSKitManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit JSKitManager(AppManager *apps, AppMsgManager *appmsg, QObject *parent = 0);
    ~JSKitManager();

    QJSEngine * engine();
    bool isJSKitAppRunning() const;

signals:
    void appNotification(const QUuid &uuid, const QString &title, const QString &body);
    void appOpenUrl(const QUrl &url);

public slots:
    void showConfiguration();
    void handleWebviewClosed(const QString &result);

private slots:
    void handleAppStarted(const QUuid &uuid);
    void handleAppStopped(const QUuid &uuid);
    void handleAppMessage(const QUuid &uuid, const QVariantMap &data);

private:
    void startJsApp();
    void stopJsApp();

private:
    friend class JSKitPebble;

    AppManager *_apps;
    AppMsgManager *_appmsg;
    AppInfo _curApp;
    QJSEngine *_engine;
    QPointer<JSKitPebble> _jspebble;
    QPointer<JSKitConsole> _jsconsole;
    QPointer<JSKitLocalStorage> _jsstorage;
};

#endif // JSKITMANAGER_H
