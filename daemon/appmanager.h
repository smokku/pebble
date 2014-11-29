#ifndef APPMANAGER_H
#define APPMANAGER_H

#include <QObject>

class AppManager : public QObject
{
    Q_OBJECT

public:
    explicit AppManager(QObject *parent = 0);

    bool installPebbleApp(const QString &pbwFile);

    QList<QUuid> allInstalledApps() const;

    QString getAppDir(const QUuid &uuid) const;
};

#endif // APPMANAGER_H
