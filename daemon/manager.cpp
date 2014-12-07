#include <QDebug>
#include <QtContacts/QContact>
#include <QtContacts/QContactPhoneNumber>

#include "manager.h"
#include "watch_adaptor.h"

Manager::Manager(Settings *settings, QObject *parent) :
    QObject(parent), settings(settings),
    proxy(new PebbledProxy(this)),
    watch(new WatchConnector(this)),
    dbus(new DBusConnector(this)),
    upload(new UploadManager(watch, this)),
    apps(new AppManager(this)),
    bank(new BankManager(watch, upload, apps, this)),
    voice(new VoiceCallManager(settings, this)),
    notifications(new NotificationManager(settings, this)),
    music(new MusicManager(watch, this)),
    datalog(new DataLogManager(watch, this)),
    appmsg(new AppMsgManager(apps, watch, this)),
    js(new JSKitManager(apps, appmsg, this)),
    notification(MNotification::DeviceEvent)
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
    watch->setEndpointHandler(WatchConnector::watchPHONE_VERSION,
                              [this](const QByteArray& data) {
        Q_UNUSED(data);
        watch->sendPhoneVersion();
        return true;
    });
    watch->setEndpointHandler(WatchConnector::watchPHONE_CONTROL,
                              [this](const QByteArray &data) {
        if (data.at(0) == WatchConnector::callHANGUP) {
            voice->hangupAll();
        }
        return true;
    });

    connect(voice, SIGNAL(activeVoiceCallChanged()), SLOT(onActiveVoiceCallChanged()));
    connect(voice, SIGNAL(error(const QString &)), SLOT(onVoiceError(const QString &)));

    connect(notifications, SIGNAL(error(const QString &)), SLOT(onNotifyError(const QString &)));
    connect(notifications, SIGNAL(emailNotify(const QString &,const QString &,const QString &)), SLOT(onEmailNotify(const QString &,const QString &,const QString &)));
    connect(notifications, SIGNAL(smsNotify(const QString &,const QString &)), SLOT(onSmsNotify(const QString &,const QString &)));
    connect(notifications, SIGNAL(twitterNotify(const QString &,const QString &)), SLOT(onTwitterNotify(const QString &,const QString &)));
    connect(notifications, SIGNAL(facebookNotify(const QString &,const QString &)), SLOT(onFacebookNotify(const QString &,const QString &)));

    connect(appmsg, &AppMsgManager::appStarted, this, &Manager::onAppOpened);
    connect(appmsg, &AppMsgManager::appStopped, this, &Manager::onAppClosed);

    connect(js, &JSKitManager::appNotification, this, &Manager::onAppNotification);

    QDBusConnection session = QDBusConnection::sessionBus();
    new WatchAdaptor(proxy);
    session.registerObject("/org/pebbled/Watch", proxy);
    session.registerService("org.pebbled");

    connect(dbus, &DBusConnector::pebbleChanged, proxy, &PebbledProxy::NameChanged);
    connect(dbus, &DBusConnector::pebbleChanged, proxy, &PebbledProxy::AddressChanged);
    connect(watch, &WatchConnector::connectedChanged, proxy, &PebbledProxy::ConnectedChanged);
    connect(bank, &BankManager::slotsChanged, proxy, &PebbledProxy::AppSlotsChanged);

    QString currentProfile = getCurrentProfile();
    defaultProfile = currentProfile.isEmpty() ? "ambience" : currentProfile;
    connect(watch, SIGNAL(connectedChanged()), SLOT(applyProfile()));

    // Music Control interface
    session.connect("", "/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties", "PropertiesChanged",
                this, SLOT(onMprisPropertiesChanged(QString,QMap<QString,QVariant>,QStringList)));

    connect(this, SIGNAL(mprisMetadataChanged(QVariantMap)), music, SLOT(onMprisMetadataChanged(QVariantMap)));

    // Set BT icon for notification
    notification.setImage("icon-system-bluetooth-device");

    if (btDevice.isValid()) {
        logger()->debug() << "BT local name:" << btDevice.name();
        connect(dbus, SIGNAL(pebbleChanged()), SLOT(onPebbleChanged()));
        dbus->findPebble();
    }
}

