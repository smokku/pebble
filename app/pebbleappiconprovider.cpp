#include <QDebug>
#include <QUrl>
#include "pebbleappiconprovider.h"

PebbleAppIconProvider::PebbleAppIconProvider(PebbledInterface *interface)
    : QQuickImageProvider(QQmlImageProviderBase::Image), pebbled(interface)
{
}

QImage PebbleAppIconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QUuid uuid(QUrl::fromPercentEncoding(id.toLatin1()));
    QImage img = pebbled->menuIconForApp(uuid);

    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        img = img.scaled(requestedSize, Qt::KeepAspectRatio);
    } else if (requestedSize.width() > 0) {
        img = img.scaledToWidth(requestedSize.width());
    } else if (requestedSize.height() > 0) {
        img = img.scaledToHeight(requestedSize.height());
    }

    if (size) {
        *size = img.size();
    }

    return img;
}
