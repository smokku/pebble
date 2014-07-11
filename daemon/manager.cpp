#include "manager.h"
#include "dbusadaptor.h"

#include <QDebug>
#include <QtContacts/QContact>
#include <QtContacts/QContactPhoneNumber>

Manager::Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice, NotificationManager *notifications) :
    QObject(0), watch(watch), dbus(dbus), voice(voice), notifications(notifications),
    notification(MNotification::DeviceEvent)
{
    // We don't need to handle presence changes, so report them separately and ignore them
    QMap<QString, QString> parameters;
    parameters.insert(QString::fromLatin1("mergePresenceChanges"), QString::fromLatin1("false"));
    contacts = new QContactManager("", parameters, this);

    numberFilter.setDetailType(QContactDetail::TypePhoneNumber, QContactPhoneNumber::FieldNumber);
    numberFilter.setMatchFlags(QContactFilter::MatchPhoneNumber);

    connect(voice, SIGNAL(activeVoiceCallChanged()), SLOT(onActiveVoiceCallChanged()));
    connect(voice, SIGNAL(error(const QString &)), SLOT(onVoiceError(const QString &)));

    connect(notifications, SIGNAL(error(const QString &)), SLOT(onNotifyError(const QString &)));
    connect(notifications, SIGNAL(emailNotify(const QString &,const QString &,const QString &)), SLOT(onEmailNotify(const QString &,const QString &,const QString &)));
    connect(notifications, SIGNAL(smsNotify(const QString &,const QString &)), SLOT(onSmsNotify(const QString &,const QString &)));

    // Watch instantiated hangup, follow the orders
    connect(watch, SIGNAL(hangup()), SLOT(hangupAll()));
    connect(watch, SIGNAL(connectedChanged()), SLOT(onConnectedChanged()));

    // Set BT icon for notification
    notification.setImage("icon-system-bluetooth-device");

    if (btDevice.isValid()) {
        qDebug() << "BT local name:" << btDevice.name();
        connect(dbus, SIGNAL(pebbleChanged()), SLOT(onPebbleChanged()));
        dbus->findPebble();
    }

    PebbledProxy *proxy = new PebbledProxy(this);
    PebbledAdaptor *adaptor = new PebbledAdaptor(proxy);
    QDBusConnection connection = QDBusConnection::sessionBus();
    connection.registerObject("/", proxy);
    connection.registerService("org.pebbled");
    connect(dbus, SIGNAL(pebbleChanged()), adaptor, SIGNAL(pebbleChanged()));
    connect(watch, SIGNAL(connectedChanged()), adaptor, SIGNAL(connectedChanged()));
}

void Manager::onPebbleChanged()
{
    const QVariantMap & pebble = dbus->pebble();
    QString name = pebble["Name"].toString();
    if (name.isEmpty()) {
        qDebug() << "Pebble gone";
    } else {
        watch->deviceConnect(name, pebble["Address"].toString());
    }
}

void Manager::onConnectedChanged()
{
    QString message = QString("%1 %2")
            .arg(watch->name().isEmpty() ? "Pebble" : watch->name())
            .arg(watch->isConnected() ? "connected" : "disconnected");
    qDebug() << message;

    if (notification.isPublished()) notification.remove();

    notification.setBody(message);
    if (!notification.publish()) {
        qDebug() << "Failed publishing notification";
    }
}

void Manager::onActiveVoiceCallChanged()
{
    qDebug() << "Manager::onActiveVoiceCallChanged()";

    VoiceCallHandler* handler = voice->activeVoiceCall();
    if (handler) {
        connect(handler, SIGNAL(statusChanged()), SLOT(onActiveVoiceCallStatusChanged()));
        return;
    }
}

void Manager::onActiveVoiceCallStatusChanged()
{
    VoiceCallHandler* handler = voice->activeVoiceCall();
    if (!handler) {
        qWarning() << "ActiveVoiceCallStatusChanged but no activeVoiceCall??";
        return;
    }

    qDebug() << "handlerId:" << handler->handlerId()
             << "providerId:" << handler->providerId()
             << "status:" << handler->status()
             << "statusText:" << handler->statusText()
             << "lineId:" << handler->lineId()
             << "incoming:" << handler->isIncoming();

    if (!watch->isConnected()) {
        qDebug() << "Watch is not connected";
        return;
    }

    switch ((VoiceCallHandler::VoiceCallStatus)handler->status()) {
    case VoiceCallHandler::STATUS_ALERTING:
    case VoiceCallHandler::STATUS_DIALING:
        qDebug() << "Tell outgoing:" << handler->lineId();
        watch->ring(handler->lineId(), findPersonByNumber(handler->lineId()), false);
        break;
    case VoiceCallHandler::STATUS_INCOMING:
    case VoiceCallHandler::STATUS_WAITING:
        qDebug() << "Tell incoming:" << handler->lineId();
        watch->ring(handler->lineId(), findPersonByNumber(handler->lineId()));
        break;
    case VoiceCallHandler::STATUS_NULL:
    case VoiceCallHandler::STATUS_DISCONNECTED:
        qDebug() << "Endphone";
        watch->endPhoneCall();
        break;
    case VoiceCallHandler::STATUS_ACTIVE:
        qDebug() << "Startphone";
        watch->startPhoneCall();
        break;
    case VoiceCallHandler::STATUS_HELD:
        break;
    }
}

QString Manager::findPersonByNumber(QString number)
{
    numberFilter.setValue(number);

    const QList<QContact> &found = contacts->contacts(numberFilter);
    if (found.size() == 1) {
        return found[0].detail(QContactDetail::TypeDisplayLabel).value(0).toString();
    }

    return QString();
}

void Manager::onVoiceError(const QString &message)
{
    qWarning() << "Error: " << message;
}


void Manager::onNotifyError(const QString &message)
{
    qWarning() << "Error: " << message;
}

void Manager::onSmsNotify(const QString &sender, const QString &data)
{
    qDebug() << "SMS:";
    qDebug() << sender;
    qDebug() << data;
    watch->sendSMSNotification(sender, data);
}

void Manager::onEmailNotify(const QString &sender, const QString &data,const QString &subject)
{
    qDebug() << "Email:";
    qDebug() << sender;
    qDebug() << data;
    qDebug() << subject;
    watch->sendEmailNotification(sender, data, subject);
}

void Manager::hangupAll()
{
    foreach (VoiceCallHandler* handler, voice->voiceCalls()) {
        handler->hangup();
    }
}
