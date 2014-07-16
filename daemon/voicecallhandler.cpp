#include "voicecallhandler.h"

#include <QDebug>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QVariantMap>

/*!
  \class VoiceCallHandler
  \brief This is the D-Bus proxy for communicating with the voice call manager
    from a declarative context, this interface specifically interfaces with
    the managers' voice call handler instances.
*/
class VoiceCallHandlerPrivate
{
    Q_DECLARE_PUBLIC(VoiceCallHandler)

public:
    VoiceCallHandlerPrivate(VoiceCallHandler *q, const QString &pHandlerId)
        : q_ptr(q), handlerId(pHandlerId), interface(NULL)
        , duration(0), status(0), emergency(false), multiparty(false), forwarded(false)
    { /* ... */ }

    VoiceCallHandler *q_ptr;

    QString handlerId;

    QDBusInterface *interface;

    int duration;
    int status;
    QString statusText;
    QString lineId;
    QString providerId;
    QDateTime startedAt;
    bool emergency;
    bool multiparty;
    bool forwarded;
};

/*!
  Constructs a new proxy interface for the provided voice call handlerId.
*/
VoiceCallHandler::VoiceCallHandler(const QString &handlerId, QObject *parent)
    : QObject(parent), d_ptr(new VoiceCallHandlerPrivate(this, handlerId))
{
    Q_D(VoiceCallHandler);
    logger()->debug() << QString("Creating D-Bus interface to: ") + handlerId;
    d->interface = new QDBusInterface("org.nemomobile.voicecall",
                                      "/calls/" + handlerId,
                                      "org.nemomobile.voicecall.VoiceCall",
                                      QDBusConnection::sessionBus(),
                                      this);
    this->initialize(true);
}

VoiceCallHandler::~VoiceCallHandler()
{
    Q_D(VoiceCallHandler);
    delete d;
}

void VoiceCallHandler::initialize(bool notifyError)
{
    Q_D(VoiceCallHandler);

/*
method return sender=:1.13 -> dest=:1.150 reply_serial=2
   string "<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
<node>
  <interface name="org.nemomobile.voicecall.VoiceCall">
    <property name="handlerId" type="s" access="read"/>
    <property name="providerId" type="s" access="read"/>
    <property name="status" type="i" access="read"/>
    <property name="statusText" type="s" access="read"/>
    <property name="lineId" type="s" access="read"/>
    <property name="startedAt" type="((iii)(iiii)i)" access="read">
      <annotation name="org.qtproject.QtDBus.QtTypeName" value="QDateTime"/>
    </property>
    <property name="duration" type="i" access="read"/>
    <property name="isIncoming" type="b" access="read"/>
    <property name="isEmergency" type="b" access="read"/>
    <property name="isMultiparty" type="b" access="read"/>
    <property name="isForwarded" type="b" access="read"/>
    <signal name="error">
      <arg name="message" type="s" direction="out"/>
    </signal>
    <signal name="statusChanged">
    </signal>
    <signal name="lineIdChanged">
    </signal>
    <signal name="startedAtChanged">
    </signal>
    <signal name="durationChanged">
    </signal>
    <signal name="emergencyChanged">
    </signal>
    <signal name="multipartyChanged">
    </signal>
    <signal name="forwardedChanged">
    </signal>
    <method name="answer">
      <arg type="b" direction="out"/>
    </method>
    <method name="hangup">
      <arg type="b" direction="out"/>
    </method>
    <method name="hold">
      <arg type="b" direction="out"/>
      <arg name="on" type="b" direction="in"/>
    </method>
    <method name="deflect">
      <arg type="b" direction="out"/>
      <arg name="target" type="s" direction="in"/>
    </method>
    <method name="sendDtmf">
      <arg name="tones" type="s" direction="in"/>
    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="Get">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="property_name" type="s" direction="in"/>
      <arg name="value" type="v" direction="out"/>
    </method>
    <method name="Set">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="property_name" type="s" direction="in"/>
      <arg name="value" type="v" direction="in"/>
    </method>
    <method name="GetAll">
      <arg name="interface_name" type="s" direction="in"/>
      <arg name="values" type="a{sv}" direction="out"/>
      <annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="QVariantMap"/>
    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Introspectable">
    <method name="Introspect">
      <arg name="xml_data" type="s" direction="out"/>
    </method>
  </interface>
  <interface name="org.freedesktop.DBus.Peer">
    <method name="Ping"/>
    <method name="GetMachineId">
      <arg name="machine_uuid" type="s" direction="out"/>
    </method>
  </interface>
</node>
"
*/

    if (d->interface->isValid()) {
        if (getProperties()) {
            emit durationChanged();
            emit statusChanged();
            emit lineIdChanged();
            emit startedAtChanged();
            emit multipartyChanged();
            emit emergencyChanged();
            emit forwardedChanged();

            connect(d->interface, SIGNAL(error(QString)), SIGNAL(error(QString)));
            connect(d->interface, SIGNAL(statusChanged()), SLOT(onStatusChanged()));
            connect(d->interface, SIGNAL(lineIdChanged()), SLOT(onLineIdChanged()));
            connect(d->interface, SIGNAL(durationChanged()), SLOT(onDurationChanged()));
            connect(d->interface, SIGNAL(startedAtChanged()), SLOT(onStartedAtChanged()));
            connect(d->interface, SIGNAL(emergencyChanged()), SLOT(onEmergencyChanged()));
            connect(d->interface, SIGNAL(multipartyChanged()), SLOT(onMultipartyChanged()));
            connect(d->interface, SIGNAL(forwardedChanged()), SLOT(onForwardedChanged()));
        }
        else {
            if (notifyError) emit this->error("Failed to get VoiceCall properties from VCM D-Bus service.");
        }
    }
    else {
        logger()->error() << d->interface->lastError().name() << d->interface->lastError().message();
    }
}

