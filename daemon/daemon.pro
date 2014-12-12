TARGET = pebbled

CONFIG += console
CONFIG += link_pkgconfig
QT -= gui

QT += core bluetooth dbus contacts
PKGCONFIG += mlite5
QMAKE_CXXFLAGS += -std=c++0x

LIBS += -licuuc -licui18n

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
    org.pebbled.xml

INSTALLS += target pebbled

target.path = /usr/bin

pebbled.files = $${TARGET}.service
pebbled.path = /usr/lib/systemd/user

# unnecesary includes, just so QtCreator could find headers... :-(
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/mlite5
