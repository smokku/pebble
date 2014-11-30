#ifndef UNPACKER_H
#define UNPACKER_H

#include <QtEndian>
#include <QByteArray>
#include <QString>
#include <QUuid>

class Unpacker
{
public:
    Unpacker(const QByteArray &data);

    template <typename T>
    T read();

    QString readFixedString(int n);

    QUuid readUuid();

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

inline QString Unpacker::readFixedString(int n)
{
    if (checkBad(n)) return QString();
    const char *u = &_buf.constData()[_offset];
    _offset += n;
    return QString::fromUtf8(u, strnlen(u, n));
}

inline QUuid Unpacker::readUuid()
{
    if (checkBad(16)) return QString();
    _offset += 16;
    return QUuid::fromRfc4122(_buf.mid(_offset - 16, 16));
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
