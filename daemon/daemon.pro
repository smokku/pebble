TARGET = pebbled

CONFIG += console
CONFIG += link_pkgconfig
QT -= gui

QT += bluetooth dbus contacts gui qml positioning
PKGCONFIG += mlite5 icu-i18n
CONFIG += c++11

LIBS += -llog4qt

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
    stm32crc.cpp

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
    stm32crc.h

OTHER_FILES += \
    ../log4qt-debug.conf \
    ../log4qt-release.conf \
    js/typedarray.js

DBUS_ADAPTORS += ../org.pebbled.Watch.xml

INSTALLS += target systemd confile js

target.path = /usr/bin

systemd.files = $${TARGET}.service
systemd.path = /usr/lib/systemd/user

js.files = js/*
js.path = /usr/share/pebble/js

CONFIG(debug, debug|release) {
    confile.extra = cp $$PWD/../log4qt-debug.conf $$OUT_PWD/../log4qt.conf
} else {
    confile.extra = cp $$PWD/../log4qt-release.conf $$OUT_PWD/../log4qt.conf
}

confile.files = $$OUT_PWD/../log4qt.conf
confile.path = /usr/share/pebble

# unnecesary includes, just so QtCreator could find headers... :-(
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/mlite5
