#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <QObject>
#include <QDBusContext>
#include <QDBusServiceWatcher>
#include "watchconnector.h"

class MusicManager : public QObject, protected QDBusContext
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit MusicManager(WatchConnector *watch, QObject *parent = 0);

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
};

#endif // MUSICMANAGER_H
