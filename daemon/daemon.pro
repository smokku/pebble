TARGET = pebbled

CONFIG += console
CONFIG -= app_bundle

INCLUDEPATH += ../lib
LIBS += -L$$OUT_PWD/../lib -lpebble

QT += bluetooth
QMAKE_CXXFLAGS += -std=c++0x

SOURCES += \
    daemon.cpp
