TARGET = pebbled

CONFIG += console
CONFIG += link_pkgconfig
QT -= gui

QT += bluetooth dbus contacts
PKGCONFIG += commhistory-qt5 mlite5
QMAKE_CXXFLAGS += -std=c++0x

LIBS += -L$$OUT_PWD/../ext/Log4Qt/ -llog4qt
QMAKE_RPATHDIR += /usr/share/pebble/lib
INCLUDEPATH += ../ext/Log4Qt/src ../ext/Log4Qt/deploy/include

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

OTHER_FILES += \
    org.pebbled.xml \
    ../log4qt-debug.conf \
    ../log4qt-release.conf

INSTALLS += target pebbled confile lib

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

lib.files += $$OUT_PWD/../ext/Log4Qt/*.s*
lib.path = /usr/share/pebble/lib

# unnecesary includes, just so QtCreator could find headers... :-(
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/commhistory-qt5
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/mlite5
