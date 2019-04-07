/* 24x8 LED matrix display */

#ifndef MATRIX_HPP_
#define MATRIX_HPP_

#include <stdint.h>
#include "max7221.hpp"
#include "print.hpp"

class Matrix: public Print {
private:
        Max7221 &max_chain;             /* 3 daisy-chained MAX7221 */
        uint8_t buffer[24];             /* Display's buffer (byte per column) */
        uint8_t cur_x;                  /* Current x position for putc */
        void max_all(uint16_t data);    /* Write the same word to all MAX chips */
public:
        /* Special characters for putc */
        enum {
                special_up_arrow        = 1,
                special_down_arrow      = 2,
                special_min             = 3,
                special_max             = 4,
        };

        explicit Matrix(Max7221 &max_chain);
        void init();
        void set_brightness(uint8_t br);    /* 0..15 */
        void sync();

        /* 
         * The following methods works only in the buffer.
         * Call sync() to apply changes.
         */
        void clear();
        void draw_point(uint8_t x, uint8_t y, bool val = 1);
        void putc(char c) override;
};

#endif
