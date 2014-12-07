#ifndef STM32CRC_H
#define STM32CRC_H

#include <QByteArray>

class Stm32Crc
{
public:
    Stm32Crc();

    void reset();

    // Data size must be multiple of 4, data must be aligned to 4.

    void addData(const char *data, int length);
    void addData(const QByteArray &data);

    quint32 result() const;

private:
    quint32 crc;
};

#endif // STM32CRC_H
