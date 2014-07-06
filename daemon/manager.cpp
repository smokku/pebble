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
        qDebug() << "BT local name:" << btDevice.name();
        connect(dbus, SIGNAL(pebbleChanged()), SLOT(onPebbleChanged()));
        dbus->findPebble();
    }

    DBusProxy *proxy = new DBusProxy(this);
    DBusAdaptor *adaptor = new DBusAdaptor(proxy);
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

void Manager::hangupAll()
{
    foreach (VoiceCallHandler* handler, voice->voiceCalls()) {
        handler->hangup();
    }
}

void Manager::onConversationGroupAdded(GroupObject *group)
{
    if (!group) {
        qWarning() << "Got null conversation group";
        return;
    }

    connect(group, SIGNAL(unreadMessagesChanged()), SLOT(onUnreadMessagesChanged()));
    if (group->unreadMessages()) processUnreadMessages(group);
}


void Manager::onUnreadMessagesChanged()
{
    GroupObject *group = qobject_cast<GroupObject*>(sender());
    if (!group) {
        qWarning() << "Got unreadMessagesChanged for null group";
        return;
    }
    processUnreadMessages(group);
}

void Manager::processUnreadMessages(GroupObject *group)
{
    if (group->unreadMessages()) {
        QString name = group->contactName();
        QString message = group->lastMessageText();
        qDebug() << "Msg:" << message;
        qDebug() << "From:" << name;
        watch->sendSMSNotification(name.isEmpty()?"Unknown":name, message);
    } else {
        qWarning() << "Got processUnreadMessages for group with no new messages";
    }
}
