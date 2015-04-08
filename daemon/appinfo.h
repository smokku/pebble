#ifndef APPINFO_H
#define APPINFO_H

#include <QSharedDataPointer>
#include <QUuid>
#include <QHash>
#include <QImage>
#include <QLoggingCategory>
#include "bundle.h"
#include "bankmanager.h"

class AppInfoData;

class AppInfo : public Bundle
{
    Q_GADGET

    static QLoggingCategory l;

public:
    enum Capability {
        Location = 1 << 0,
        Configurable = 1 << 2
    };
    Q_DECLARE_FLAGS(Capabilities, Capability)

    Q_PROPERTY(bool local READ isLocal)
    Q_PROPERTY(bool valid READ isValid)
    Q_PROPERTY(QUuid uuid READ uuid)
    Q_PROPERTY(QString shortName READ shortName)
    Q_PROPERTY(QString longName READ longName)
    Q_PROPERTY(QString companyName READ companyName)
    Q_PROPERTY(int versionCode READ versionCode)
    Q_PROPERTY(QString versionLabel READ versionLabel)
    Q_PROPERTY(bool watchface READ isWatchface)
    Q_PROPERTY(bool jskit READ isJSKit)
    Q_PROPERTY(Capabilities capabilities READ capabilities)
    Q_PROPERTY(bool menuIcon READ hasMenuIcon)
    Q_PROPERTY(QImage menuIconImage READ getMenuIconImage)

    static AppInfo fromPath(const QString &path);
    static AppInfo fromSlot(const BankManager::SlotInfo &slot);

public:
    AppInfo();
    AppInfo(const AppInfo &);
    AppInfo(const Bundle &);
    AppInfo &operator=(const AppInfo &);
    ~AppInfo();

    bool isLocal() const;
    bool isValid() const;
    QUuid uuid() const;
    QString shortName() const;
    QString longName() const;
    QString companyName() const;
    int versionCode() const;
    QString versionLabel() const;
    bool isWatchface() const;
    bool isJSKit() const;
    Capabilities capabilities() const;
    bool hasMenuIcon() const;

    void addAppKey(const QString &key, int value);
    bool hasAppKeyValue(int value) const;
    QString appKeyForValue(int value) const;

    bool hasAppKey(const QString &key) const;
    int valueForAppKey(const QString &key) const;

    QImage getMenuIconImage() const;
    QByteArray getMenuIconPng() const;
    QString getJSApp() const;

    void setInvalid();

protected:
    QByteArray extractFromResourcePack(QIODevice *dev, int id) const;
    QImage decodeResourceImage(const QByteArray &data) const;

private:
    QSharedDataPointer<AppInfoData> d;
};

#endif // APPINFO_H
