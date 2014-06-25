#ifndef MANAGER_H
#define MANAGER_H

#include "watchconnector.h"
#include "dbusconnector.h"
#include "voicecallmanager.h"

#include <QObject>
#include <QBluetoothLocalDevice>
#include <QtContacts/QContactManager>
#include <QtContacts/QContactDetailFilter>

using namespace QtContacts;

class Manager : public QObject
{
    Q_OBJECT

    QBluetoothLocalDevice btDevice;

    watch::WatchConnector *watch;
    DBusConnector *dbus;
    VoiceCallManager *voice;

    QContactManager *contacts;
    QContactDetailFilter numberFilter;

public:
    explicit Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice);

    Q_INVOKABLE QString findPersonByNumber(QString number);

signals:

public slots:
    void hangupAll();

protected slots:
    void onActiveVoiceCallChanged();
    void onVoiceError(const QString &message);
    void onActiveVoiceCallStatusChanged();

};

#endif // MANAGER_H
