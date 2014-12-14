#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include "appmanager.h"
#include "unpacker.h"
#include "stm32crc.h"

namespace {
struct ResourceEntry {
    int index;
    quint32 offset;
    quint32 length;
    quint32 crc;
};
}

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

QList<QUuid> AppManager::appUuids() const
{
    return _apps.keys();
}

AppInfo AppManager::info(const QUuid &uuid) const
{
    return _apps.value(uuid);
}

AppInfo AppManager::info(const QString &name) const
{
    QUuid uuid = _names.value(name);
    if (!uuid.isNull()) {
        return info(uuid);
    } else {
        return AppInfo();
    }
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
    emit appsChanged();
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
    info.setUuid(QUuid(root["uuid"].toString()));
    info.setShortName(root["shortName"].toString());
    info.setLongName(root["longName"].toString());
    info.setCompanyName(root["companyName"].toString());
    info.setVersionCode(root["versionCode"].toInt());
    info.setVersionLabel(root["versionLabel"].toString());

    const QJsonObject watchapp = root["watchapp"].toObject();
    info.setWatchface(watchapp["watchface"].toBool());
    info.setJSKit(appDir.exists("pebble-js-app.js"));

    if (root.contains("capabilities")) {
        const QJsonArray capabilities = root["capabilities"].toArray();
        AppInfo::Capabilities caps = 0;
        for (auto it = capabilities.constBegin(); it != capabilities.constEnd(); ++it) {
            QString cap = (*it).toString();
            if (cap == "location") caps |= AppInfo::Location;
            if (cap == "configurable") caps |= AppInfo::Configurable;
        }
        info.setCapabilities(caps);
    }

    if (root.contains("appKeys")) {
        const QJsonObject appkeys = root["appKeys"].toObject();
        for (auto it = appkeys.constBegin(); it != appkeys.constEnd(); ++it) {
            info.addAppKey(it.key(), it.value().toInt());
        }
    }

    if (root.contains("resources")) {
        const QJsonObject resources = root["resources"].toObject();
        const QJsonArray media = resources["media"].toArray();
        int index = 0;

        for (auto it = media.constBegin(); it != media.constEnd(); ++it) {
            const QJsonObject res = (*it).toObject();
            const QJsonValue menuIcon = res["menuIcon"];

            bool is_menu_icon = false;
            switch (menuIcon.type()) {
            case QJsonValue::Bool:
                is_menu_icon = menuIcon.toBool();
                break;
            case QJsonValue::String:
                is_menu_icon = !menuIcon.toString().isEmpty();
                break;
            default:
                break;
            }

            if (is_menu_icon) {
                QByteArray data = extractFromResourcePack(appDir.filePath("app_resources.pbpack"), index);
                if (!data.isEmpty()) {
                    QImage icon = decodeResourceImage(data);
                    info.setMenuIcon(icon);
                }
            }

            index++;
        }
    }

    info.setPath(path);

    if (info.uuid().isNull() || info.shortName().isEmpty()) {
        logger()->warn() << "invalid or empty uuid/name in" << appInfoFile.fileName();
        return;
    }

    _apps.insert(info.uuid(), info);
    _names.insert(info.shortName(), info.uuid());

    const char *type = info.isWatchface() ? "watchface" : "app";
    logger()->debug() << "found installed" << type << info.shortName() << info.versionLabel() << "with uuid" << info.uuid().toString();
}

QByteArray AppManager::extractFromResourcePack(const QString &file, int wanted_id) const
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        logger()->warn() << "cannot open resource file" << f.fileName();
        return QByteArray();
    }

    QByteArray data = f.readAll();
    Unpacker u(data);

    int num_files = u.readLE<quint32>();
    u.readLE<quint32>(); // crc for entire file
    u.readLE<quint32>(); // timestamp

    logger()->debug() << "reading" << num_files << "resources from" << file;

    QList<ResourceEntry> table;

    for (int i = 0; i < num_files; i++) {
        ResourceEntry e;
        e.index = u.readLE<quint32>();
        e.offset = u.readLE<quint32>();
        e.length = u.readLE<quint32>();
        e.crc = u.readLE<quint32>();

        if (u.bad()) {
            logger()->warn() << "short read on resource file";
            return QByteArray();
        }

        table.append(e);
    }

    if (wanted_id >= table.size()) {
        logger()->warn() << "specified resource does not exist";
        return QByteArray();
    }

    const ResourceEntry &e = table[wanted_id];

    int offset = 12 + 256 * 16 + e.offset;

    QByteArray res = data.mid(offset, e.length);

    Stm32Crc crc;
    crc.addData(res);

    if (crc.result() != e.crc) {
        logger()->warn() << "CRC failure in resource" << e.index << "on file" << file;
        return QByteArray();
    }

    return res;
}

QImage AppManager::decodeResourceImage(const QByteArray &data) const
{
    Unpacker u(data);
    int scanline = u.readLE<quint16>();
    u.skip(sizeof(quint16) + sizeof(quint32));
    int width = u.readLE<quint16>();
    int height = u.readLE<quint16>();

    QImage img(width, height, QImage::Format_MonoLSB);
    const uchar *src = reinterpret_cast<const uchar *>(&data.constData()[12]);
    for (int line = 0; line < height; ++line) {
        memcpy(img.scanLine(line), src, qMin(scanline, img.bytesPerLine()));
        src += scanline;
    }

    return img;
}
