#ifndef PEBBLEDINTERFACE_H
#define PEBBLEDINTERFACE_H

#include <QObject>
#include <QtDBus/QtDBus>

class PebbledInterface : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    bool enabled() const;
    void setEnabled(bool);

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    bool active() const;
    void setActive(bool);

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

private:
    QDBusInterface *systemd;
    QString systemdUnit;

    QDBusInterface *pebbled;
};

#endif // PEBBLEDINTERFACE_H
