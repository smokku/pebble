#ifndef JSKITMANAGER_P_H
#define JSKITMANAGER_P_H

#include <QSettings>
#include "jskitmanager.h"

class JSKitPebble : public QObject
{
    Q_OBJECT

public:
    explicit JSKitPebble(JSKitManager *mgr);

    Q_INVOKABLE void addEventListener(const QString &event, QJSValue callback);
private:
    QHash<QString, QList<QJSValue>> _callbacks;
};

class JSKitLocalStorage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int length READ length NOTIFY lengthChanged)

public:
    explicit JSKitLocalStorage(const QUuid &uuid, JSKitManager *mgr);

    int length() const;

    Q_INVOKABLE QJSValue getItem(const QString &key) const;
    Q_INVOKABLE void setItem(const QString &key, const QString &value);
    Q_INVOKABLE void removeItem(const QString &key);

    Q_INVOKABLE void clear();

signals:
    void lengthChanged();

private:
    void checkLengthChanged();
    static QString getStorageFileFor(const QUuid &uuid);

private:
    QSettings *_storage;
    int _len;
};

#endif // JSKITMANAGER_P_H
