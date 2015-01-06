#include <QSharedData>
#include <QBuffer>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "appinfo.h"
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

struct AppInfoData : public QSharedData {
    QUuid uuid;
    QString shortName;
    QString longName;
    QString companyName;
    int versionCode;
    QString versionLabel;
    bool watchface;
    bool jskit;
    AppInfo::Capabilities capabilities;
    QHash<QString, int> keyInts;
    QHash<int, QString> keyNames;
    bool menuIcon;
    int menuIconResource;
    QString path;
};

QLoggingCategory AppInfo::l("AppInfo");

AppInfo::AppInfo() : d(new AppInfoData)
{
    d->versionCode = 0;
    d->watchface = false;
    d->jskit = false;
    d->capabilities = 0;
    d->menuIcon = false;
    d->menuIconResource = -1;
}

AppInfo::AppInfo(const AppInfo &rhs) : d(rhs.d)
{
}

AppInfo &AppInfo::operator=(const AppInfo &rhs)
{
    if (this != &rhs)
        d.operator=(rhs.d);
    return *this;
}

AppInfo::~AppInfo()
{
}

bool AppInfo::isLocal() const
{
    return ! d->path.isEmpty();
}

bool AppInfo::isValid() const
{
    return ! d->uuid.isNull();
}

void AppInfo::setInvalid()
{
    d->uuid = QUuid(); // Clear the uuid to force invalid app
}

QUuid AppInfo::uuid() const
{
    return d->uuid;
}

QString AppInfo::shortName() const
{
    return d->shortName;
}

QString AppInfo::longName() const
{
    return d->longName;
}

QString AppInfo::companyName() const
{
    return d->companyName;
}

int AppInfo::versionCode() const
{
    return d->versionCode;
}

QString AppInfo::versionLabel() const
{
    return d->versionLabel;
}

bool AppInfo::isWatchface() const
{
    return d->watchface;
}

bool AppInfo::isJSKit() const
{
    return d->jskit;
}

AppInfo::Capabilities AppInfo::capabilities() const
{
    return d->capabilities;
}

void AppInfo::addAppKey(const QString &key, int value)
{
    d->keyInts.insert(key, value);
    d->keyNames.insert(value, key);
}

bool AppInfo::hasAppKeyValue(int value) const
{
    return d->keyNames.contains(value);
}

QString AppInfo::appKeyForValue(int value) const
{
    return d->keyNames.value(value);
}

bool AppInfo::hasAppKey(const QString &key) const
{
    return d->keyInts.contains(key);
}

int AppInfo::valueForAppKey(const QString &key) const
{
    return d->keyInts.value(key, -1);
}

bool AppInfo::hasMenuIcon() const
{
    return d->menuIcon && d->menuIconResource >= 0;
}

QImage AppInfo::getMenuIconImage() const
{
    if (hasMenuIcon()) {
        QByteArray data = extractFromResourcePack(
                    QDir(d->path).filePath("app_resources.pbpack"), d->menuIconResource);
        if (!data.isEmpty()) {
            return decodeResourceImage(data);
        }
    }

    return QImage();
}

QByteArray AppInfo::getMenuIconPng() const
{
    QByteArray data;
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);
    getMenuIconImage().save(&buf, "PNG");
    buf.close();
    return data;
}

QString AppInfo::getJSApp() const
{
    if (!isValid() || !isLocal()) return QString();

    QFile file(d->path + "/pebble-js-app.js");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(l) << "Failed to load JS file:" << file.fileName();
        return QString();
    }

    return QString::fromUtf8(file.readAll());

}

