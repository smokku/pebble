#include <QSharedData>
#include <QBuffer>
#include "appinfo.h"

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
    QImage menuIcon;
    QString path;
};

AppInfo::AppInfo() : d(new AppInfoData)
{
    d->versionCode = 0;
    d->watchface = false;
    d->jskit = false;
    d->capabilities = 0;
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

QUuid AppInfo::uuid() const
{
    return d->uuid;
}

void AppInfo::setUuid(const QUuid &uuid)
{
    d->uuid = uuid;
}

QString AppInfo::shortName() const
{
    return d->shortName;
}

void AppInfo::setShortName(const QString &string)
{
    d->shortName = string;
}

QString AppInfo::longName() const
{
    return d->longName;
}

void AppInfo::setLongName(const QString &string)
{
    d->longName = string;
}

QString AppInfo::companyName() const
{
    return d->companyName;
}

void AppInfo::setCompanyName(const QString &string)
{
    d->companyName = string;
}

int AppInfo::versionCode() const
{
    return d->versionCode;
}

void AppInfo::setVersionCode(int code)
{
    d->versionCode = code;
}

QString AppInfo::versionLabel() const
{
    return d->versionLabel;
}

void AppInfo::setVersionLabel(const QString &string)
{
    d->versionLabel = string;
}

bool AppInfo::isWatchface() const
{
    return d->watchface;
}

void AppInfo::setWatchface(bool b)
{
    d->watchface = b;
}

bool AppInfo::isJSKit() const
{
    return d->jskit;
}

void AppInfo::setJSKit(bool b)
{
    d->jskit = b;
}

AppInfo::Capabilities AppInfo::capabilities() const
{
    return d->capabilities;
}

void AppInfo::setCapabilities(Capabilities caps)
{
    d->capabilities = caps;
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

QImage AppInfo::menuIcon() const
{
    return d->menuIcon;
}

QByteArray AppInfo::menuIconAsPng() const
{
    QByteArray data;
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);
    d->menuIcon.save(&buf, "PNG");
    buf.close();
    return data;
}

void AppInfo::setMenuIcon(const QImage &img)
{
    d->menuIcon = img;
}

QString AppInfo::path() const
{
    return d->path;
}

void AppInfo::setPath(const QString &string)
{
    d->path = string;
}
