TARGET = pebbled

CONFIG += console
CONFIG -= app_bundle
QT -= gui

INCLUDEPATH += ../lib
LIBS += -L$$OUT_PWD/../lib -lpebble

QT += bluetooth dbus
QMAKE_CXXFLAGS += -std=c++0x

SOURCES += \
    daemon.cpp \
    voicecallmanager.cpp \
    voicecallhandler.cpp \
    manager.cpp

HEADERS += \
    voicecallmanager.h \
    voicecallhandler.h \
    manager.h

INSTALLS += target
target.path = /usr/sbin
