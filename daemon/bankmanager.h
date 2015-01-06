#ifndef BANKMANAGER_H
#define BANKMANAGER_H

#include <QTimer>
#include <QUuid>
#include <QVector>
#include <QLoggingCategory>

class WatchConnector;
class UploadManager;
class AppManager;

class BankManager : public QObject
{
    Q_OBJECT
    QLoggingCategory l;

public:
    struct SlotInfo {
        bool used;
        quint32 id;
        QString name;
        QString company;
        quint32 flags;
        quint16 version;
        QUuid uuid;
    };

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

    QVector<SlotInfo> _slots;
    QTimer *_refresh;
};

#endif // BANKMANAGER_H
