TARGET = pebble

CONFIG += sailfishapp

QT += dbus
CONFIG += c++11

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    pebble.cpp \
    pebbledinterface.cpp \
    pebbleappiconprovider.cpp

HEADERS += \
    pebbledinterface.h \
    pebbleappiconprovider.h

DBUS_INTERFACES += ../org.pebbled.Watch.xml

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/ManagerPage.qml \
    qml/pages/WatchPage.qml \
    qml/pages/AboutPage.qml \
    qml/pebble.qml \
    qml/images/* \
    pebble.desktop \
    pebble.png \
    qml/pages/InstallAppDialog.qml \
    qml/pages/AppConfigDialog.qml
