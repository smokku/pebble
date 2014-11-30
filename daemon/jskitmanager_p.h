#ifndef JSKITMANAGER_P_H
#define JSKITMANAGER_P_H

#include "jskitmanager.h"

class JSKitPebble : public QObject
{
    Q_OBJECT

public:
    explicit JSKitPebble(JSKitManager *mgr);
    ~JSKitPebble();
};

#endif // JSKITMANAGER_P_H
