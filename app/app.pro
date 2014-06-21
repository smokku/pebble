TARGET = waterwatch

CONFIG += sailfishapp

SOURCES += waterwatch.cpp

INCLUDEPATH += ../lib
LIBS += -L$$OUT_PWD/../lib -lpebble

QT += bluetooth
QMAKE_CXXFLAGS += -std=c++0x

OTHER_FILES += qml/waterwatch.qml \
    qml/cover/CoverPage.qml \
    waterwatch.desktop \
    qml/pages/WatchPage.qml
