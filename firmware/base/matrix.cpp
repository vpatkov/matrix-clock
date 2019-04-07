#include <avr/pgmspace.h>
#include "common.hpp"
#include "matrix.hpp"
#include "font5x8.hpp"  /* Auto-generated */

Matrix::Matrix(Max7221 &m): max_chain(m), cur_x(0)
{
        clear();
}

void Matrix::clear()
{
        for (uint8_t i = 0; i < 24; i++)
                buffer[i] = 0;
}

void Matrix::init()
{
        max_all(Max7221::REG_DISPLAY_TEST | 0);
        max_all(Max7221::REG_SCAN_LIMIT | 7);
        max_all(Max7221::REG_DECODE_MODE | 0);
        max_all(Max7221::REG_SHUTDOWN | 1);
}

void Matrix::max_all(uint16_t data)
{
        uint16_t d[] = {data, data, data};
        max_chain.write(d, 3);
}

void Matrix::set_brightness(uint8_t br)
{
        max_all(Max7221::REG_INTENSITY | (br & 15));
}

void Matrix::sync()
{
        for (uint8_t col = 1; col <= 8; col++) {
                uint16_t d[] = {
                        concat16(col, buffer[col+15]),
                        concat16(col, buffer[col+7]),
                        concat16(col, buffer[col-1])
                };
                max_chain.write(d, 3);
        }
}

void Matrix::draw_point(uint8_t x, uint8_t y, bool val)
{
        if (x < 24 && y < 8)
                set_bits(buffer[x], 1<<y, val);
}

void Matrix::putc(char c)
{
        uint8_t d = c;

        if (d == '\r' || d == '\n') {
                if (d == '\n')
                        clear();
                cur_x = 0;
                return;
        }

        if (cur_x > 18)
                return;

        uint8_t *p = &buffer[cur_x];
        const uint8_t *q = &font5x8[(d < 128) ? pgm_read_byte(&ascii[d]) : 0][0];
        for (uint8_t i = 0; i < 5; i++)
                *p++ = pgm_read_byte(q++);

        *p = 0;
        cur_x += 6;
}
