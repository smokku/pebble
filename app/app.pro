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

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/ManagerPage.qml \
    qml/pages/WatchPage.qml \
    qml/pages/AboutPage.qml \
    qml/pebble.qml \
    qml/images/* \
    pebble.desktop \
    pebble.png \
    qml/pages/HarbourPage.qml

INSTALLS += harbour

harbour.path = /usr/share/harbour-$$TARGET
harbour.files = \
    harbour-pebble.desktop \
    harbour-pebble.png
