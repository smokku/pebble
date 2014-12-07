#ifndef BANKMANAGER_H
#define BANKMANAGER_H

#include "watchconnector.h"
#include "appmanager.h"

class BankManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit BankManager(WatchConnector *watch, AppManager *apps, QObject *parent = 0);

    int numSlots() const;


signals:
    void slotsChanged();

public slots:
    bool uploadApp(const QUuid &uuid, int slot = -1);
    bool unloadApp(int slot);

    void refresh();

private:
    int findUnusedSlot() const;


private slots:
    void handleWatchConnected();

private:
    WatchConnector *watch;
    AppManager *apps;

    struct SlotInfo {
        bool used;
        quint32 id;
        QString name;
        QString company;
        quint32 flags;
        quint16 version;
        QUuid uuid;
    };

    QVector<SlotInfo> _slots;
};

#endif // BANKMANAGER_H
