#include <QFile>
#include <QDir>
#include "unpacker.h"
#include "packer.h"
#include "bankmanager.h"
#include "watchconnector.h"
#include "uploadmanager.h"
#include "appmanager.h"

#if 0
// TODO -- This is how language files seems to be installed.
if (slot == -4) {
    qCDebug(l) << "starting lang install";
    QFile *pbl = new QFile(QDir::home().absoluteFilePath("es.pbl"));
    if (!pbl->open(QIODevice::ReadOnly)) {
        qCWarning(l) << "Failed to open pbl";
        return false;
    }

    upload->uploadFile("lang", pbl, [this]() {
        qCDebug(l) << "success";
    }, [this](int code) {
        qCWarning(l) << "Some error" << code;
    });

    return true;
}
#endif

BankManager::BankManager(WatchConnector *watch, UploadManager *upload, AppManager *apps, QObject *parent) :
    QObject(parent), l(metaObject()->className()),
    watch(watch), upload(upload), apps(apps), _refresh(new QTimer(this))
{
    connect(watch, &WatchConnector::connectedChanged,
            this, &BankManager::handleWatchConnected);

    _refresh->setInterval(0);
    _refresh->setSingleShot(true);
    connect(_refresh, &QTimer::timeout,
            this, &BankManager::refresh);
}

int BankManager::numSlots() const
{
    return _slots.size();
}

bool BankManager::isUsed(int slot) const
{
    return _slots.at(slot).used;
}

QUuid BankManager::appAt(int slot) const
{
    return _slots.at(slot).uuid;
}

bool BankManager::uploadApp(const QUuid &uuid, int slot)
{
    AppInfo info = apps->info(uuid);
    if (info.uuid() != uuid) {
        qCWarning(l) << "uuid" << uuid << "is not installed";
        return false;
    }
    if (slot == -1) {
        slot = findUnusedSlot();
        if (slot == -1) {
            qCWarning(l) << "no free slots!";
            return false;
        }
    }
    if (slot < 0 || slot > _slots.size()) {
        qCWarning(l) << "invalid slot index";
        return false;
    }
    if (_slots[slot].used) {
        qCWarning(l) << "slot in use";
        return false;
    }

    qCDebug(l) << "about to install app" << info.shortName() << "into slot" << slot;

    QSharedPointer<QIODevice> binaryFile(info.openFile(AppInfo::BINARY));
    if (!binaryFile) {
        qCWarning(l) << "failed to open" << info.shortName() << "AppInfo::BINARY";
        return false;
    }

    qCDebug(l) << "binary file size is" << binaryFile->size();

    // Mark the slot as used, but without any app, just in case.
    _slots[slot].used = true;
    _slots[slot].name.clear();
    _slots[slot].uuid = QUuid();

    upload->uploadAppBinary(slot, binaryFile.data(), info.crcFile(AppInfo::BINARY),
    [this, info, binaryFile, slot]() {
        qCDebug(l) << "app binary upload succesful";
        binaryFile->close();

        // Proceed to upload the resource file
        QSharedPointer<QIODevice> resourceFile(info.openFile(AppInfo::RESOURCES));
        if (resourceFile) {
            upload->uploadAppResources(slot, resourceFile.data(), info.crcFile(AppInfo::RESOURCES),
            [this, resourceFile, slot]() {
                qCDebug(l) << "app resources upload succesful";
                resourceFile->close();
                // Upload succesful
                // Tell the watch to reload the slot
                refreshWatchApp(slot, [this]() {
                    qCDebug(l) << "app refresh succesful";
                    _refresh->start();
                }, [this](int code) {
                    qCWarning(l) << "app refresh failed" << code;
                    _refresh->start();
                });
            }, [this, resourceFile](int code) {
                resourceFile->close();
                qCWarning(l) << "app resources upload failed" << code;
                _refresh->start();
            });
        } else {
            // No resource file
            // Tell the watch to reload the slot
            refreshWatchApp(slot, [this]() {
                qCDebug(l) << "app refresh succesful";
                _refresh->start();
            }, [this](int code) {
                qCWarning(l) << "app refresh failed" << code;
                _refresh->start();
            });
        }
    }, [this, binaryFile](int code) {
        binaryFile->close();
        qCWarning(l) << "app binary upload failed" << code;
        _refresh->start();
    });

    return true;
}

