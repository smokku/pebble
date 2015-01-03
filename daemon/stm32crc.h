#ifndef STM32CRC_H
#define STM32CRC_H

#include <QByteArray>

class Stm32Crc
{
public:
    Stm32Crc();

    void reset();

    void addData(const char *data, int length);
    void addData(const QByteArray &data);

    quint32 result() const;

private:
    quint32 crc;
    quint8 buffer[4];
    quint8 rem;
};

#endif // STM32CRC_H
