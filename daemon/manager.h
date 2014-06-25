#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "dbusconnector.h"
#include "voicecallmanager.h"

#include <QObject>
#include <QBluetoothLocalDevice>

class Manager : public QObject
{
    Q_OBJECT

    QBluetoothLocalDevice btDevice;

    watch::WatchConnector *watch;
    DBusConnector *dbus;
    VoiceCallManager *voice;

public:
    explicit Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice);

signals:

public slots:
    void hangupAll();

protected slots:
    void onActiveVoiceCallChanged();
    void onVoiceError(const QString &message);
    void onActiveVoiceCallStatusChanged();

};

#endif // MANAGER_H
