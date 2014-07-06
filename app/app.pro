TARGET = pebble

CONFIG += sailfishapp

SOURCES += \
    pebble.cpp \
    pebbledinterface.cpp

HEADERS += \
    pebbledinterface.h

QT += dbus
QMAKE_CXXFLAGS += -std=c++0x

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/WatchPage.qml \
    qml/pebble.qml \
    pebble.desktop \
    pebble.png
