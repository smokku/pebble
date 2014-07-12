#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>

class NotificationManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

    Q_PROPERTY(QDBusInterface* interface READ interface)
public:
    explicit NotificationManager(QObject *parent = 0);
            ~NotificationManager();

    QDBusInterface* interface() const;

Q_SIGNALS:
    void error(const QString &message);
    void smsNotify(const QString &sender, const QString &data);
    void emailNotify(const QString &sender, const QString &data,const QString &subject);

public Q_SLOTS:
    void Notify(const QString &app_name, uint replaces_id, const QString &app_icon, const QString &summary, const QString &body, const QStringList &actions, const QVariantHash &hints, int expire_timeout);

protected Q_SLOTS:
    void initialize(bool notifyError = false);

private:
    class NotificationManagerPrivate *d_ptr;

    QString detectCleanAppname(QString app_name);

    Q_DISABLE_COPY(NotificationManager)
    Q_DECLARE_PRIVATE(NotificationManager)
};

#endif // NOTIFICATIONMANAGER_H