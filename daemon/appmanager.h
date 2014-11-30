#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>
#include <QHash>
#include <QUuid>
#include <QFileSystemWatcher>
#include <Log4Qt/Logger>

class AppManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER

public:
    explicit AppManager(QObject *parent = 0);

    struct AppInfo {
        QUuid uuid;
        QString shortName;
        QString longName;
        QString company;
        int versionCode;
        QString versionLabel;
        bool isWatchface;
        bool isJSKit;
        QHash<QString, int> appKeys;
        QString path;
    };

    QStringList appPaths() const;

    bool installPebbleApp(const QString &pbwFile);

public slots:
    void rescan();

private:
    void scanApp(const QString &path);

private:
    QFileSystemWatcher *_watcher;
    QHash<QUuid, AppInfo> _apps;
    QHash<QString, QUuid> _names;
};

#endif // APPMANAGER_H