Manager::~Manager()
{
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

QString Manager::mpris() const
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

QString Manager::getCurrentProfile() const
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

void Manager::onAppNotification(const QUuid &uuid, const QString &title, const QString &body)
{
    Q_UNUSED(uuid);
    watch->sendSMSNotification(title, body);
}

void Manager::onAppOpened(const QUuid &uuid)
{
    currentAppUuid = uuid;
    emit proxy->AppUuidChanged();
    emit proxy->AppOpened(uuid.toString());
}

void Manager::onAppClosed(const QUuid &uuid)
{
    currentAppUuid = QUuid();
    emit proxy->AppClosed(uuid.toString());
    emit proxy->AppUuidChanged();
}

QStringList PebbledProxy::AppSlots() const
{
    const int num_slots = manager()->bank->numSlots();
    QStringList l;
    l.reserve(num_slots);

    for (int i = 0; i < num_slots; ++i) {
        if (manager()->bank->isUsed(i)) {
            QUuid uuid = manager()->bank->appAt(i);
            l.append(uuid.toString());
        } else {
            l.append(QString());
        }
    }

    Q_ASSERT(l.size() == num_slots);

    return l;
}

bool PebbledProxy::SendAppMessage(const QString &uuid, const QVariantMap &data)
{
    Q_ASSERT(calledFromDBus());
    const QDBusMessage msg = message();
    setDelayedReply(true);
    manager()->appmsg->send(uuid, data, [this, msg]() {
        QDBusMessage reply = msg.createReply(QVariant::fromValue(true));
        this->connection().send(reply);
    }, [this, msg]() {
        QDBusMessage reply = msg.createReply(QVariant::fromValue(false));
        this->connection().send(reply);
    });
    return false; // D-Bus clients should never see this reply.
}

QString PebbledProxy::StartAppConfiguration(const QString &uuid)
{
    Q_ASSERT(calledFromDBus());
    const QDBusMessage msg = message();

    if (manager()->currentAppUuid != uuid) {
        sendErrorReply(msg.interface() + ".Error.AppNotRunning",
                       "The requested app is not currently opened in the watch");
        return QString();
    }

    if (!manager()->js->isJSKitAppRunning()) {
        sendErrorReply(msg.interface() + ".Error.JSNotActive",
                       "The requested app is not a PebbleKit JS application");
        return QString();
    }

    // After calling showConfiguration() on the script,
    // it will (eventually!) return a URL to us via the appOpenUrl signal.

    // So we can't send the D-Bus reply right now.
    setDelayedReply(true);

    // Set up a signal handler to catch the appOpenUrl signal.
    QMetaObject::Connection c = connect(manager()->js, &JSKitManager::appOpenUrl,
                                        [this,msg,c](const QUrl &url) {
        // Workaround: due to a GCC bug we can't capture the uuid parameter, but we can extract
        // it again from the original message arguments.
        QString uuid = msg.arguments().at(0).toString();
        if (manager()->currentAppUuid != uuid) {
            // App was changed while we were waiting for the script..
            QDBusMessage reply = msg.createErrorReply(msg.interface() + ".Error.AppNotRunning",
                                                      "The requested app is not currently opened in the watch");
            connection().send(reply);
        } else {
            QDBusMessage reply = msg.createReply(QVariant::fromValue(url.toString()));
            connection().send(reply);
        }

        disconnect(c);
    });
    // TODO: JS script may fail, never call OpenURL, or something like that
    // In those cases we may leak the above connection
    // So we need to also set a timeout or similar.

    manager()->js->showConfiguration();

    // Note that the above signal handler _might_ have been already called by this point.

    return QString(); // This return value should never be used.
}

void PebbledProxy::SendAppConfigurationData(const QString &uuid, const QString &data)
{
    Q_ASSERT(calledFromDBus());
    const QDBusMessage msg = message();

    if (manager()->currentAppUuid != uuid) {
        sendErrorReply(msg.interface() + ".Error.AppNotRunning",
                       "The requested app is not currently opened in the watch");
        return;
    }

    if (!manager()->js->isJSKitAppRunning()) {
        sendErrorReply(msg.interface() + ".Error.JSNotActive",
                       "The requested app is not a PebbleKit JS application");
        return;
    }

    manager()->js->handleWebviewClosed(data);
}

void PebbledProxy::UnloadApp(int slot)
{
    Q_ASSERT(calledFromDBus());
    const QDBusMessage msg = message();

    if (!manager()->bank->unloadApp(slot)) {
        sendErrorReply(msg.interface() + ".Error.CannotUnload",
                       "Cannot unload application");
    }
}

void PebbledProxy::UploadApp(const QString &uuid, int slot)
{
    Q_ASSERT(calledFromDBus());
    const QDBusMessage msg = message();

    if (!manager()->bank->uploadApp(QUuid(uuid), slot)) {
        sendErrorReply(msg.interface() + ".Error.CannotUpload",
                       "Cannot upload application");
    }
}
