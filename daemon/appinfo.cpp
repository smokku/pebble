#include "appinfo.h"
#include <QSharedData>

struct AppInfoData : public QSharedData {
    QUuid uuid;
    QString shortName;
    QString longName;
    QString companyName;
    int versionCode;
    QString versionLabel;
    bool watchface;
    bool jskit;
    QHash<QString, int> appKeys;
    QString path;
};

AppInfo::AppInfo() : d(new AppInfoData)
{
    d->versionCode = 0;
    d->watchface = false;
    d->jskit = false;
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

QHash<QString, int> AppInfo::appKeys() const
{
    return d->appKeys;
}

void AppInfo::setAppKeys(const QHash<QString, int> &appKeys)
{
    d->appKeys = appKeys;
}

void AppInfo::addAppKey(const QString &key, int value)
{
    d->appKeys.insert(key, value);
}

QString AppInfo::path() const
{
    return d->path;
}

void AppInfo::setPath(const QString &string)
{
    d->path = string;
}
