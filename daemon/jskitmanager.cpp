#include <QFile>
#include <QDir>

#include "jskitmanager.h"
#include "jskitobjects.h"

JSKitManager::JSKitManager(WatchConnector *watch, AppManager *apps, AppMsgManager *appmsg, Settings *settings, QObject *parent) :
    QObject(parent), _watch(watch), _apps(apps), _appmsg(appmsg), _settings(settings), _engine(0)
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

QJSEngine * JSKitManager::engine()
{
    return _engine;
}

bool JSKitManager::isJSKitAppRunning() const
{
    return _engine != 0;
}

QString JSKitManager::describeError(QJSValue error)
{
    return QString("%1:%2: %3")
            .arg(error.property("fileName").toString())
            .arg(error.property("lineNumber").toInt())
            .arg(error.toString());
}

void JSKitManager::showConfiguration()
{
    if (_engine) {
        logger()->debug() << "requesting configuration";
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

void JSKitManager::handleAppMessage(const QUuid &uuid, const QVariantMap &msg)
{
    if (_curApp.uuid() == uuid) {
        logger()->debug() << "received a message for the current JSKit app";

        if (!_engine) {
            logger()->debug() << "but engine is stopped";
            return;
        }

        QJSValue eventObj = _engine->newObject();
        eventObj.setProperty("payload", _engine->toScriptValue(msg));

        _jspebble->invokeCallbacks("appmessage", QJSValueList({eventObj}));
    }
}

bool JSKitManager::loadJsFile(const QString &filename)
{
    Q_ASSERT(_engine);

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger()->warn() << "Failed to load JS file:" << file.fileName();
        return false;
    }

    logger()->debug() << "now parsing" << file.fileName();

    QJSValue result = _engine->evaluate(QString::fromUtf8(file.readAll()), file.fileName());
    if (result.isError()) {
        logger()->warn() << "error while evaluating JS script:" << describeError(result);
        return false;
    }

    logger()->debug() << "JS script evaluated";

    return true;
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

    QJSValue navigatorObj = _engine->newObject();
    navigatorObj.setProperty("geolocation", _engine->newQObject(_jsgeo));
    navigatorObj.setProperty("language", _engine->toScriptValue(QLocale().name()));
    globalObj.setProperty("navigator", navigatorObj);

    // Set this.window = this
    globalObj.setProperty("window", globalObj);

    // Shims for compatibility...
    QJSValue result = _engine->evaluate(
                "function XMLHttpRequest() { return Pebble.createXMLHttpRequest(); }\n"
                );
    Q_ASSERT(!result.isError());

    // Polyfills...
    loadJsFile("/usr/share/pebble/js/typedarray.js");

    // Now load the actual script
    loadJsFile(_curApp.path() + "/pebble-js-app.js");

    // Setup the message callback
    QUuid uuid = _curApp.uuid();
    _appmsg->setMessageHandler(uuid, [this, uuid](const QVariantMap &msg) {
        QMetaObject::invokeMethod(this, "handleAppMessage", Qt::QueuedConnection,
                                  Q_ARG(QUuid, uuid),
                                  Q_ARG(QVariantMap, msg));

        // Invoke the slot as a queued connection to give time for the ACK message
        // to go through first.

        return true;
    });

    // We try to invoke the callbacks even if script parsing resulted in error...
    _jspebble->invokeCallbacks("ready");
}

void JSKitManager::stopJsApp()
{
    if (!_engine) return; // Nothing to do!

    logger()->debug() << "stopping JS app";

    if (!_curApp.uuid().isNull()) {
        _appmsg->clearMessageHandler(_curApp.uuid());
    }

    _engine->collectGarbage();

    _engine->deleteLater();
    _engine = 0;
    _jsstorage->deleteLater();
    _jsstorage = 0;
    _jsgeo->deleteLater();
    _jsgeo = 0;
    _jspebble->deleteLater();
    _jspebble = 0;
}
