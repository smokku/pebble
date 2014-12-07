#ifndef BANKMANAGER_H
#define BANKMANAGER_H

#include "watchconnector.h"
#include "uploadmanager.h"
#include "appmanager.h"

class BankManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit BankManager(WatchConnector *watch, UploadManager *upload, AppManager *apps, QObject *parent = 0);

    int numSlots() const;

    bool isUsed(int slot) const;
    QUuid appAt(int slot) const;

signals:
    void slotsChanged();

public slots:
    bool uploadApp(const QUuid &uuid, int slot = -1);
    bool unloadApp(int slot);

    void refresh();

private:
    int findUnusedSlot() const;
    void refreshWatchApp(int slot, std::function<void()> successCallback, std::function<void(int)> errorCallback);

private slots:
    void handleWatchConnected();

private:
    WatchConnector *watch;
    UploadManager *upload;
    AppManager *apps;

    enum ResultCodes {
        Success = 1,
        BankInUse = 2,
        InvalidCommand = 3,
        GeneralFailure = 4
    };

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
    QTimer *_refresh;
};

#endif // BANKMANAGER_H
