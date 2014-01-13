TARGET = waterwatch

CONFIG += sailfishapp

SOURCES += src/waterwatch.cpp \
    src/watchconnector.cpp

QT += bluetooth
QMAKE_CXXFLAGS += -std=c++0x

OTHER_FILES += qml/waterwatch.qml \
    qml/cover/CoverPage.qml \
    rpm/waterwatch.spec \
    rpm/waterwatch.yaml \
    waterwatch.desktop \
    qml/pages/WatchPage.qml

HEADERS += \
    src/watchconnector.h

