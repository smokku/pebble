#ifndef MUSICMANAGER_H
#define MUSICMANAGER_H

#include <QObject>
#include <QDBusContext>
#include "watchconnector.h"

class MusicManager : public QObject, protected QDBusContext
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit MusicManager(WatchConnector *watch, QObject *parent = 0);

private:
    void musicControl(WatchConnector::MusicControl operation);
    void switchToService(const QString &service);
    void setMprisMetadata(const QVariantMap &data);

private slots:
    void handleServiceRegistered(const QString &service);
    void handleServiceUnregistered(const QString &service);
    void handleServiceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void handleMprisPropertiesChanged(const QString &interface, const QMap<QString,QVariant> &changed, const QStringList &invalidated);
    void handleWatchConnected();

private:
    WatchConnector *watch;

    QVariantMap _curMetadata;
    QString _curService;
};

#endif // MUSICMANAGER_H
