#include <avr/io.h>
#include <util/twi.h>
#include "i2c-hardware.hpp"
#include "common.hpp"
#include "shared.hpp"

static inline void twi_wait()
{
        while ((TWCR & 1<<TWINT) == 0)
                memory_barrier();
}

static uint8_t start()
{
        TWCR = 1<<TWINT | 1<<TWSTA | 1<<TWEN;
        twi_wait();
        return TW_STATUS;
}

static void stop()
{
        TWCR = 1<<TWINT | 1<<TWSTO | 1<<TWEN;
        while (TWCR & 1<<TWSTO)
                memory_barrier();
}

/* Single byte write */
static uint8_t write(uint8_t byte)
{
        TWDR = byte;
        TWCR = 1<<TWINT | 1<<TWEN;
        twi_wait();
        return TW_STATUS;
}

/* Single byte read */
static uint8_t read(bool ack)
{
        TWCR = ack ? (1<<TWINT | 1<<TWEA | 1<<TWEN) : (1<<TWINT | 1<<TWEN);
        twi_wait();
        return TWDR;
}

bool I2cHardware::init(uint32_t freq)
{
        if (freq < F_CPU/526 || freq > F_CPU/16)
                return false;
        TWCR = 0;
        TWSR = 0;
        TWBR = (F_CPU/freq-16)/2;
        return true;
}

void I2cHardware::deinit()
{
        TWCR = 0;
}

/* Bulk write */
static size_t write(const uint8_t *data, size_t len)
{
        size_t written;
        for (written = 0; written < len; written++)
                if (::write(*data++) != TW_MT_DATA_ACK)
                        break;
        return written;
}

/* Bulk read */
static size_t read(uint8_t *data, size_t len)
{
        size_t readen;
        for (readen = 0; readen < len; readen++) {
                bool last = (readen == len-1);
                *data++ = ::read(last ? false : true);
                if (TW_STATUS != (last ? TW_MR_DATA_NACK : TW_MR_DATA_ACK))
                        break;
        }
        return readen;
}

/* Write transaction: START SLA+W (DATA ack)... [STOP] */
size_t I2cHardware::write(uint8_t addr, const uint8_t *data, size_t len, bool make_stop)
{
        size_t written;

        uint8_t t = start();
        if ((t != TW_START && t != TW_REP_START) || ::write((addr << 1) | TW_WRITE) != TW_MT_SLA_ACK) {
                stop();
                return 0;
        }
        if ((written = ::write(data, len)) != len || make_stop)
                stop();

        return written;
}

/* Read transaction: START SLA+R [data ACK]... (data NACK) [STOP] */
size_t I2cHardware::read(uint8_t addr, uint8_t *data, size_t len, bool make_stop)
{
        size_t readen;

        uint8_t t = start();
        if ((t != TW_START && t != TW_REP_START) || ::write((addr << 1) | TW_READ) != TW_MR_SLA_ACK) {
                stop();
                return 0;
        }
        if ((readen = ::read(data, len)) != len || make_stop)
                stop();

        return readen;
}