AppInfo AppInfo::fromPath(const QString &path)
{
    AppInfo info;

    QDir appDir(path);
    if (!appDir.isReadable()) {
        qCWarning(l) << "app" << appDir.absolutePath() << "is not readable";
        return info;
    }

    QFile appInfoFile(path + "/appinfo.json");
    if (!appInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(l) << "cannot open app info file" << appInfoFile.fileName() << ":"
                         << appInfoFile.errorString();
        return info;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(appInfoFile.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(l) << "cannot parse app info file" << appInfoFile.fileName() << ":"
                         << parseError.errorString();
        return info;
    }

    const QJsonObject root = doc.object();
    info.d->uuid = QUuid(root["uuid"].toString());
    info.d->shortName = root["shortName"].toString();
    info.d->longName = root["longName"].toString();
    info.d->companyName = root["companyName"].toString();
    info.d->versionCode = root["versionCode"].toInt();
    info.d->versionLabel = root["versionLabel"].toString();

    const QJsonObject watchapp = root["watchapp"].toObject();
    info.d->watchface = watchapp["watchface"].toBool();
    info.d->jskit = appDir.exists("pebble-js-app.js");

    if (root.contains("capabilities")) {
        const QJsonArray capabilities = root["capabilities"].toArray();
        AppInfo::Capabilities caps = 0;
        for (auto it = capabilities.constBegin(); it != capabilities.constEnd(); ++it) {
            QString cap = (*it).toString();
            if (cap == "location") caps |= AppInfo::Location;
            if (cap == "configurable") caps |= AppInfo::Configurable;
        }
        info.d->capabilities = caps;
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

            switch (menuIcon.type()) {
            case QJsonValue::Bool:
                info.d->menuIcon = menuIcon.toBool();
                info.d->menuIconResource = index;
                break;
            case QJsonValue::String:
                info.d->menuIcon = !menuIcon.toString().isEmpty();
                info.d->menuIconResource = index;
                break;
            default:
                break;
            }

            index++;
        }
    }

    info.d->path = path;

    if (info.uuid().isNull() || info.shortName().isEmpty()) {
        qCWarning(l) << "invalid or empty uuid/name in" << appInfoFile.fileName();
        return AppInfo();
    }

    return info;
}

AppInfo AppInfo::fromSlot(const BankManager::SlotInfo &slot)
{
    AppInfo info;

    info.d->uuid = QUuid::createUuid();
    info.d->shortName = slot.name;
    info.d->companyName = slot.company;
    info.d->versionCode = slot.version;
    info.d->capabilities = AppInfo::Capabilities(slot.flags);

    return info;
}

QByteArray AppInfo::extractFromResourcePack(const QString &file, int wanted_id) const
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) {
        qCWarning(l) << "cannot open resource file" << f.fileName();
        return QByteArray();
    }

    QByteArray data = f.readAll();
    Unpacker u(data);

    int num_files = u.readLE<quint32>();
    u.readLE<quint32>(); // crc for entire file
    u.readLE<quint32>(); // timestamp

    qCDebug(l) << "reading" << num_files << "resources from" << file;

    QList<ResourceEntry> table;

    for (int i = 0; i < num_files; i++) {
        ResourceEntry e;
        e.index = u.readLE<quint32>();
        e.offset = u.readLE<quint32>();
        e.length = u.readLE<quint32>();
        e.crc = u.readLE<quint32>();

        if (u.bad()) {
            qCWarning(l) << "short read on resource file";
            return QByteArray();
        }

        table.append(e);
    }

    if (wanted_id >= table.size()) {
        qCWarning(l) << "specified resource does not exist";
        return QByteArray();
    }

    const ResourceEntry &e = table[wanted_id];

    int offset = 12 + 256 * 16 + e.offset;

    QByteArray res = data.mid(offset, e.length);

    Stm32Crc crc;
    crc.addData(res);

    if (crc.result() != e.crc) {
        qCWarning(l) << "CRC failure in resource" << e.index << "on file" << file;
        return QByteArray();
    }

    return res;
}

QImage AppInfo::decodeResourceImage(const QByteArray &data) const
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

// TODO: abstract to QIOReader
QString AppInfo::filePath(enum AppInfo::File file) const
{
    QString fileName;
    switch (file) {
    case AppInfo::BINARY:
        fileName = "pebble-app.bin";
        break;
    case AppInfo::RESOURCES:
        fileName = "app_resources.pbpack";
        break;
    }

    QDir appDir(d->path);
    if (appDir.exists(fileName)) {
        return appDir.absoluteFilePath(fileName);
    }

    return QString();
}
