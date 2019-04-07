#include "bmp085.hpp"
#include "common.hpp"
#include "delay.hpp"

constexpr uint8_t i2c_addr = 0x77;
constexpr uint8_t oss = 3;

constexpr double up_conversion_time = 1.5 + (3<<oss);  /* ms */
constexpr double ut_conversion_time = 4.5;             /* ms */

bool Bmp085::read_eeprom(uint8_t addr, uint16_t &data)
{
        uint8_t in[2];

        if (i2c.write(i2c_addr, &addr, 1, 0) < 1 || i2c.read(i2c_addr, in, 2, 1) < 2)
                return false;

        data = concat16(in[0], in[1]);
        return true;
}

bool Bmp085::read_calibration()
{
        return  read_eeprom(0xaa, (uint16_t &)cal.ac1) &&
                read_eeprom(0xac, (uint16_t &)cal.ac2) &&
                read_eeprom(0xae, (uint16_t &)cal.ac3) &&
                read_eeprom(0xb0, (uint16_t &)cal.ac4) &&
                read_eeprom(0xb2, (uint16_t &)cal.ac5) &&
                read_eeprom(0xb4, (uint16_t &)cal.ac6) &&
                read_eeprom(0xb6, (uint16_t &)cal.b1)  &&
                read_eeprom(0xb8, (uint16_t &)cal.b2)  &&
                read_eeprom(0xba, (uint16_t &)cal.mb)  &&
                read_eeprom(0xbc, (uint16_t &)cal.mc)  &&
                read_eeprom(0xbe, (uint16_t &)cal.md);
}

bool Bmp085::read_uncompensated(bool p, int32_t &val)
{
        /* Start conversion */
        {
                const uint8_t out[] = {0xf4, p ? 0x34+(oss<<6) : 0x2e};
                if (i2c.write(i2c_addr, out, 2, 1) < 2)
                        return false;
        }

        /* Wait */
        if (p)
                delay_ms(up_conversion_time);
        else
                delay_ms(ut_conversion_time);

        /* Read the measured value */
        {
                const uint8_t out[] = {0xf6};
                uint8_t in[3];
                const uint8_t len = p ? 3 : 2;

                if (i2c.write(i2c_addr, out, 1, 0) < 1 || i2c.read(i2c_addr, in, len, 1) < len)
                        return false;

                val = p ? concat32(0, in[0], in[1], in[2]) >> (8-oss)
                        : concat16(in[0], in[1]);
        }

        return true;
}

bool Bmp085::read()
{
        return read_uncompensated(0, ut) && read_uncompensated(1, up);
}

uint32_t Bmp085::get_pressure()
{
        int32_t b6, x1, x2, x3, b3, b5, b7, p;
        uint32_t b4;

        x1 = ((ut - cal.ac6) * cal.ac5) >> 15;
        x2 = ((int32_t)cal.mc << 11)/(x1 + cal.md);
        b5 = x1 + x2;

        b6 = b5 - 4000;
        x1 = cal.b2 * (b6 * b6 >> 12) >> 11;
        x2 = cal.ac2 * b6 >> 11;
        x3 = x1 + x2;
        b3 = ((((int32_t)cal.ac1 * 4 + x3) << oss) + 2) / 4;

        x1 = cal.ac3 * b6 >> 13;
        x2 = cal.b1 * (b6 * b6 >> 12) >> 16;
        x3 = (x1 + x2 + 2) / 4;
        b4 = cal.ac4 * (uint32_t)(x3 + 32768) >> 15;

        b7 = (uint32_t)(up - b3) * (50000 >> oss);
        p = (b7 > 0) ? (b7 * 2 / b4) : (b7 / b4 * 2);

        x1 = (p >> 8) * (p >> 8);
        x1 = x1 * 3038 >> 16;
        x2 = -7357 * p >> 16;
        p += (x1 + x2 + 3791) >> 4;

        return p;
}

int16_t Bmp085::get_temperature()
{
        int32_t x1, x2, b5;

        x1 = ((ut - cal.ac6) * cal.ac5) >> 15;
        x2 = ((int32_t)cal.mc << 11)/(x1 + cal.md);
        b5 = x1 + x2;

        return static_cast<int16_t>((b5 + 8) >> 4);
}

bool Bmp085::init()
{
        return read_calibration();
}

Bmp085::Bmp085(I2c &bus): i2c(bus) {}
