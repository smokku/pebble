#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <QObject>
#include <QDBusContext>
#include <QDBusServiceWatcher>
#include "watchconnector.h"
#include "settings.h"

class MusicManager : public QObject, protected QDBusContext
{
    Q_OBJECT
    QLoggingCategory l;

public:
    explicit MusicManager(WatchConnector *watch, Settings *settings, QObject *parent = 0);
    virtual ~MusicManager();

private:
    void switchToService(const QString &service);
    void fetchMetadataFromService();
    void sendCurrentMprisMetadata();
    void callMprisMethod(const QString &method);

private slots:
    void handleMusicControl(WatchConnector::MusicControl operation);
    void handleMprisServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void handleMprisPropertiesChanged(const QString &interface, const QMap<QString,QVariant> &changed, const QStringList &invalidated);
    void handleWatchConnected();

private:
    WatchConnector *watch;
    QDBusServiceWatcher *_watcher;
    QString _curService;
    QVariantMap _curMetadata;
    QDBusConnection *_pulseBus;
    Settings *settings;
    uint _maxVolume;
};

#endif // MUSICMANAGER_H
