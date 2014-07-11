#include "manager.h"
#include "dbusadaptor.h"

#include <QDebug>
#include <QtContacts/QContact>
#include <QtContacts/QContactPhoneNumber>

Manager::Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice) :
    QObject(0), watch(watch), dbus(dbus), voice(voice),
    notification(MNotification::DeviceEvent)
{
    // We don't need to handle presence changes, so report them separately and ignore them
    QMap<QString, QString> parameters;
    parameters.insert(QString::fromLatin1("mergePresenceChanges"), QString::fromLatin1("false"));
    contacts = new QContactManager("", parameters, this);

    numberFilter.setDetailType(QContactDetail::TypePhoneNumber, QContactPhoneNumber::FieldNumber);
    numberFilter.setMatchFlags(QContactFilter::MatchPhoneNumber);

    conversations = new GroupManager(this);
    connect(conversations, SIGNAL(groupAdded(GroupObject*)), SLOT(onConversationGroupAdded(GroupObject*)));
    conversations->getGroups();

    connect(voice, SIGNAL(activeVoiceCallChanged()), SLOT(onActiveVoiceCallChanged()));
    connect(voice, SIGNAL(error(const QString &)), SLOT(onVoiceError(const QString &)));

    // Watch instantiated hangup, follow the orders
    connect(watch, SIGNAL(hangup()), SLOT(hangupAll()));
    connect(watch, SIGNAL(connectedChanged()), SLOT(onConnectedChanged()));

    // Set BT icon for notification
    notification.setImage("icon-system-bluetooth-device");

    if (btDevice.isValid()) {
        logger()->debug() << "BT local name:" << btDevice.name();
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
        logger()->debug() << "Pebble gone";
    } else {
        watch->deviceConnect(name, pebble["Address"].toString());
    }
}

void Manager::onConnectedChanged()
{
    QString message = QString("%1 %2")
            .arg(watch->name().isEmpty() ? "Pebble" : watch->name())
            .arg(watch->isConnected() ? "connected" : "disconnected");
    logger()->debug() << message;

    if (notification.isPublished()) notification.remove();

    notification.setBody(message);
    if (!notification.publish()) {
        logger()->debug() << "Failed publishing notification";
    }
}

void Manager::onActiveVoiceCallChanged()
{
    logger()->debug() << "Manager::onActiveVoiceCallChanged()";

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
        logger()->debug() << "ActiveVoiceCallStatusChanged but no activeVoiceCall??";
        return;
    }

    logger()->debug() << "handlerId:" << handler->handlerId()
             << "providerId:" << handler->providerId()
             << "status:" << handler->status()
             << "statusText:" << handler->statusText()
             << "lineId:" << handler->lineId()
             << "incoming:" << handler->isIncoming();

    if (!watch->isConnected()) {
        logger()->debug() << "Watch is not connected";
        return;
    }

    switch ((VoiceCallHandler::VoiceCallStatus)handler->status()) {
    case VoiceCallHandler::STATUS_ALERTING:
    case VoiceCallHandler::STATUS_DIALING:
        logger()->debug() << "Tell outgoing:" << handler->lineId();
        watch->ring(handler->lineId(), findPersonByNumber(handler->lineId()), false);
        break;
    case VoiceCallHandler::STATUS_INCOMING:
    case VoiceCallHandler::STATUS_WAITING:
        logger()->debug() << "Tell incoming:" << handler->lineId();
        watch->ring(handler->lineId(), findPersonByNumber(handler->lineId()));
        break;
    case VoiceCallHandler::STATUS_NULL:
    case VoiceCallHandler::STATUS_DISCONNECTED:
        logger()->debug() << "Endphone";
        watch->endPhoneCall();
        break;
    case VoiceCallHandler::STATUS_ACTIVE:
        logger()->debug() << "Startphone";
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
    logger()->error() << "Error: " << message;
}

void Manager::hangupAll()
{
    foreach (VoiceCallHandler* handler, voice->voiceCalls()) {
        handler->hangup();
    }
}

void Manager::onConversationGroupAdded(GroupObject *group)
{
    if (!group) {
        logger()->debug() << "Got null conversation group";
        return;
    }

    connect(group, SIGNAL(unreadMessagesChanged()), SLOT(onUnreadMessagesChanged()));
    if (group->unreadMessages()) processUnreadMessages(group);
}


void Manager::onUnreadMessagesChanged()
{
    GroupObject *group = qobject_cast<GroupObject*>(sender());
    if (!group) {
        logger()->debug() << "Got unreadMessagesChanged for null group";
        return;
    }
    processUnreadMessages(group);
}

void Manager::processUnreadMessages(GroupObject *group)
{
    if (group->unreadMessages()) {
        QString name = group->contactName();
        QString message = group->lastMessageText();
        logger()->debug() << "Msg:" << message;
        logger()->debug() << "From:" << name;
        watch->sendSMSNotification(name.isEmpty()?"Unknown":name, message);
    } else {
        logger()->debug() << "Got processUnreadMessages for group with no new messages";
    }
}
