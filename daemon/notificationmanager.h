#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <Log4Qt/Logger>
#include "settings.h"

#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

typedef QHash<QString, QString> QStringHash;

enum {
    notificationEMAIL = 0,
    notificationSMS = 1,
    notificationFACEBOOK = 2,
    notificationTWITTER = 3
};

struct NotificationManagerMessage
{
    int type;
    QString subject;
    QString data;
    QString sender;

    NotificationManagerMessage() {}
    NotificationManagerMessage(const int &type, const QString &sender, const QString &data, const QString &subject = QString())
        : type(type), subject(subject), data(data), sender(sender) {}
};

Q_DECLARE_METATYPE(NotificationManagerMessage)

class NotificationManagerQueueHandler : public QThread
{
    Q_OBJECT
public:
    explicit NotificationManagerQueueHandler(QObject *parent = 0);
            ~NotificationManagerQueueHandler();
    void queueNotification(const NotificationManagerMessage &notification);
Q_SIGNALS:
    void notificationReady(const NotificationManagerMessage &notification);
protected:
    void run();
private:
    QMutex *mutex;
    QQueue<NotificationManagerMessage> *queue;
    QWaitCondition waitcond;
};

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
    void onQueuedNotification(const NotificationManagerMessage &notification);

protected Q_SLOTS:
    void initialize(bool notifyError = false);

private:
    class NotificationManagerPrivate *d_ptr;

    QString getCleanAppName(QString app_name);
    QStringHash getCategoryParams(QString category);
    Settings *settings;
    NotificationManagerQueueHandler *queueHandler;

    Q_DISABLE_COPY(NotificationManager)
    Q_DECLARE_PRIVATE(NotificationManager)
};

#endif // NOTIFICATIONMANAGER_H
