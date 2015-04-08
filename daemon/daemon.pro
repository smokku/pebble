TARGET = pebbled

CONFIG += console
CONFIG += link_pkgconfig

QT += core gui qml bluetooth dbus contacts positioning
PKGCONFIG += mlite5 icu-i18n zlib
CONFIG += c++11

DEFINES += APP_VERSION=\\\"$$VERSION\\\"

SOURCES += \
    daemon.cpp \
    manager.cpp \
    voicecallmanager.cpp \
    voicecallhandler.cpp \
    notificationmanager.cpp \
    watchconnector.cpp \
    dbusconnector.cpp \
    appmanager.cpp \
    musicmanager.cpp \
    datalogmanager.cpp \
    unpacker.cpp \
    appmsgmanager.cpp \
    jskitmanager.cpp \
    appinfo.cpp \
    jskitobjects.cpp \
    packer.cpp \
    bankmanager.cpp \
    uploadmanager.cpp \
    stm32crc.cpp \
    bundle.cpp

HEADERS += \
    manager.h \
    voicecallmanager.h \
    voicecallhandler.h \
    notificationmanager.h \
    watchconnector.h \
    dbusconnector.h \
    settings.h \
    appmanager.h \
    musicmanager.h \
    unpacker.h \
    datalogmanager.h \
    appmsgmanager.h \
    jskitmanager.h \
    appinfo.h \
    jskitobjects.h \
    packer.h \
    bankmanager.h \
    uploadmanager.h \
    stm32crc.h \
    bundle.h

DBUS_ADAPTORS += ../org.pebbled.Watch.xml

OTHER_FILES += js/typedarray.js

DEFINES += QUAZIP_STATIC
include(quazip/quazip.pri)

INSTALLS += target systemd js

target.path = /usr/bin

systemd.files = $${TARGET}.service
systemd.path = /usr/lib/systemd/user

js.files = js/*
js.path = /usr/share/pebble/js

# unnecesary includes, just so QtCreator could find headers... :-(
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/mlite5