bool BankManager::unloadApp(int slot)
{
    if (slot < 0 || slot > _slots.size()) {
        qCWarning(l) << "invalid slot index";
        return false;
    }
    if (!_slots[slot].used) {
        qCWarning(l) << "slot is empty";
        return false;
    }

    qCDebug(l) << "going to unload app" << _slots[slot].name << "in slot" << slot;

    int installId = _slots[slot].id;

    QByteArray msg;
    msg.reserve(1 + 2 * sizeof(quint32));
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
        case Success: /* Success */
            qCDebug(l) << "sucessfully unloaded app";
            break;
        default:
            qCWarning(l) << "could not unload app. result code:" << result;
            break;
        }

        _refresh->start();

        return true;
    });

    return true; // Operation in progress
}

void BankManager::refresh()
{
    qCDebug(l) << "refreshing bank status";

    watch->sendMessage(WatchConnector::watchAPP_MANAGER,
                       QByteArray(1, WatchConnector::appmgrGET_APPBANK_STATUS),
                       [this](const QByteArray &data) {
        if (data.at(0) != WatchConnector::appmgrGET_APPBANK_STATUS) {
            return false;
        }

        if (data.size() < 9) {
            qCWarning(l) << "invalid getAppbankStatus response";
            return true;
        }

        Unpacker u(data);

        u.skip(sizeof(quint8));

        unsigned int num_banks = u.read<quint32>();
        unsigned int apps_installed = u.read<quint32>();

        qCDebug(l) << "Bank status:" << apps_installed << "/" << num_banks;

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
               qCWarning(l) << "Invalid slot index" << index;
               continue;
           }

           if (u.bad()) {
               qCWarning(l) << "short read";
               return true;
           }

           _slots[index].used = true;
           _slots[index].id = id;
           _slots[index].name = name;
           _slots[index].company = company;
           _slots[index].flags = flags;
           _slots[index].version = version;

           AppInfo info = apps->info(name);
           if (info.shortName() != name) {
               info = AppInfo::fromSlot(_slots[index]);
               apps->insertAppInfo(info);
           }
           QUuid uuid = info.uuid();
           _slots[index].uuid = uuid;

           qCDebug(l) << index << id << name << company << flags << version << uuid;
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

void BankManager::refreshWatchApp(int slot, std::function<void ()> successCallback, std::function<void (int)> errorCallback)
{
    QByteArray msg;
    Packer p(&msg);
    p.write<quint8>(WatchConnector::appmgrREFRESH_APP);
    p.write<quint32>(slot);

    watch->sendMessage(WatchConnector::watchAPP_MANAGER, msg,
                       [this, successCallback, errorCallback](const QByteArray &data) {
        Unpacker u(data);
        int type = u.read<quint8>();
        // For some reason, the watch might sometimes reply an "app installed" message
        // with a "app removed" confirmation message
        // Every other implementation seems to ignore this fact, so I guess it's not important.
        if (type != WatchConnector::appmgrREFRESH_APP && type != WatchConnector::appmgrREMOVE_APP) {
            return false;
        }
        int code = u.read<quint32>();
        if (code == Success) {
            if (successCallback) {
                successCallback();
            }
        } else {
            if (errorCallback) {
                errorCallback(code);
            }
        }

        return true;
    });
}

void BankManager::handleWatchConnected()
{
    if (watch->isConnected()) {
        _refresh->start();
    }
}

#if 0
void BankManager::getAppbankUuids(const function<void(const QList<QUuid> &)>& callback)
{
    watch->sendMessage(WatchConnector::watchAPP_MANAGER,
                       QByteArray(1, WatchConnector::appmgrGET_APPBANK_UUIDS),
                [this, callback](const QByteArray &data) {
        if (data.at(0) != WatchConnector::appmgrGET_APPBANK_UUIDS) {
            return false;
        }
        qCDebug(l) << "getAppbankUuids response" << data.toHex();

        if (data.size() < 5) {
            qCWarning(l) << "invalid getAppbankUuids response";
            return true;
        }

        Unpacker u(data);

        u.skip(sizeof(quint8));

        unsigned int apps_installed = u.read<quint32>();

        qCDebug(l) << apps_installed;

        QList<QUuid> uuids;

        for (unsigned int i = 0; i < apps_installed; i++) {
           QUuid uuid = u.readUuid();

           qCDebug(l) << uuid.toString();

           if (u.bad()) {
               qCWarning(l) << "short read";
               return true;
           }

           uuids.push_back(uuid);
        }

        qCDebug(l) << "finished";

        callback(uuids);

        return true;
    });
}
#endif
