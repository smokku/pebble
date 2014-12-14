#include "packer.h"
#include "watchconnector.h"

void Packer::writeBytes(int n, const QByteArray &b)
{
    if (b.size() > n) {
        _buf->append(b.constData(), n);
    } else {
        int diff = n - b.size();
        _buf->append(b);
        if (diff > 0) {
            _buf->append(QByteArray(diff, '\0'));
        }
    }
}

void Packer::writeCString(const QString &s)
{
    _buf->append(s.toUtf8());
    _buf->append('\0');
}

void Packer::writeUuid(const QUuid &uuid)
{
    writeBytes(16, uuid.toRfc4122());
}

void Packer::writeDict(const QMap<int, QVariant> &d)
{
    int size = d.size();
    if (size > 0xFF) {
        logger()->warn() << "Dictionary is too large to encode";
        writeLE<quint8>(0);
        return;
    }

    writeLE<quint8>(size);

    for (QMap<int, QVariant>::const_iterator it = d.constBegin(); it != d.constEnd(); ++it) {
        writeLE<quint32>(it.key());

        switch (int(it.value().type())) {
        case QMetaType::Char:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(char));
            writeLE<char>(it.value().value<char>());
            break;
        case QMetaType::Short:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(short));
            writeLE<short>(it.value().value<short>());
            break;
        case QMetaType::Int:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(int));
            writeLE<int>(it.value().value<int>());
            break;

        case QMetaType::UChar:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(char));
            writeLE<char>(it.value().value<char>());
            break;
        case QMetaType::UShort:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(short));
            writeLE<short>(it.value().value<short>());
            break;
        case QMetaType::UInt:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(int));
            writeLE<int>(it.value().value<int>());
            break;

        case QMetaType::Bool:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(char));
            writeLE<char>(it.value().value<char>());
            break;

        case QMetaType::Float: // Treat qreals as ints
        case QMetaType::Double:
            writeLE<quint8>(WatchConnector::typeINT);
            writeLE<quint16>(sizeof(int));
            writeLE<int>(it.value().value<int>());
            break;

        case QMetaType::QByteArray: {
            QByteArray ba = it.value().toByteArray();
            writeLE<quint8>(WatchConnector::typeBYTES);
            writeLE<quint16>(ba.size());
            _buf->append(ba);
            break;
        }

        case QMetaType::QVariantList: {
            // Generally a JS array, which we marshal as a byte array.
            QVariantList list = it.value().toList();
            QByteArray ba;
            ba.reserve(list.size());

            Q_FOREACH (const QVariant &v, list) {
                ba.append(v.toInt());
            }

            writeLE<quint8>(WatchConnector::typeBYTES);
            writeLE<quint16>(ba.size());
            _buf->append(ba);
            break;
        }

        default:
            logger()->warn() << "Unknown dict item type:" << it.value().typeName();
            /* Fallthrough */
        case QMetaType::QString:
        case QMetaType::QUrl:
        {
            QByteArray s = it.value().toString().toUtf8();
            if (s.isEmpty() || s[s.size() - 1] != '\0') {
                // Add null terminator if it doesn't have one
                s.append('\0');
            }
            writeLE<quint8>(WatchConnector::typeSTRING);
            writeLE<quint16>(s.size());
            _buf->append(s);
            break;
        }
        }
    }
}
