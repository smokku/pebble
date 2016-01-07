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

NotificationManager::NotificationManager(Settings *settings, QObject *parent)
    : QObject(parent), l(metaObject()->className()), d_ptr(new NotificationManagerPrivate(this)), settings(settings)
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

QString NotificationManager::getCleanAppName(QString app_name)
{
    QString desktopFile = QString("/usr/share/applications/%1.desktop").arg(app_name);
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

QStringHash NotificationManager::getCategoryParams(QString category)
{
    if (!category.isEmpty()) {
        QString categoryConfigFile = QString("/usr/share/lipstick/notificationcategories/%1.conf").arg(category);
        QFile testFile(categoryConfigFile);
        if (testFile.exists()) {
            QStringHash categories;
            QSettings settings(categoryConfigFile, QSettings::IniFormat);
            const QStringList settingKeys = settings.allKeys();
            foreach (const QString &settingKey, settingKeys) {
                categories[settingKey] = settings.value(settingKey).toString();
            }
            return categories;
        }
    }
    return QStringHash();
}

uint NotificationManager::Notify(const QString &app_name, uint replaces_id, const QString &app_icon,
                                 const QString &summary, const QString &body, const QStringList &actions, const QVariantHash &hints, int expire_timeout)
{
    Q_UNUSED(replaces_id);
    Q_UNUSED(app_icon);
    Q_UNUSED(actions);
    Q_UNUSED(expire_timeout);

    // new place to check notification owner in Sailfish 1.1.6
    QString owner = hints.value("x-nemo-owner", "none").toString();

    // Ignore transient notifcations
    if (hints.value("transient", false).toBool()) {
        qCDebug(l) << "Ignoring transient notification from " << owner;
        return 0;
    }

    qCDebug(l) << Q_FUNC_INFO  << "Got notification via dbus from" << this->getCleanAppName(app_name) << " Owner: " << owner;
    qCDebug(l) << hints;

    // Avoid sending a reply for this method call, since we've received it because we're eavesdropping.
    // The actual target of the method call will send the proper reply.
    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);

    if (app_name == "messageserver5" || owner == "messageserver5") {
        if (!settings->property("notificationsEmails").toBool()) {
            qCDebug(l) << "Ignoring email notification because of setting!";
            return 0;
        }

        QString subject = hints.value("x-nemo-preview-summary", "").toString();
        QString data = hints.value("x-nemo-preview-body", "").toString();

        // Prioritize subject over data
        if (subject.isEmpty() && !data.isEmpty()) {
            subject = data;
            data = "";
        }

        if (!subject.isEmpty()) {
            emit this->emailNotify(subject, data, "");
        }
    } else if (app_name == "commhistoryd" || owner == "commhistoryd") {
        if (summary == "" && body == "") {
            QString category = hints.value("category", "").toString();

            if (category == "x-nemo.call.missed") {
                if (!settings->property("notificationsMissedCall").toBool()) {
                    qCDebug(l) << "Ignoring MissedCall notification because of setting!";
                    return 0;
                }
            } else {
                if (!settings->property("notificationsCommhistoryd").toBool()) {
                    qCDebug(l) << "Ignoring commhistoryd notification because of setting!";
                    return 0;
                }
            }
            emit this->smsNotify(hints.value("x-nemo-preview-summary", "default").toString(),
                                 hints.value("x-nemo-preview-body", "default").toString()
                                );
        }
    } else if (app_name == "harbour-mitakuuluu2-server" || owner == "harbour-mitakuuluu2-server") {
        if (!settings->property("notificationsMitakuuluu").toBool()) {
            qCDebug(l) << "Ignoring mitakuuluu notification because of setting!";
            return 0;
        }

        emit this->smsNotify(hints.value("x-nemo-preview-body", "default").toString(),
                             hints.value("x-nemo-preview-summary", "default").toString()
                            );
    } else if (app_name == "twitter-notifications-client" || owner == "twitter-notifications-client") {
        if (!settings->property("notificationsTwitter").toBool()) {
            qCDebug(l) << "Ignoring twitter notification because of setting!";
            return 0;
        }

        emit this->twitterNotify(hints.value("x-nemo-preview-body", body).toString(),
                                 hints.value("x-nemo-preview-summary", summary).toString()
                                );
    } else {
        // Prioritize x-nemo-preview* over dbus direct summary and body
        QString subject = hints.value("x-nemo-preview-summary", "").toString();
        QString data = hints.value("x-nemo-preview-body", "").toString();
        QString category = hints.value("category", "").toString();
        QStringHash categoryParams = this->getCategoryParams(category);
        int prio = categoryParams.value("x-nemo-priority", "0").toInt();

        qCDebug(l) << "MSG Prio:" << prio;

        if (!settings->property("notificationsAll").toBool() && prio <= 10) {
            qCDebug(l) << "Ignoring notification because of setting! (all)";
            return 0;
        }

        if (!settings->property("notificationsOther").toBool() && prio < 90) {
            qCDebug(l) << "Ignoring notification because of setting! (other)";
            return 0;
        }

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
            qCWarning(l) << Q_FUNC_INFO << "Empty subject and data in dbus app:" << app_name;
            return 0;
        }

        emit this->emailNotify(this->getCleanAppName(app_name), data, subject);
    }

    return 0;
}
