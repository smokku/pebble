#include "manager.h"
#include "dbusadaptor.h"

#include <QDebug>
#include <QtContacts/QContact>
#include <QtContacts/QContactPhoneNumber>

Manager::Manager(watch::WatchConnector *watch, DBusConnector *dbus, VoiceCallManager *voice, NotificationManager *notifications, Settings *settings) :
    QObject(0), watch(watch), dbus(dbus), voice(voice), notifications(notifications), commands(new WatchCommands(watch, this)),
    settings(settings), notification(MNotification::DeviceEvent)
{
    connect(settings, SIGNAL(valueChanged(QString)), SLOT(onSettingChanged(const QString&)));
    connect(settings, SIGNAL(valuesChanged()), SLOT(onSettingsChanged()));
    //connect(settings, SIGNAL(silentWhenConnectedChanged(bool)), SLOT(onSilentWhenConnectedChanged(bool)));

    // We don't need to handle presence changes, so report them separately and ignore them
    QMap<QString, QString> parameters;
    parameters.insert(QString::fromLatin1("mergePresenceChanges"), QString::fromLatin1("false"));
    contacts = new QContactManager("", parameters, this);

    numberFilter.setDetailType(QContactDetail::TypePhoneNumber, QContactPhoneNumber::FieldNumber);
    numberFilter.setMatchFlags(QContactFilter::MatchPhoneNumber);

    connect(watch, SIGNAL(connectedChanged()), SLOT(onConnectedChanged()));

    connect(voice, SIGNAL(activeVoiceCallChanged()), SLOT(onActiveVoiceCallChanged()));
    connect(voice, SIGNAL(error(const QString &)), SLOT(onVoiceError(const QString &)));

    connect(notifications, SIGNAL(error(const QString &)), SLOT(onNotifyError(const QString &)));
    connect(notifications, SIGNAL(emailNotify(const QString &,const QString &,const QString &)), SLOT(onEmailNotify(const QString &,const QString &,const QString &)));
    connect(notifications, SIGNAL(smsNotify(const QString &,const QString &)), SLOT(onSmsNotify(const QString &,const QString &)));
    connect(notifications, SIGNAL(twitterNotify(const QString &,const QString &)), SLOT(onTwitterNotify(const QString &,const QString &)));
    connect(notifications, SIGNAL(facebookNotify(const QString &,const QString &)), SLOT(onFacebookNotify(const QString &,const QString &)));

    connect(watch, SIGNAL(messageDecoded(uint,QByteArray)), commands, SLOT(processMessage(uint,QByteArray)));
    connect(commands, SIGNAL(hangup()), SLOT(hangupAll()));

    PebbledProxy *proxy = new PebbledProxy(this);
    PebbledAdaptor *adaptor = new PebbledAdaptor(proxy);
    QDBusConnection session = QDBusConnection::sessionBus();
    session.registerObject("/", proxy);
    session.registerService("org.pebbled");
    connect(dbus, SIGNAL(pebbleChanged()), adaptor, SIGNAL(pebbleChanged()));
    connect(watch, SIGNAL(connectedChanged()), adaptor, SIGNAL(connectedChanged()));

    QString currentProfile = getCurrentProfile();
    defaultProfile = currentProfile.isEmpty() ? "ambience" : currentProfile;
    connect(watch, SIGNAL(connectedChanged()), SLOT(applyProfile()));

    // Music Control interface
    session.connect("", "/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties", "PropertiesChanged",
                this, SLOT(onMprisPropertiesChanged(QString,QMap<QString,QVariant>,QStringList)));

    connect(this, SIGNAL(mprisMetadataChanged(QVariantMap)), commands, SLOT(onMprisMetadataChanged(QVariantMap)));

    // Set BT icon for notification
    notification.setImage("icon-system-bluetooth-device");

    if (btDevice.isValid()) {
        logger()->debug() << "BT local name:" << btDevice.name();
        connect(dbus, SIGNAL(pebbleChanged()), SLOT(onPebbleChanged()));
        dbus->findPebble();
    }

}

void Manager::onSettingChanged(const QString &key)
{
    logger()->debug() << __FUNCTION__ << key << ":" << settings->property(qPrintable(key));
}

