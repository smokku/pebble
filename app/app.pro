TARGET = pebble

CONFIG += sailfishapp

QT += dbus
QMAKE_CXXFLAGS += -std=c++0x

SOURCES += \
    pebble.cpp \
    pebbledinterface.cpp

HEADERS += \
    pebbledinterface.h

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/ManagerPage.qml \
    qml/pages/WatchPage.qml \
    qml/pebble.qml \
    pebble.desktop \
    pebble.png