bool VoiceCallHandler::getProperties()
{
    Q_D(VoiceCallHandler);

    QDBusInterface props(d->interface->service(), d->interface->path(),
                         "org.freedesktop.DBus.Properties", d->interface->connection());

    QDBusReply<QVariantMap> reply = props.call("GetAll", d->interface->interface());
    if (reply.isValid()) {
        QVariantMap props = reply.value();
        logger()->debug() << props;
        d->providerId = props["providerId"].toString();
        d->duration = props["duration"].toInt();
        d->status = props["status"].toInt();
        d->statusText = props["statusText"].toString();
        d->lineId = props["lineId"].toString();
        d->startedAt = QDateTime::fromMSecsSinceEpoch(props["startedAt"].toULongLong());
        d->multiparty = props["isMultiparty"].toBool();
        d->emergency = props["isEmergency"].toBool();
        d->forwarded = props["isForwarded"].toBool();
        return true;
    }
    else {
        logger()->error() << "Failed to get VoiceCall properties from VCM D-Bus service.";
        return false;
    }
}

void VoiceCallHandler::onDurationChanged()
{
    Q_D(VoiceCallHandler);
    d->duration = d->interface->property("duration").toInt();
    emit durationChanged();
}

void VoiceCallHandler::onStatusChanged()
{
    // a) initialize() might returned crap with STATUS_NULL (no lineId)
    // b) we need to fetch two properties "status" and "statusText"
    //    so, we might aswell get them all
    getProperties();
    emit statusChanged();
}

void VoiceCallHandler::onLineIdChanged()
{
    Q_D(VoiceCallHandler);
    d->lineId = d->interface->property("lineId").toString();
    emit lineIdChanged();
}

void VoiceCallHandler::onStartedAtChanged()
{
    Q_D(VoiceCallHandler);
    d->startedAt = d->interface->property("startedAt").toDateTime();
    emit startedAtChanged();
}

void VoiceCallHandler::onEmergencyChanged()
{
    Q_D(VoiceCallHandler);
    d->emergency = d->interface->property("isEmergency").toBool();
    emit emergencyChanged();
}

void VoiceCallHandler::onMultipartyChanged()
{
    Q_D(VoiceCallHandler);
    d->multiparty = d->interface->property("isMultiparty").toBool();
    emit multipartyChanged();
}

void VoiceCallHandler::onForwardedChanged()
{
    Q_D(VoiceCallHandler);
    d->forwarded = d->interface->property("isForwarded").toBool();
    emit forwardedChanged();
}

