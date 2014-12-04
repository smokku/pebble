#include <QFile>
#include <QDir>

#include "jskitmanager.h"
#include "jskitobjects.h"

JSKitManager::JSKitManager(AppManager *apps, AppMsgManager *appmsg, QObject *parent) :
    QObject(parent), _apps(apps), _appmsg(appmsg), _engine(0)
{
    connect(_appmsg, &AppMsgManager::appStarted, this, &JSKitManager::handleAppStarted);
    connect(_appmsg, &AppMsgManager::appStopped, this, &JSKitManager::handleAppStopped);
    connect(_appmsg, &AppMsgManager::messageReceived, this, &JSKitManager::handleAppMessage);
}

JSKitManager::~JSKitManager()
{
    if (_engine) {
        stopJsApp();
    }
}

QJSEngine * JSKitManager::engine()
{
    return _engine;
}

bool JSKitManager::isJSKitAppRunning() const
{
    return _engine != 0;
}

void JSKitManager::showConfiguration()
{
    if (_engine) {
        _jspebble->invokeCallbacks("showConfiguration");
    } else {
        logger()->warn() << "requested to show configuration, but JS engine is not running";
    }
}

void JSKitManager::handleWebviewClosed(const QString &result)
{
    if (_engine) {
        QJSValue eventObj = _engine->newObject();
        eventObj.setProperty("response", _engine->toScriptValue(result));

        logger()->debug() << "webview closed with the following result: " << result;

        _jspebble->invokeCallbacks("webviewclosed", QJSValueList({eventObj}));
    } else {
        logger()->warn() << "webview closed event, but JS engine is not running";
    }
}

void JSKitManager::handleAppStarted(const QUuid &uuid)
{
    AppInfo info = _apps->info(uuid);
    if (!info.uuid().isNull() && info.isJSKit()) {
        logger()->debug() << "Preparing to start JSKit app" << info.uuid() << info.shortName();
        _curApp = info;
        startJsApp();
    }
}

void JSKitManager::handleAppStopped(const QUuid &uuid)
{
    if (!_curApp.uuid().isNull()) {
        if (_curApp.uuid() != uuid) {
            logger()->warn() << "Closed app with invalid UUID";
        }

        stopJsApp();
        _curApp.setUuid(QUuid()); // Clear the uuid to force invalid app
    }
}

void JSKitManager::handleAppMessage(const QUuid &uuid, const QVariantMap &data)
{
    if (_curApp.uuid() == uuid) {
        logger()->debug() << "received a message for the current JSKit app";

        if (!_engine) {
            logger()->debug() << "but engine is stopped";
            return;
        }

        QJSValue eventObj = _engine->newObject();
        eventObj.setProperty("payload", _engine->toScriptValue(data));

        _jspebble->invokeCallbacks("appmessage", QJSValueList({eventObj}));
    }
}

void JSKitManager::startJsApp()
{
    if (_engine) stopJsApp();
    if (_curApp.uuid().isNull()) {
        logger()->warn() << "Attempting to start JS app with invalid UUID";
        return;
    }

    _engine = new QJSEngine(this);
    _jspebble = new JSKitPebble(_curApp, this);
    _jsconsole = new JSKitConsole(this);
    _jsstorage = new JSKitLocalStorage(_curApp.uuid(), this);
    _jsgeo = new JSKitGeolocation(this);

    logger()->debug() << "starting JS app";

    QJSValue globalObj = _engine->globalObject();

    globalObj.setProperty("Pebble", _engine->newQObject(_jspebble));
    globalObj.setProperty("console", _engine->newQObject(_jsconsole));
    globalObj.setProperty("localStorage", _engine->newQObject(_jsstorage));

    QJSValue windowObj = _engine->newObject();
    windowObj.setProperty("localStorage", globalObj.property("localStorage"));
    globalObj.setProperty("window", windowObj);

    QJSValue navigatorObj = _engine->newObject();
    navigatorObj.setProperty("geolocation", _engine->newQObject(_jsgeo));
    globalObj.setProperty("navigator", navigatorObj);

    _engine->evaluate("function XMLHttpRequest() { return Pebble.createXMLHttpRequest(); }");

    QFile scriptFile(_curApp.path() + "/pebble-js-app.js");
    if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger()->warn() << "Failed to open JS file at:" << scriptFile.fileName();
        stopJsApp();
        return;
    }

    QString script = QString::fromUtf8(scriptFile.readAll());

    QJSValue result = _engine->evaluate(script, scriptFile.fileName());
    if (result.isError()) {
        logger()->warn() << "error while evaluating JSKit script:" << result.toString();
    }

    logger()->debug() << "JS script evaluated";

    _jspebble->invokeCallbacks("ready");
}

void JSKitManager::stopJsApp()
{
    if (!_engine) return; // Nothing to do!

    logger()->debug() << "stopping JS app";

    _engine->collectGarbage();

    delete _engine;
    _engine = 0;
    delete _jsstorage;
    _jsstorage = 0;
    delete _jspebble;
    _jspebble = 0;
    delete _jsgeo;
    _jsgeo = 0;
}
