#ifndef PEBBLEAPPICONPROVIDER_H
#define PEBBLEAPPICONPROVIDER_H

#include <QQuickImageProvider>
#include "pebbledinterface.h"

class PebbleAppIconProvider : public QQuickImageProvider
{
public:
    explicit PebbleAppIconProvider(PebbledInterface *interface);

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);

private:
    PebbledInterface *pebbled;
};

#endif // PEBBLEAPPICONPROVIDER_H
