#include <QFile>
#include <QDir>
#include "unpacker.h"
#include "packer.h"
#include "bankmanager.h"

#if 0
// TODO -- This is how language files seems to be installed.
if (slot == -4) {
    logger()->debug() << "starting lang install";
    QFile *pbl = new QFile(QDir::home().absoluteFilePath("es.pbl"));
    if (!pbl->open(QIODevice::ReadOnly)) {
        logger()->warn() << "Failed to open pbl";
        return false;
    }

    upload->uploadFile("lang", pbl, [this]() {
        logger()->debug() << "success";
    }, [this](int code) {
        logger()->warn() << "Some error" << code;
    });

    return true;
}
#endif

BankManager::BankManager(WatchConnector *watch, UploadManager *upload, AppManager *apps, QObject *parent) :
    QObject(parent), watch(watch), upload(upload), apps(apps), _refresh(new QTimer(this))
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
    if (_slots[slot].used) {
        logger()->warn() << "slot in use";
        return false;
    }

    QDir appDir(info.path());

    logger()->debug() << "about to install app from" << appDir.absolutePath() << "into slot" << slot;

    QFile *binaryFile = new QFile(appDir.absoluteFilePath("pebble-app.bin"), this);
    if (!binaryFile->open(QIODevice::ReadOnly)) {
        logger()->warn() << "failed to open" << binaryFile->fileName() << ":" << binaryFile->errorString();
        delete binaryFile;
        return false;
    }

    logger()->debug() << "binary file size is" << binaryFile->size();

    QFile *resourceFile = 0;
    if (appDir.exists("app_resources.pbpack")) {
        resourceFile = new QFile(appDir.absoluteFilePath("app_resources.pbpack"), this);
        if (!resourceFile->open(QIODevice::ReadOnly)) {
            logger()->warn() << "failed to open" << resourceFile->fileName() << ":" << resourceFile->errorString();
            delete resourceFile;
            return false;
        }
    }

    // Mark the slot as used, but without any app, just in case.
    _slots[slot].used = true;
    _slots[slot].name.clear();
    _slots[slot].uuid = QUuid();

    upload->uploadAppBinary(slot, binaryFile,
    [this, binaryFile, resourceFile, slot]() {
        logger()->debug() << "app binary upload succesful";
        delete binaryFile;

        // Proceed to upload the resource file
        if (resourceFile) {
            upload->uploadAppResources(slot, resourceFile,
            [this, resourceFile, slot]() {
                logger()->debug() << "app resources upload succesful";
                delete resourceFile;

                // Upload succesful
                // Tell the watch to reload the slot
                refreshWatchApp(slot, [this]() {
                    logger()->debug() << "app refresh succesful";
                    _refresh->start();
                }, [this](int code) {
                    logger()->warn() << "app refresh failed" << code;
                    _refresh->start();
                });
            }, [this, resourceFile](int code) {
                logger()->warn() << "app resources upload failed" << code;
                delete resourceFile;

                _refresh->start();
            });

        } else {
            // No resource file
            // Tell the watch to reload the slot
            refreshWatchApp(slot, [this]() {
                logger()->debug() << "app refresh succesful";
                _refresh->start();
            }, [this](int code) {
                logger()->warn() << "app refresh failed" << code;
                _refresh->start();
            });
        }
    }, [this, binaryFile, resourceFile](int code) {
        logger()->warn() << "app binary upload failed" << code;
        delete binaryFile;
        delete resourceFile;

        _refresh->start();
    });

    return true;
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
            logger()->debug() << "sucessfully unloaded app";
            break;
        default:
            logger()->warn() << "could not unload app. result code:" << result;
            break;
        }

        _refresh->start();

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
