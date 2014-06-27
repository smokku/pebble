TARGET = pebble

CONFIG += sailfishapp

SOURCES += \
    pebble.cpp

INCLUDEPATH += ../lib
LIBS += -L$$OUT_PWD/../lib -lpebble

QT += bluetooth
QMAKE_CXXFLAGS += -std=c++0x

OTHER_FILES += \
    qml/cover/CoverPage.qml \
    qml/pages/WatchPage.qml \
    qml/pebble.qml \
    pebble.desktop \
    pebble.png