void Manager::onSettingsChanged()
{
    logger()->warn() << __FUNCTION__ << "Not implemented!";
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

    if (watch->isConnected()) {
        QString mpris = this->mpris();
        if (not mpris.isEmpty()) {
            QDBusReply<QDBusVariant> Metadata = QDBusConnection::sessionBus().call(
                        QDBusMessage::createMethodCall(mpris, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get")
                        << "org.mpris.MediaPlayer2.Player" << "Metadata");
            if (Metadata.isValid()) {
                setMprisMetadata(Metadata.value().variant().value<QDBusArgument>());
            }
            else {
                logger()->error() << Metadata.error().message();
                setMprisMetadata(QVariantMap());
            }
        }
    }
}

void Manager::onActiveVoiceCallChanged()
{
    logger()->debug() << "Manager::onActiveVoiceCallChanged()";

    QVariant incomingCallNotification = settings->property("incomingCallNotification");
    if (incomingCallNotification.isValid() && !incomingCallNotification.toBool()) {
        logger()->debug() << "Ignoring ActiveVoiceCallChanged because of setting!";
        return;
    }

    VoiceCallHandler* handler = voice->activeVoiceCall();
    if (handler) {
        connect(handler, SIGNAL(statusChanged()), SLOT(onActiveVoiceCallStatusChanged()));
        connect(handler, SIGNAL(destroyed()), SLOT(onActiveVoiceCallStatusChanged()));
        return;
    }
}

void Manager::onActiveVoiceCallStatusChanged()
{
    VoiceCallHandler* handler = voice->activeVoiceCall();
    if (!handler) {
        logger()->debug() << "ActiveVoiceCall destroyed";
        watch->endPhoneCall();
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
    QString person;
    numberFilter.setValue(number);

    const QList<QContact> &found = contacts->contacts(numberFilter);
    if (found.size() == 1) {
        person = found[0].detail(QContactDetail::TypeDisplayLabel).value(0).toString();
    }

    if (settings->property("transliterateMessage").toBool()) {
        transliterateMessage(person);
    }
    return person;
}

void Manager::onVoiceError(const QString &message)
{
    logger()->error() << "Error:" << message;
}


void Manager::onNotifyError(const QString &message)
{
    qWarning() << "Error:" << message;
}

void Manager::onSmsNotify(const QString &sender, const QString &data)
{
    if (settings->property("transliterateMessage").toBool()) {
        transliterateMessage(sender);
        transliterateMessage(data);
    }
    watch->sendSMSNotification(sender, data);
}

void Manager::onTwitterNotify(const QString &sender, const QString &data)
{
    if (settings->property("transliterateMessage").toBool()) {
        transliterateMessage(sender);
        transliterateMessage(data);
    }
    watch->sendTwitterNotification(sender, data);
}


void Manager::onFacebookNotify(const QString &sender, const QString &data)
{
    if (settings->property("transliterateMessage").toBool()) {
        transliterateMessage(sender);
        transliterateMessage(data);
    }
    watch->sendFacebookNotification(sender, data);
}


void Manager::onEmailNotify(const QString &sender, const QString &data,const QString &subject)
{
    if (settings->property("transliterateMessage").toBool()) {
        transliterateMessage(sender);
        transliterateMessage(data);
        transliterateMessage(subject);
    }
    watch->sendEmailNotification(sender, data, subject);
}

void Manager::hangupAll()
{
    foreach (VoiceCallHandler* handler, voice->voiceCalls()) {
        handler->hangup();
    }
}

void Manager::onMprisPropertiesChanged(QString interface, QMap<QString,QVariant> changed, QStringList invalidated)
{
    logger()->debug() << interface << changed << invalidated;

    if (changed.contains("Metadata")) {
        setMprisMetadata(changed.value("Metadata").value<QDBusArgument>());
    }

    if (changed.contains("PlaybackStatus")) {
        QString PlaybackStatus = changed.value("PlaybackStatus").toString();
        if (PlaybackStatus == "Stopped") {
            setMprisMetadata(QVariantMap());
        }
    }

    lastSeenMpris = message().service();
    logger()->debug() << "lastSeenMpris:" << lastSeenMpris;
}

QString Manager::mpris()
{
    const QStringList &services = dbus->services();
    if (not lastSeenMpris.isEmpty() && services.contains(lastSeenMpris))
        return lastSeenMpris;

    foreach (QString service, services)
        if (service.startsWith("org.mpris.MediaPlayer2."))
            return service;

    return QString();
}

void Manager::setMprisMetadata(QDBusArgument metadata)
{
    if (metadata.currentType() == QDBusArgument::MapType) {
        metadata >> mprisMetadata;
        emit mprisMetadataChanged(mprisMetadata);
    }
}

void Manager::setMprisMetadata(QVariantMap metadata)
{
    mprisMetadata = metadata;
    emit mprisMetadataChanged(mprisMetadata);
}

QString Manager::getCurrentProfile()
{
    QDBusReply<QString> profile = QDBusConnection::sessionBus().call(
                QDBusMessage::createMethodCall("com.nokia.profiled", "/com/nokia/profiled", "com.nokia.profiled", "get_profile"));
    if (profile.isValid()) {
        QString currentProfile = profile.value();
        logger()->debug() << "Got profile" << currentProfile;
        return currentProfile;
    }

    logger()->error() << profile.error().message();
    return QString();
}

void Manager::applyProfile()
{
    QString currentProfile = getCurrentProfile();
    QString newProfile;

    if (settings->property("silentWhenConnected").toBool()) {
        if (watch->isConnected() && currentProfile != "silent") {
            newProfile = "silent";
            defaultProfile = currentProfile;
        }
        if (!watch->isConnected() && currentProfile == "silent" && defaultProfile != "silent") {
            newProfile = defaultProfile;
        }
    }
    else if (currentProfile != defaultProfile) {
        newProfile = defaultProfile;
    }

    if (!newProfile.isEmpty()) {
        QDBusReply<bool> res = QDBusConnection::sessionBus().call(
                    QDBusMessage::createMethodCall("com.nokia.profiled", "/com/nokia/profiled", "com.nokia.profiled", "set_profile")
                    << newProfile);
        if (res.isValid()) {
            if (!res.value()) {
                logger()->error() << "Unable to set profile" << newProfile;
            }
        }
        else {
            logger()->error() << res.error().message();
        }
    }
}

void Manager::transliterateMessage(const QString &text)
{
    if (transliterator.isNull()) {
        UErrorCode status = U_ZERO_ERROR;
        transliterator.reset(icu::Transliterator::createInstance(icu::UnicodeString::fromUTF8("Any-Latin; Latin-ASCII"),UTRANS_FORWARD, status));
        if (U_FAILURE(status)) {
            logger()->warn() << "Error creaing ICU Transliterator \"Any-Latin; Latin-ASCII\":" << u_errorName(status);
        }
    }
    if (!transliterator.isNull()) {
        logger()->debug() << "String before transliteration:" << text;

        icu::UnicodeString uword = icu::UnicodeString::fromUTF8(text.toStdString());
        transliterator->transliterate(uword);

        std::string translited;
        uword.toUTF8String(translited);

        const_cast<QString&>(text) = QString::fromStdString(translited);
        logger()->debug() << "String after transliteration:" << text;
    }
}
