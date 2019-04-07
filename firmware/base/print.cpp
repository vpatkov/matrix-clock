#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include "print.hpp"

/* For Harvard architectures define "pm_read(p)" to read one byte from the
 * program memory pointer "p", and "pm_attr" to the attribute for data to be
 * stored in the program memory.
 */
#if defined(__AVR_ARCH__)       /* AVR */
#  include <avr/pgmspace.h>
#  define pm_read(p) pgm_read_byte(p)
#  define pm_attr PROGMEM
#else                           /* von Neumann */
#  define pm_read(p) (*(p))
#  define pm_attr
#endif

void Print::printf(const char *format, ...)
{
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
}

void Print::vprintf(const char *format, va_list args)
{
        struct Options {
                bool signed_: 1;
                bool force_sign: 1;
                bool space_for_plus: 1;
                bool negative: 1;
                bool left_justify: 1;
                bool uppercase: 1;
                bool length_long: 1;
                bool length_size_t: 1;
        };

        for (char c = pm_read(format++); c != '\0'; c = pm_read(format++))
        {
                if (c != '%') {
                        putc(c);
                        continue;
                }

                /*------------------------------*/
                /* Process "flags"              */
                /*------------------------------*/

                Options options = {};
                char pad = ' ';
                switch (c = pm_read(format++)) {
                case '+':
                        options.force_sign = 1;
                        c = pm_read(format++);
                        break;
                case '-':
                        options.left_justify = 1;
                        c = pm_read(format++);
                        break;
                case '0':
                        pad = '0';
                        c = pm_read(format++);
                        break;
                case ' ':
                        options.space_for_plus = 1;
                        c = pm_read(format++);
                        break;
                case '=':
                        if ((c = pm_read(format++)) == '\0')
                                break;
                        else if (c == '*')
                                pad = static_cast<char>(va_arg(args, int));
                        else
                                pad = c;
                        c = pm_read(format++);
                        break;
                }

                if (c == '\0')
                        break;

                /*------------------------------*/
                /* Process "minimal-width"      */
                /*------------------------------*/

                size_t min_width = 0;
                if (c == '*') {
                        min_width = va_arg(args, size_t);
                        c = pm_read(format++);
                } else {
                        for (min_width = 0; c >= '0' && c <= '9'; c = pm_read(format++))
                                min_width = min_width * 10 + (c - '0');
                }

                if (c == '\0')
                        break;

                /*------------------------------*/
                /* Process "maximal-width"      */
                /*------------------------------*/

                size_t max_width = SIZE_MAX;
                if (c == '.') {
                        if ((c = pm_read(format++)) == '\0')
                                break;
                        else if (c == '*') {
                                max_width = va_arg(args, size_t);
                                c = pm_read(format++);
                        } else {
                                for (max_width = 0; c >= '0' && c <= '9'; c = pm_read(format++))
                                        max_width = max_width * 10 + (c - '0');
                        }
                }

                if (c == '\0')
                        break;

                if (max_width < min_width)
                        continue;

                /*------------------------------*/
                /* Process "length"             */
                /*------------------------------*/

                if (c == 'l') {
                        options.length_long = 1;
                        c = pm_read(format++);
                } else if (c == 'z') {
                        options.length_size_t = 1;
                        c = pm_read(format++);
                }

                if (c == '\0')
                        break;

                /*------------------------------*/
                /* Process "specifier"          */
                /*------------------------------*/

                uint8_t base;
                switch (c) {
                case 's':
                case 'S': {
                        uint8_t i = 0;
                        const char *s = va_arg(args, const char *);
                        for (; ((c == 's') ? s[i] : pm_read(&s[i])) != '\0'; i++)
                                ;

                        while (!options.left_justify && i++ < min_width)
                                putc(pad);

                        uint8_t k = max_width - ((min_width > i) ? min_width - i : 0);
                        for (char d; (d = (c == 's') ? *s : pm_read(s)) != '\0' && k-- != 0; s++)
                                putc(d);

                        while (i++ < min_width)
                                putc(pad);

                        continue;
                }
                case 'c':
                        putc(static_cast<char>(va_arg(args, int)));
                        continue;
                case 'i':
                case 'd':
                        options.signed_ = 1;
                case 'u':
                        base = 10;
                        break;
                case 'o':
                        base = 8;
                        break;
                case 'b':
                        base = 2;
                        break;
                case 'X':
                        options.uppercase = 1;
                case 'x':
                        base = 16;
                        break;
                case '%':
                        putc('%');
                default:
                        continue;
                }

                /*------------------------------*/
                /* Print the integer            */
                /*------------------------------*/

                unsigned long val;
                if (options.length_long)
                        val = va_arg(args, unsigned long);
                else if (options.length_size_t)
                        val = va_arg(args, size_t);
                else
                        val = va_arg(args, int);

                if (options.signed_ && (val & 0x80000000)) {
                        val = -val;
                        options.negative = 1;
                }

                static const uint8_t digits[] pm_attr = "0123456789abcdef";
                char buf[16];
                uint8_t i = 0;
                do {
                        char d = pm_read(&digits[val % base]);
                        val /= base;
                        if (d >= 'a' && options.uppercase)
                                d += 0x20;
                        buf[i++] = d;
                } while (val > 0 && i < sizeof(buf)-2);  /* -2: sign and '\0' */

                if (options.negative)
                        buf[i++] = '-';
                else if (options.force_sign)
                        buf[i++] = '+';
                else if (options.space_for_plus)
                        buf[i++] = ' ';

                uint8_t j = i;
                uint8_t k = max_width - ((min_width > i) ? min_width - i : 0);

                while (!options.left_justify && j++ < min_width)
                        putc(pad);

                do {
                        putc(buf[--i]);
                } while (i != 0 && k-- != 0);

                while (j++ < min_width)
                        putc(pad);
        }
}
