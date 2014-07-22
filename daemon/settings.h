#ifndef SETTINGS_H
#define SETTINGS_H

#include <MDConfGroup>

class Settings : public MDConfGroup
{
    Q_OBJECT

    Q_PROPERTY(bool silentWhenConnected MEMBER silentWhenConnected NOTIFY silentWhenConnectedChanged)
    Q_PROPERTY(bool notificationsCommhistoryd MEMBER notificationsCommhistoryd NOTIFY notificationsCommhistorydChanged)
    Q_PROPERTY(bool notificationsMissedCall MEMBER notificationsMissedCall NOTIFY notificationsMissedCallChanged)
    Q_PROPERTY(bool notificationsEmails MEMBER notificationsEmails NOTIFY notificationsEmailsChanged)
    Q_PROPERTY(bool notificationsMitakuuluu MEMBER notificationsMitakuuluu NOTIFY notificationsMitakuuluuChanged)
    Q_PROPERTY(bool notificationsOther MEMBER notificationsOther NOTIFY notificationsOtherChanged)
    Q_PROPERTY(bool notificationsAll MEMBER notificationsAll NOTIFY notificationsAllChanged)
    bool silentWhenConnected;
    bool notificationsCommhistoryd;
    bool notificationsMissedCall;
    bool notificationsEmails;
    bool notificationsMitakuuluu;
    bool notificationsOther;
    bool notificationsAll;

public:
    explicit Settings(QObject *parent = 0) :
        MDConfGroup("/org/pebbled/settings", parent, BindProperties)
    { resolveMetaObject(); }

signals:
    void silentWhenConnectedChanged(bool);
    void notificationsCommhistorydChanged(bool);
    void notificationsMissedCallChanged(bool);
    void notificationsEmailsChanged(bool);
    void notificationsMitakuuluuChanged(bool);
    void notificationsOtherChanged(bool);
    void notificationsAllChanged(bool);

public slots:

};

#endif // SETTINGS_H
