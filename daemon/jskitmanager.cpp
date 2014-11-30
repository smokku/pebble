#include <QQmlEngine>
#include <QJSValueIterator>
#include "jskitmanager.h"
#include "jskitmanager_p.h"

JSKitPebble::JSKitPebble(JSKitManager *mgr)
    : QObject(mgr)
{
}

JSKitPebble::~JSKitPebble()
{
}

JSKitManager::JSKitManager(AppManager *apps, AppMsgManager *appmsg, QObject *parent) :
    QObject(parent), _apps(apps), _appmsg(appmsg), _engine(0)
{
    connect(_appmsg, &AppMsgManager::appStarted, this, &JSKitManager::handleAppStarted);
    connect(_appmsg, &AppMsgManager::appStopped, this, &JSKitManager::handleAppStopped);
}

JSKitManager::~JSKitManager()
{
    if (_engine) {
        stopJsApp();
    }
}

void JSKitManager::handleAppStarted(const QUuid &uuid)
{
    const auto &info = _apps->info(uuid);
    if (!info.uuid.isNull() && info.isJSKit) {
        logger()->debug() << "Preparing to start JSKit app" << info.uuid << info.shortName;
        _curApp = info;
        startJsApp();
    }
}

void JSKitManager::handleAppStopped(const QUuid &uuid)
{
    if (!_curApp.uuid.isNull()) {
        if (_curApp.uuid != uuid) {
            logger()->warn() << "Closed app with invalid UUID";
        }

        _curApp = AppManager::AppInfo();
    }
}

void JSKitManager::startJsApp()
{
    if (_engine) stopJsApp();
    _engine = new QJSEngine(this);
    _jspebble = new JSKitPebble(this);

    QJSValue globalObj = _engine->globalObject();

    globalObj.setProperty("Pebble", _engine->newQObject(_jspebble));

    QJSValueIterator it(globalObj);
    while (it.hasNext()) {
        it.next();
        logger()->debug() << "JS property:" << it.name();
    }
}

void JSKitManager::stopJsApp()
{
    if (!_engine) return; // Nothing to do!

    _engine->collectGarbage();

    delete _engine;
    _engine = 0;
    delete _jspebble;
    _jspebble = 0;
}
