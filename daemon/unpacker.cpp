#include "unpacker.h"
#include "watchconnector.h"

QByteArray Unpacker::readBytes(int n)
{
    if (checkBad(n)) return QByteArray();
    const char *u = &_buf.constData()[_offset];
    _offset += n;
    return QByteArray(u, n);
}

QString Unpacker::readFixedString(int n)
{
    if (checkBad(n)) return QString();
    const char *u = &_buf.constData()[_offset];
    _offset += n;
    return QString::fromUtf8(u, strnlen(u, n));
}

QUuid Unpacker::readUuid()
{
    if (checkBad(16)) return QString();
    _offset += 16;
    return QUuid::fromRfc4122(_buf.mid(_offset - 16, 16));
}

QMap<int, QVariant> Unpacker::readDict()
{
    QMap<int, QVariant> d;
    if (checkBad(1)) return d;

    const int n = readLE<quint8>();

    for (int i = 0; i < n; i++) {
        if (checkBad(4 + 1 + 2)) return d;
        const int key = readLE<qint32>(); // For some reason, this is little endian.
        const int type = readLE<quint8>();
        const int width = readLE<quint16>();

        switch (type) {
        case WatchConnector::typeBYTES:
            d.insert(key, QVariant::fromValue(readBytes(width)));
            break;
        case WatchConnector::typeSTRING:
            d.insert(key, QVariant::fromValue(readFixedString(width)));
            break;
        case WatchConnector::typeUINT:
            switch (width) {
            case sizeof(quint8):
                d.insert(key, QVariant::fromValue(readLE<quint8>()));
                break;
            case sizeof(quint16):
                d.insert(key, QVariant::fromValue(readLE<quint16>()));
                break;
            case sizeof(quint32):
                d.insert(key, QVariant::fromValue(readLE<quint32>()));
                break;
            default:
                _bad = true;
                return d;
            }

            break;
        case WatchConnector::typeINT:
            switch (width) {
            case sizeof(qint8):
                d.insert(key, QVariant::fromValue(readLE<qint8>()));
                break;
            case sizeof(qint16):
                d.insert(key, QVariant::fromValue(readLE<qint16>()));
                break;
            case sizeof(qint32):
                d.insert(key, QVariant::fromValue(readLE<qint32>()));
                break;
            default:
                _bad = true;
                return d;
            }

            break;
        default:
            _bad = true;
            return d;
        }
    }

    return d;
}
