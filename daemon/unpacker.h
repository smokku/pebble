#ifndef UNPACKER_H
#define UNPACKER_H

#include <QtEndian>
#include <QByteArray>
#include <QString>
#include <QUuid>
#include <QVariantMap>
#include <Log4Qt/Logger>

class Unpacker
{
    LOG4QT_DECLARE_STATIC_LOGGER(logger, Unpacker)

public:
    Unpacker(const QByteArray &data);

    template <typename T>
    T read();

    template <typename T>
    T readLE();

    QByteArray readBytes(int n);

    QString readFixedString(int n);

    QUuid readUuid();

    QMap<int, QVariant> readDict();

    void skip(int n);

    bool bad() const;

private:
    const uchar * p();
    bool checkBad(int n = 0);

    const QByteArray &_buf;
    int _offset;
    bool _bad;
};

inline Unpacker::Unpacker(const QByteArray &data)
    : _buf(data), _offset(0), _bad(false)
{
}

template <typename T>
inline T Unpacker::read()
{
    if (checkBad(sizeof(T))) return 0;
    const uchar *u = p();
    _offset += sizeof(T);
    return qFromBigEndian<T>(u);
}

template <typename T>
inline T Unpacker::readLE()
{
    if (checkBad(sizeof(T))) return 0;
    const uchar *u = p();
    _offset += sizeof(T);
    return qFromLittleEndian<T>(u);
}

inline void Unpacker::skip(int n)
{
    _offset += n;
    checkBad();
}

inline bool Unpacker::bad() const
{
    return _bad;
}

inline const uchar * Unpacker::p()
{
    return reinterpret_cast<const uchar *>(&_buf.constData()[_offset]);
}

inline bool Unpacker::checkBad(int n)
{
    if (_offset + n > _buf.size()) {
        _bad = true;
    }
    return _bad;
}

#endif // UNPACKER_H
