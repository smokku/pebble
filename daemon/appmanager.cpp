#include <QStandardPaths>
#include <QUuid>
#include "appmanager.h"

AppManager::AppManager(QObject *parent)
    : QObject(parent)
{
}

QString AppManager::getAppDir(const QUuid& uuid) const
{
    return QStandardPaths::locate(QStandardPaths::DataLocation,
                                  QString("apps/%1").arg(uuid.toString()),
                                  QStandardPaths::LocateDirectory);
}
