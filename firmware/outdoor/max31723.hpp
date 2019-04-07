/* Maxim MAX31723 chip (SPI mode) */

#ifndef MAX31723_HPP_
#define MAX31723_HPP_

#include <stdint.h>
#include <stddef.h>
#include "spi.hpp"
#include "gpio.hpp"

class Max31723 {
private:
        Spi &spi;
        const Gpio::Pin ce;
public:
        /* Registers */
        enum {
                REG_CONF_STATUS         = 0,
                REG_TEMPERATURE_LSB     = 1,
                REG_TEMPERATURE_MSB     = 2,
                REG_THIGH_LSB           = 3,
                REG_THIGH_MSB           = 4,
                REG_TLOW_LSB            = 5,
                REG_TLOW_MSB            = 6,
        };

        /* REG_CONF_STATUS */
        enum {
                CONF_STATUS_SD          = 1 << 0,
                CONF_STATUS_R0          = 1 << 1,
                CONF_STATUS_R1          = 1 << 2,
                CONF_STATUS_TM          = 1 << 3,
                CONF_STATUS_1SHOT       = 1 << 4,
                CONF_STATUS_NVB         = 1 << 5,
                CONF_STATUS_MEMW        = 1 << 6,
        };

        Max31723(Spi &spi, Gpio::Pin ce);
        void init();

        void write(uint8_t reg, uint8_t data);
        uint8_t read(uint8_t reg);
};

#endif
