#include "notificationmanager.h"

#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QSettings>
#include <QDBusInterface>
#include <QDBusPendingReply>

class NotificationManagerPrivate
{
    Q_DECLARE_PUBLIC(NotificationManager)

public:
    NotificationManagerPrivate(NotificationManager *q)
        : q_ptr(q),
          interface(NULL),
          connected(false)
    { /*...*/ }

    NotificationManager *q_ptr;

    QDBusInterface *interface;

    bool connected;
};

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent), d_ptr(new NotificationManagerPrivate(this))
{
    Q_D(NotificationManager);
    QDBusConnection::sessionBus().registerObject("/org/freedesktop/Notifications", this, QDBusConnection::ExportAllSlots);

    d->interface = new QDBusInterface("org.freedesktop.DBus",
                                      "/org/freedesktop/DBus",
                                     "org.freedesktop.DBus");

    d->interface->call("AddMatch", "interface='org.freedesktop.Notifications',member='Notify',type='method_call',eavesdrop='true'");

    this->initialize();
}

NotificationManager::~NotificationManager()
{
    Q_D(NotificationManager);
    delete d;
}


void NotificationManager::initialize(bool notifyError)
{
    Q_D(NotificationManager);
    bool success = false;

    if(d->interface->isValid())
    {
        success = true;
    }

    if(!(d->connected = success))
    {
        QTimer::singleShot(2000, this, SLOT(initialize()));
        if(notifyError) emit this->error("Failed to connect to Notifications D-Bus service.");
    }
}

QDBusInterface* NotificationManager::interface() const
{
    Q_D(const NotificationManager);
    return d->interface;
}

QString NotificationManager::detectCleanAppname(QString app_name)
{
    QString desktopFile = "/usr/share/applications/" + app_name + ".desktop";
    QFile testFile(desktopFile);
    if (testFile.exists()) {
        QSettings settings(desktopFile, QSettings::IniFormat);
        settings.beginGroup("Desktop Entry");
        QString cleanName = settings.value("Name").toString();
        settings.endGroup();
        if (!cleanName.isEmpty()) {
            return cleanName;
        }
    }
    return app_name;
}

void NotificationManager::Notify(const QString &app_name, uint replaces_id, const QString &app_icon, const QString &summary, const QString &body, const QStringList &actions, const QVariantHash &hints, int expire_timeout) {

    //Ignore notifcations from myself
    if (app_name == "pebbled") {
        return;
    }

    logger()->debug() << Q_FUNC_INFO  << "Got notification via dbus from" << detectCleanAppname(app_name);

    if (app_name == "messageserver5") {
        emit this->emailNotify(hints.value("x-nemo-preview-summary", detectCleanAppname(app_name)).toString(),
                               hints.value("x-nemo-preview-body", "default").toString(),
                               ""
                               );
    } else if (app_name == "commhistoryd") {
        if (summary == "" && body == "") {
            emit this->smsNotify(hints.value("x-nemo-preview-summary", "default").toString(),
                                 hints.value("x-nemo-preview-body", "default").toString()
                                );
        }
    } else {
        //Prioritize x-nemo-preview* over dbus direct summary and body
        QString subject = hints.value("x-nemo-preview-summary", "").toString();
        QString data = hints.value("x-nemo-preview-body", "").toString();

        if (subject.isEmpty()) {
            subject = summary;
        }
        if (data.isEmpty()) {
            data = body;
        }

        //Prioritize data over subject
        if (data.isEmpty() && !subject.isEmpty()) {
            data = subject;
            subject = "";
        }

        //Never send empty data and subject
        if (data.isEmpty() && subject.isEmpty()) {
            logger()->warn() << Q_FUNC_INFO << "Empty subject and data in dbus app " << app_name;
            return;
        }

        emit this->emailNotify(detectCleanAppname(app_name), data, subject);
    }
}
