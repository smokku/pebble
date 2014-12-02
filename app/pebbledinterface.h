#ifndef PEBBLEDINTERFACE_H
#define PEBBLEDINTERFACE_H

#include <QObject>
#include <QUrl>
#include <QDBusInterface>

class OrgPebbledWatchInterface;

class PebbledInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString address READ address NOTIFY addressChanged)
    Q_PROPERTY(QString appUuid READ appUuid NOTIFY appUuidChanged)

public:
    explicit PebbledInterface(QObject *parent = 0);

    bool enabled() const;
    bool active() const;
    bool connected() const;
    QString name() const;
    QString address() const;
    QString appUuid() const;

signals:
    void enabledChanged();
    void activeChanged();
    void connectedChanged();
    void nameChanged();
    void addressChanged();
    void appUuidChanged();

public slots:
    void setEnabled(bool);
    void setActive(bool);
    void ping();
    void time();
    void disconnect();
    void reconnect();

    QUrl configureApp(const QString &uuid);
    void setAppConfiguration(const QString &uuid, const QString &data);

private slots:
    void getUnitProperties();
    void onPropertiesChanged(QString interface, QMap<QString, QVariant> changed, QStringList invalidated);

private:
    QDBusInterface *systemd;
    OrgPebbledWatchInterface *watch;
    QDBusObjectPath unitPath;
    QVariantMap unitProperties;
};

#endif // PEBBLEDINTERFACE_H
