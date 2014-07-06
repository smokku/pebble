TARGET = pebbled

CONFIG += console
CONFIG -= app_bundle
CONFIG += link_pkgconfig
QT -= gui

QT += bluetooth dbus contacts
PKGCONFIG += commhistory-qt5 mlite5
QMAKE_CXXFLAGS += -std=c++0x

SOURCES += \
    daemon.cpp \
    manager.cpp \
    voicecallmanager.cpp \
    voicecallhandler.cpp \
    watchconnector.cpp \
    dbusconnector.cpp \
    dbusadaptor.cpp

HEADERS += \
    manager.h \
    voicecallmanager.h \
    voicecallhandler.h \
    watchconnector.h \
    dbusconnector.h \
    dbusadaptor.h

INSTALLS += target pebbled

target.path = /usr/bin

pebbled.files = $${TARGET}.service
pebbled.path = /usr/lib/systemd/user

OTHER_FILES += org.pebbled.xml
