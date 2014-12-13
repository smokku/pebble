TARGET = pebble

CONFIG += sailfishapp

QT += dbus
QMAKE_CXXFLAGS += -std=c++0x

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    pebble.cpp \
    pebbledinterface.cpp

HEADERS += \
    pebbledinterface.h

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
    qml/pages/AppConfigPage.qml \
    qml/pages/InstallAppDialog.qml
