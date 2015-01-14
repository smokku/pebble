TARGET = pebble

CONFIG += sailfishapp

QT += dbus webkit quick-private webkit-private
CONFIG += c++11

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    pebble.cpp \
    pebbledinterface.cpp \
    pebbleappiconprovider.cpp \
    pebblestoreview.cpp

HEADERS += \
    pebbledinterface.h \
    pebbleappiconprovider.h \
    pebblestoreview.h

DBUS_INTERFACES += ../org.pebbled.Watch.xml

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/ManagerPage.qml \
    qml/pages/WatchPage.qml \
    qml/pages/AboutPage.qml \
    qml/pages/InstallAppDialog.qml \
    qml/pages/AppConfigDialog.qml \
    qml/pages/WebItemSelDialog.qml \
    qml/pebble.qml \
    qml/images/* \
    translations/*.ts \
    pebble.desktop \
    pebble.png \
    qml/pages/AppStorePage.qml

CONFIG += sailfishapp_i18n
TRANSLATIONS += translations/pebble-es.ts
