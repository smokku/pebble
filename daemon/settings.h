#ifndef SETTINGS_H
#define SETTINGS_H

#include <MDConfGroup>

class Settings : public MDConfGroup
{
    Q_OBJECT

    Q_PROPERTY(bool silentWhenConnected MEMBER silentWhenConnected NOTIFY silentWhenConnectedChanged)
    Q_PROPERTY(bool transliterateMessage MEMBER transliterateMessage NOTIFY transliterateMessageChanged)
    Q_PROPERTY(bool incomingCallNotification MEMBER incomingCallNotification NOTIFY incomingCallNotificationChanged)
    Q_PROPERTY(bool notificationsCommhistoryd MEMBER notificationsCommhistoryd NOTIFY notificationsCommhistorydChanged)
    Q_PROPERTY(bool notificationsMissedCall MEMBER notificationsMissedCall NOTIFY notificationsMissedCallChanged)
    Q_PROPERTY(bool notificationsEmails MEMBER notificationsEmails NOTIFY notificationsEmailsChanged)
    Q_PROPERTY(bool notificationsMitakuuluu MEMBER notificationsMitakuuluu NOTIFY notificationsMitakuuluuChanged)
    Q_PROPERTY(bool notificationsTwitter MEMBER notificationsTwitter NOTIFY notificationsTwitterChanged)
    Q_PROPERTY(bool notificationsFacebook MEMBER notificationsFacebook NOTIFY notificationsFacebookChanged)
    Q_PROPERTY(bool notificationsOther MEMBER notificationsOther NOTIFY notificationsOtherChanged)
    Q_PROPERTY(bool notificationsAll MEMBER notificationsAll NOTIFY notificationsAllChanged)
    Q_PROPERTY(QString accountToken MEMBER accountToken NOTIFY accountTokenChanged)

    bool silentWhenConnected;
    bool transliterateMessage;
    bool incomingCallNotification;
    bool notificationsCommhistoryd;
    bool notificationsMissedCall;
    bool notificationsEmails;
    bool notificationsMitakuuluu;
    bool notificationsTwitter;
    bool notificationsFacebook;
    bool notificationsOther;
    bool notificationsAll;
    QString accountToken;

public:
    explicit Settings(QObject *parent = 0) :
        MDConfGroup("/org/pebbled/settings", parent, BindProperties)
    { resolveMetaObject(); }

signals:
    void silentWhenConnectedChanged();
    void transliterateMessageChanged();
    void incomingCallNotificationChanged();
    void notificationsCommhistorydChanged();
    void notificationsMissedCallChanged();
    void notificationsEmailsChanged();
    void notificationsMitakuuluuChanged();
    void notificationsTwitterChanged();
    void notificationsFacebookChanged();
    void notificationsOtherChanged();
    void notificationsAllChanged();
    void accountTokenChanged();
};

#endif // SETTINGS_H
