TEMPLATE = lib
TARGET = pebble

HEADERS += \
    watchconnector.h

SOURCES += \
    watchconnector.cpp

QT += bluetooth
QMAKE_CXXFLAGS += -std=c++0x

INSTALLS += target
target.path = /usr/lib
