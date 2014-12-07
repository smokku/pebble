#include "unpacker.h"
#include "packer.h"
#include "bankmanager.h"

BankManager::BankManager(WatchConnector *watch, AppManager *apps, QObject *parent) :
    QObject(parent), watch(watch), apps(apps)
{
    connect(watch, &WatchConnector::connectedChanged, this, &BankManager::handleWatchConnected);
}

int BankManager::numSlots() const
{
    return _slots.size();
}

bool BankManager::uploadApp(const QUuid &uuid, int slot)
{
    AppInfo info = apps->info(uuid);
    if (info.uuid() != uuid) {
        logger()->warn() << "uuid" << uuid << "is not installed";
        return false;
    }
    if (slot == -1) {
        slot = findUnusedSlot();
        if (slot == -1) {
            logger()->warn() << "no free slots!";
            return false;
        }
    }
    if (slot < 0 || slot > _slots.size()) {
        logger()->warn() << "invalid slot index";
        return false;
    }

    // TODO

    return false;
}

bool BankManager::unloadApp(int slot)
{
    if (slot < 0 || slot > _slots.size()) {
        logger()->warn() << "invalid slot index";
        return false;
    }
    if (!_slots[slot].used) {
        logger()->warn() << "slot is empty";
        return false;
    }

    logger()->debug() << "going to unload app" << _slots[slot].name << "in slot" << slot;

    int installId = _slots[slot].id;

    QByteArray msg;
    msg.reserve(2 * sizeof(quint32));
    Packer p(&msg);
    p.write<quint8>(WatchConnector::appmgrREMOVE_APP);
    p.write<quint32>(installId);
    p.write<quint32>(slot);

    watch->sendMessage(WatchConnector::watchAPP_MANAGER, msg,
                       [this](const QByteArray &data) {
        Unpacker u(data);
        if (u.read<quint8>() != WatchConnector::appmgrREMOVE_APP) {
            return false;
        }

        uint result = u.read<quint32>();
        switch (result) {
        case 1: /* Success */
            logger()->debug() << "sucessfully unloaded app";
            break;
        default:
            logger()->warn() << "could not unload app. result code:" << result;
            break;
        }

        QMetaObject::invokeMethod(this, "refresh", Qt::QueuedConnection);

        return true;
    });

    return true; // Operation in progress
}

void BankManager::refresh()
{
    logger()->debug() << "refreshing bank status";

    watch->sendMessage(WatchConnector::watchAPP_MANAGER,
                       QByteArray(1, WatchConnector::appmgrGET_APPBANK_STATUS),
                       [this](const QByteArray &data) {
        if (data.at(0) != WatchConnector::appmgrGET_APPBANK_STATUS) {
            return false;
        }

        if (data.size() < 9) {
            logger()->warn() << "invalid getAppbankStatus response";
            return true;
        }

        Unpacker u(data);

        u.skip(sizeof(quint8));

        unsigned int num_banks = u.read<quint32>();
        unsigned int apps_installed = u.read<quint32>();

        logger()->debug() << "Bank status:" << apps_installed << "/" << num_banks;

        _slots.resize(num_banks);
        for (unsigned int i = 0; i < num_banks; i++) {
            _slots[i].used = false;
            _slots[i].id = 0;
            _slots[i].name.clear();
            _slots[i].company.clear();
            _slots[i].flags = 0;
            _slots[i].version = 0;
            _slots[i].uuid = QUuid();
        }

        for (unsigned int i = 0; i < apps_installed; i++) {
           unsigned int id = u.read<quint32>();
           int index = u.read<quint32>();
           QString name = u.readFixedString(32);
           QString company = u.readFixedString(32);
           unsigned int flags = u.read<quint32>();
           unsigned short version = u.read<quint16>();

           if (index < 0 || index >= _slots.size()) {
               logger()->warn() << "Invalid slot index" << index;
               continue;
           }

           if (u.bad()) {
               logger()->warn() << "short read";
               return true;
           }

           _slots[index].used = true;
           _slots[index].id = id;
           _slots[index].name = name;
           _slots[index].company = company;
           _slots[index].flags = flags;
           _slots[index].version = version;

           AppInfo info = apps->info(name);
           QUuid uuid = info.uuid();
           _slots[index].uuid = uuid;

           logger()->debug() << index << id << name << company << flags << version << uuid;
        }

        emit this->slotsChanged();

        return true;
    });
}

int BankManager::findUnusedSlot() const
{
    for (int i = 0; i < _slots.size(); ++i) {
        if (!_slots[i].used) {
            return i;
        }
    }

    return -1;
}

void BankManager::handleWatchConnected()
{
    if (watch->isConnected()) {
        refresh();
    }
}

#if 0

void WatchConnector::getAppbankStatus(const std::function<void(const QString &s)>& callback)
{
    sendMessage(watchAPP_MANAGER, QByteArray(1, appmgrGET_APPBANK_STATUS),
                [this, callback](const QByteArray &data) {

    });
}

void WatchConnector::getAppbankUuids(const function<void(const QList<QUuid> &)>& callback)
{
    sendMessage(watchAPP_MANAGER, QByteArray(1, appmgrGET_APPBANK_UUIDS),
                [this, callback](const QByteArray &data) {
        if (data.at(0) != appmgrGET_APPBANK_UUIDS) {
            return false;
        }
        logger()->debug() << "getAppbankUuids response" << data.toHex();

        if (data.size() < 5) {
            logger()->warn() << "invalid getAppbankUuids response";
            return true;
        }

        Unpacker u(data);

        u.skip(sizeof(quint8));

        unsigned int apps_installed = u.read<quint32>();

        logger()->debug() << apps_installed;

        QList<QUuid> uuids;

        for (unsigned int i = 0; i < apps_installed; i++) {
           QUuid uuid = u.readUuid();

           logger()->debug() << uuid.toString();

           if (u.bad()) {
               logger()->warn() << "short read";
               return true;
           }

           uuids.push_back(uuid);
        }

        logger()->debug() << "finished";

        callback(uuids);

        return true;
    });
}
#endif
