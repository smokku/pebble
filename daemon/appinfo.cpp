#include <QSharedData>
#include <QBuffer>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "appinfo.h"
#include "unpacker.h"

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
};

QLoggingCategory AppInfo::l("AppInfo");

AppInfo::AppInfo() : Bundle(), d(new AppInfoData)
{
    d->versionCode = 0;
    d->watchface = false;
    d->jskit = false;
    d->capabilities = 0;
    d->menuIcon = false;
    d->menuIconResource = -1;
}

AppInfo::AppInfo(const AppInfo &rhs) : Bundle(rhs), d(rhs.d)
{}

AppInfo::AppInfo(const Bundle &rhs) : Bundle(rhs), d(new AppInfoData)
{}

AppInfo &AppInfo::operator=(const AppInfo &rhs)
{
    Bundle::operator=(rhs);
    if (this != &rhs)
        d.operator=(rhs.d);
    return *this;
}

AppInfo::~AppInfo()
{}

bool AppInfo::isLocal() const
{
    return ! path().isEmpty();
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
        QScopedPointer<QIODevice> imageRes(openFile(AppInfo::RESOURCES));
        QByteArray data = extractFromResourcePack(imageRes.data(), d->menuIconResource);
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

    QScopedPointer<QIODevice> appJS(openFile(AppInfo::APPJS, QIODevice::Text));
    if (!appJS) {
        qCWarning(l) << "cannot find app" << d->shortName << "app.js";
        return QString();
    }

    return QString::fromUtf8(appJS->readAll());
}

AppInfo AppInfo::fromPath(const QString &path)
{
    AppInfo info(Bundle::fromPath(path));

    if (!static_cast<Bundle>(info).isValid()) {
        qCWarning(l) << "bundle" << path << "is not valid";
        return AppInfo();
    }

    QScopedPointer<QIODevice> appInfoJSON(info.openFile(AppInfo::INFO, QIODevice::Text));
    if (!appInfoJSON) {
        qCWarning(l) << "cannot find app" << path << "info json";
        return AppInfo();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(appInfoJSON->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(l) << "cannot parse app" << path << "info json" << parseError.errorString();
        return AppInfo();
    }
    appInfoJSON->close();

    const QJsonObject root = doc.object();
    info.d->uuid = QUuid(root["uuid"].toString());
    info.d->shortName = root["shortName"].toString();
    info.d->longName = root["longName"].toString();
    info.d->companyName = root["companyName"].toString();
    info.d->versionCode = root["versionCode"].toInt();
    info.d->versionLabel = root["versionLabel"].toString();

    const QJsonObject watchapp = root["watchapp"].toObject();
    info.d->watchface = watchapp["watchface"].toBool();

    info.d->jskit = info.fileExists(AppInfo::APPJS);

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

    if (info.uuid().isNull() || info.shortName().isEmpty()) {
        qCWarning(l) << "invalid or empty uuid/name in json of" << path;
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

QByteArray AppInfo::extractFromResourcePack(QIODevice *dev, int wanted_id) const
{
    if (!dev) {
        qCWarning(l) << "requested resource" << wanted_id
                     << "from NULL resource file";
        return QByteArray();
    }

    QByteArray data = dev->readAll();
    Unpacker u(data);

    int num_files = u.readLE<quint32>();
    u.readLE<quint32>(); // crc for entire file
    u.readLE<quint32>(); // timestamp

    qCDebug(l) << "reading" << num_files << "resources";

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

    return data.mid(offset, e.length);
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
