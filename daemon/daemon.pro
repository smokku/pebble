TARGET = pebbled

CONFIG += console
CONFIG += link_pkgconfig
QT -= gui

QT += bluetooth dbus contacts gui qml
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
    dbusadaptor.cpp \
    appmanager.cpp \
    musicmanager.cpp \
    datalogmanager.cpp \
    unpacker.cpp \
    appmsgmanager.cpp \
    jskitmanager.cpp \
    appinfo.cpp \
    jskitobjects.cpp

HEADERS += \
    manager.h \
    voicecallmanager.h \
    voicecallhandler.h \
    notificationmanager.h \
    watchconnector.h \
    dbusconnector.h \
    dbusadaptor.h \
    settings.h \
    appmanager.h \
    musicmanager.h \
    unpacker.h \
    datalogmanager.h \
    appmsgmanager.h \
    jskitmanager.h \
    appinfo.h \
    jskitobjects.h

OTHER_FILES += \
    org.pebbled.xml \
    ../log4qt-debug.conf \
    ../log4qt-release.conf

INSTALLS += target pebbled confile

target.path = /usr/bin

pebbled.files = $${TARGET}.service
pebbled.path = /usr/lib/systemd/user

CONFIG(debug, debug|release) {
    message(Debug build)
    confile.extra = cp $$PWD/../log4qt-debug.conf $$OUT_PWD/../log4qt.conf
}
else {
    message(Release build)
    confile.extra = cp $$PWD/../log4qt-release.conf $$OUT_PWD/../log4qt.conf
}

confile.files = $$OUT_PWD/../log4qt.conf
confile.path = /usr/share/pebble

# unnecesary includes, just so QtCreator could find headers... :-(
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/mlite5
