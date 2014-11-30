#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include "appmanager.h"

AppManager::AppManager(QObject *parent)
    : QObject(parent),
      _watcher(new QFileSystemWatcher(this))
{
    connect(_watcher, &QFileSystemWatcher::directoryChanged,
            this, &AppManager::rescan);

    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    if (!dataDir.exists("apps")) {
        if (!dataDir.mkdir("apps")) {
            logger()->warn() << "could not create dir" << dataDir.absoluteFilePath("apps");
        }
    }
    logger()->debug() << "install apps in" << dataDir.absoluteFilePath("apps");

    rescan();
}

QStringList AppManager::appPaths() const
{
    return QStandardPaths::locateAll(QStandardPaths::DataLocation,
                                     QLatin1String("apps"),
                                     QStandardPaths::LocateDirectory);
}

void AppManager::rescan()
{
    QStringList watchedDirs = _watcher->directories();
    if (!watchedDirs.isEmpty()) _watcher->removePaths(watchedDirs);
    QStringList watchedFiles = _watcher->files();
    if (!watchedFiles.isEmpty()) _watcher->removePaths(watchedFiles);
    _apps.clear();
    _names.clear();

    Q_FOREACH(const QString &path, appPaths()) {
        QDir dir(path);
        _watcher->addPath(dir.absolutePath());
        logger()->debug() << "scanning dir" << dir.absolutePath();
        QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable | QDir::Executable);
        logger()->debug() << "scanning dir results" << entries;
        Q_FOREACH(const QString &path, entries) {
            QString appPath = dir.absoluteFilePath(path);
            _watcher->addPath(appPath);
            if (dir.exists(path + "/appinfo.json")) {
                _watcher->addPath(appPath + "/appinfo.json");
                scanApp(appPath);
            }
        }
    }

    logger()->debug() << "now watching" << _watcher->directories() << _watcher->files();
}

void AppManager::scanApp(const QString &path)
{
    logger()->debug() << "scanning app" << path;
    QDir appDir(path);
    if (!appDir.isReadable()) {
        logger()->warn() << "app" << appDir.absolutePath() << "is not readable";
        return;
    }

    QFile appInfoFile(path + "/appinfo.json");
    if (!appInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger()->warn() << "cannot open app info file" << appInfoFile.fileName() << ":"
                         << appInfoFile.errorString();
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(appInfoFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        logger()->warn() << "cannot parse app info file" << appInfoFile.fileName() << ":"
                         << parseError.errorString();
        return;
    }

    const QJsonObject root = doc.object();
    AppInfo info;
    info.uuid = QUuid(root["uuid"].toString());
    info.shortName = root["shortName"].toString();
    info.longName = root["longName"].toString();
    info.company = root["companyName"].toString();
    info.versionCode = root["versionCode"].toInt();
    info.versionLabel = root["versionLabel"].toString();

    const QJsonObject watchapp = root["watchapp"].toObject();
    info.isWatchface = watchapp["watchface"].toBool();
    info.isJSKit = appDir.exists("pebble-js-app.js");

    const QJsonObject appkeys = root["appKeys"].toObject();
    for (QJsonObject::const_iterator it = appkeys.constBegin(); it != appkeys.constEnd(); ++it) {
        info.appKeys.insert(it.key(), it.value().toInt());
    }

    if (info.uuid.isNull() || info.shortName.isEmpty()) {
        logger()->warn() << "invalid or empty uuid/name in" << appInfoFile.fileName();
        return;
    }

    _apps.insert(info.uuid, info);
    _names.insert(info.shortName, info.uuid);

    const char *type = info.isWatchface ? "watchface" : "app";
    logger()->debug() << "found installed" << type << info.shortName << info.versionLabel << "with uuid" << info.uuid.toString();
}
