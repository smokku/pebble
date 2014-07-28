TARGET = pebbled

CONFIG += console
CONFIG += link_pkgconfig
QT -= gui

QT += bluetooth dbus contacts
PKGCONFIG += mlite5
QMAKE_CXXFLAGS += -std=c++0x

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
    watchcommands.cpp

HEADERS += \
    manager.h \
    voicecallmanager.h \
    voicecallhandler.h \
    notificationmanager.h \
    watchconnector.h \
    dbusconnector.h \
    dbusadaptor.h \
    watchcommands.h \
    settings.h

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
