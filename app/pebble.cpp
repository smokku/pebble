#include <QtQuick>
#include <QQmlPropertyMap>

#include <sailfishapp.h>
#include "pebbledinterface.h"
#include "pebbleappiconprovider.h"
#include "pebblefirmware.h"
#include "pebblestoreview.h"

const char DONATE_URL[] = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=MAGN86VCARBSA";

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationName("pebble");
    app->setOrganizationName("");

    for (int i = 1; i < argc; i++) {
        PebbledInterface::registerAppFile(argv[i]);
    }

    qmlRegisterUncreatableType<PebbledInterface>("org.pebbled", 0, 1, "PebbledInterface",
                                                 "Please use pebbled context property");

    qmlRegisterType<PebbleStoreView>("org.pebbled", 0, 1, "PebbleStoreView");

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    QScopedPointer<PebbledInterface> pebbled(new PebbledInterface);
    QScopedPointer<PebbleAppIconProvider> appicons(new PebbleAppIconProvider(pebbled.data()));
    QScopedPointer<PebbleFirmware> firmware(new PebbleFirmware);
    QScopedPointer<QQmlPropertyMap> donate(new QQmlPropertyMap);

    donate->insert("url", QString(DONATE_URL));

    view->rootContext()->setContextProperty("APP_VERSION", APP_VERSION);
    view->rootContext()->setContextProperty("pebbled", pebbled.data());
    view->rootContext()->setContextProperty("pebbleFirmware", firmware.data());
    view->rootContext()->setContextProperty("donate", donate.data());
    view->engine()->addImageProvider("pebble-app-icon", appicons.data());
    view->setSource(SailfishApp::pathTo("qml/pebble.qml"));
    view->show();

    return app->exec();
}

