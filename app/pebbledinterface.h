#ifndef PEBBLEDINTERFACE_H
#define PEBBLEDINTERFACE_H

#include <QObject>
#include <QtDBus/QtDBus>
#include <QDBusArgument>

class PebbledInterface : public QObject
{
    Q_OBJECT

    static QString PEBBLED_SYSTEMD_UNIT;
    static QString SYSTEMD_UNIT_IFACE;

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    bool enabled() const;

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    bool active() const;

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    bool connected() const;

    Q_PROPERTY(QVariantMap pebble READ pebble NOTIFY pebbleChanged)
    QVariantMap pebble() const;

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    QString name() const;

    Q_PROPERTY(QString address READ address NOTIFY addressChanged)
    QString address() const;


public:
    explicit PebbledInterface(QObject *parent = 0);

signals:
    void enabledChanged();
    void activeChanged();

    void connectedChanged();
    void pebbleChanged();
    void nameChanged();
    void addressChanged();

public slots:
    void setEnabled(bool);
    void setActive(bool);

private slots:
    void onPropertyChanged(QString string,QDBusVariant dbv);

private:
    QDBusInterface *pebbled;
    QDBusInterface *systemd;
    QDBusInterface *unitprops;

    QVariantMap properties;
};

#endif // PEBBLEDINTERFACE_H
