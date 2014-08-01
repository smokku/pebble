#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <Log4Qt/Logger>
#include "settings.h"

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>

typedef QHash<QString, QString> QStringHash;

class NotificationManager : public QObject
{
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

    Q_PROPERTY(QDBusInterface* interface READ interface)
public:
    explicit NotificationManager(Settings *settings, QObject *parent = 0);
            ~NotificationManager();

    QDBusInterface* interface() const;

Q_SIGNALS:
    void error(const QString &message);
    void smsNotify(const QString &sender, const QString &data);
    void twitterNotify(const QString &sender, const QString &data);
    void facebookNotify(const QString &sender, const QString &data);
    void emailNotify(const QString &sender, const QString &data,const QString &subject);

public Q_SLOTS:
    void Notify(const QString &app_name, uint replaces_id, const QString &app_icon, const QString &summary, const QString &body, const QStringList &actions, const QVariantHash &hints, int expire_timeout);

protected Q_SLOTS:
    void initialize(bool notifyError = false);

private:
    class NotificationManagerPrivate *d_ptr;

    QString getCleanAppName(QString app_name);
    QStringHash getCategoryParams(QString category);
    Settings *settings;

    Q_DISABLE_COPY(NotificationManager)
    Q_DECLARE_PRIVATE(NotificationManager)
};

#endif // NOTIFICATIONMANAGER_H
