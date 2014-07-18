#ifndef SETTINGS_H
#define SETTINGS_H

#include <MDConfGroup>

class Settings : public MDConfGroup
{
    Q_OBJECT

    Q_PROPERTY(bool silentWhenConnected MEMBER silentWhenConnected NOTIFY silentWhenConnectedChanged)
    bool silentWhenConnected;

public:
    explicit Settings(QObject *parent = 0) :
        MDConfGroup("/org/pebbled/settings", parent, BindProperties)
    { resolveMetaObject(); }

signals:
    void silentWhenConnectedChanged(bool);

public slots:

};

#endif // SETTINGS_H