/*!
  Returns this voice calls' handler id.
 */
QString VoiceCallHandler::handlerId() const
{
    Q_D(const VoiceCallHandler);
    return d->handlerId;
}

/*!
  Returns this voice calls' provider id.
 */
QString VoiceCallHandler::providerId() const
{
    Q_D(const VoiceCallHandler);
    return d->providerId;
}

/*!
  Returns this voice calls' call status.
 */
int VoiceCallHandler::status() const
{
    Q_D(const VoiceCallHandler);
    return d->status;
}

/*!
  Returns this voice calls' call status as a symbolic string.
 */
QString VoiceCallHandler::statusText() const
{
    Q_D(const VoiceCallHandler);
    return d->statusText;
}

/*!
  Returns this voice calls' remote end-point line id.
 */
QString VoiceCallHandler::lineId() const
{
    Q_D(const VoiceCallHandler);
    return d->lineId;
}

/*!
  Returns this voice calls' started at property.
 */
QDateTime VoiceCallHandler::startedAt() const
{
    Q_D(const VoiceCallHandler);
    return d->startedAt;
}

/*!
  Returns this voice calls' duration property.
 */
int VoiceCallHandler::duration() const
{
    Q_D(const VoiceCallHandler);
    return d->duration;
}

/*!
  Returns this voice calls' incoming call flag property.
 */
bool VoiceCallHandler::isIncoming() const
{
    Q_D(const VoiceCallHandler);
    return d->interface->property("isIncoming").toBool();
}

/*!
  Returns this voice calls' multiparty flag property.
 */
bool VoiceCallHandler::isMultiparty() const
{
    Q_D(const VoiceCallHandler);
    return d->multiparty;
}

/*!
  Returns this voice calls' forwarded flag property.
 */
bool VoiceCallHandler::isForwarded() const
{
    Q_D(const VoiceCallHandler);
    return d->forwarded;
}

/*!
  Returns this voice calls' emergency flag property.
 */
bool VoiceCallHandler::isEmergency() const
{
    Q_D(const VoiceCallHandler);
    return d->emergency;
}

/*!
  Initiates answering this call, if the call is an incoming call.
 */
void VoiceCallHandler::answer()
{
    Q_D(VoiceCallHandler);
    QDBusPendingCall call = d->interface->asyncCall("answer");

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(onPendingCallFinished(QDBusPendingCallWatcher*)));
}

/*!
  Initiates droping the call, unless the call is disconnected.
 */
void VoiceCallHandler::hangup()
{
    Q_D(VoiceCallHandler);
    QDBusPendingCall call = d->interface->asyncCall("hangup");

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(onPendingCallFinished(QDBusPendingCallWatcher*)));
}

/*!
  Initiates holding the call, unless the call is disconnected.
 */
void VoiceCallHandler::hold(bool on)
{
    Q_D(VoiceCallHandler);
    QDBusPendingCall call = d->interface->asyncCall("hold", on);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(onPendingCallFinished(QDBusPendingCallWatcher*)));
}

/*!
  Initiates deflecting the call to the provided target phone number.
 */
void VoiceCallHandler::deflect(const QString &target)
{
    Q_D(VoiceCallHandler);
    QDBusPendingCall call = d->interface->asyncCall("deflect", target);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(onPendingCallFinished(QDBusPendingCallWatcher*)));
}

void VoiceCallHandler::sendDtmf(const QString &tones)
{
    Q_D(VoiceCallHandler);
    QDBusPendingCall call = d->interface->asyncCall("sendDtmf", tones);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(onPendingCallFinished(QDBusPendingCallWatcher*)));
}

void VoiceCallHandler::onPendingCallFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<bool> reply = *watcher;

    if (reply.isError()) {
        logger()->error() << QString::fromLatin1("Received error reply for member: %1 (%2)").arg(reply.reply().member()).arg(reply.error().message());
        emit this->error(reply.error().message());
        watcher->deleteLater();
    } else {
        logger()->debug() << QString::fromLatin1("Received successful reply for member: %1").arg(reply.reply().member());
    }
}
