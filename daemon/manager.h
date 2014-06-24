#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "voicecallmanager.h"

#include <QObject>
#include <QBluetoothLocalDevice>

class Manager : public QObject
{
    Q_OBJECT

    QBluetoothLocalDevice btDevice;

    watch::WatchConnector *watch;
    VoiceCallManager *voice;

public:
    explicit Manager(watch::WatchConnector *watch, VoiceCallManager *voice);

signals:

public slots:
    void hangupAll();

protected slots:
    void onBTDeviceDiscovered(const QBluetoothDeviceInfo & device);

    void onActiveVoiceCallChanged();
    void onVoiceError(const QString &message);
    void onActiveVoiceCallStatusChanged();

};

#endif // MANAGER_H
