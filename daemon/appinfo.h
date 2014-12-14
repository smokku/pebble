#ifndef APPINFO_H
#define APPINFO_H

#include <QSharedDataPointer>
#include <QUuid>
#include <QHash>
#include <QImage>

class AppInfoData;

class AppInfo
{
    Q_GADGET

public:
    enum Capability {
        Location = 1 << 0,
        Configurable = 1 << 2
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    Q_PROPERTY(QUuid uuid READ uuid WRITE setUuid)
    Q_PROPERTY(QString shortName READ shortName WRITE setShortName)
    Q_PROPERTY(QString longName READ longName WRITE setLongName)
    Q_PROPERTY(QString companyName READ companyName WRITE setCompanyName)
    Q_PROPERTY(int versionCode READ versionCode WRITE setVersionCode)
    Q_PROPERTY(QString versionLabel READ versionLabel WRITE setVersionLabel)
    Q_PROPERTY(bool watchface READ isWatchface WRITE setWatchface)
    Q_PROPERTY(bool jskit READ isJSKit WRITE setJSKit)
    Q_PROPERTY(Capabilities capabilities READ capabilities WRITE setCapabilities)
    Q_PROPERTY(QImage menuIcon READ menuIcon WRITE setMenuIcon)
    Q_PROPERTY(QString path READ path WRITE setPath)

public:
    AppInfo();
    AppInfo(const AppInfo &);
    AppInfo &operator=(const AppInfo &);
    ~AppInfo();

    QUuid uuid() const;
    void setUuid(const QUuid &uuid);

    QString shortName() const;
    void setShortName(const QString &string);

    QString longName() const;
    void setLongName(const QString &string);

    QString companyName() const;
    void setCompanyName(const QString &string);

    int versionCode() const;
    void setVersionCode(int code);

    QString versionLabel() const;
    void setVersionLabel(const QString &string);

    bool isWatchface() const;
    void setWatchface(bool b);

    bool isJSKit() const;
    void setJSKit(bool b);

    Capabilities capabilities() const;
    void setCapabilities(Capabilities caps);

    void addAppKey(const QString &key, int value);

    bool hasAppKeyValue(int value) const;
    QString appKeyForValue(int value) const;

    bool hasAppKey(const QString &key) const;
    int valueForAppKey(const QString &key) const;

    QImage menuIcon() const;
    QByteArray menuIconAsPng() const;
    void setMenuIcon(const QImage &img);

    QString path() const;
    void setPath(const QString &string);

private:
    QSharedDataPointer<AppInfoData> d;
};

#endif // APPINFO_H
