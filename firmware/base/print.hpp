/*
 * Formatted output for embedded systems
 *
 * %[flags][minimal-width][.maximal-width][length]specifier
 *
 * Like standard printf <http://www.cplusplus.com/reference/cstdio/printf/>
 * except:
 *
 *   1. Return void, not int.
 *
 *   2. No floating point support.
 *
 *   3. Supported flags: - + 0 (space), and additional flags:
 *
 *         =<char>  Pad with <char> (except *).
 *         =*       Pad with the character given from the argument (int).
 *
 *   4. Sub-specifiers <minimal-width> and <maximal-width> are respectively
 *      the minimal and maximal number of characters to be printed. Both
 *      work with any specifier, support * (value from argument), and have
 *      size_t type.
 *
 *   5. Supported specifiers: d i u o x X c s %, and additional specifiers:
 *
 *         S        On Harvard architectures the argument is a pointer to a
 *                  string in the program memory. On von Neumann architectures
 *                  this is a synonim for 's' specifier.
 *         b        Unsigned binary integer.
 *
 *   6. Supported length: l z.
 *
 *   7. On Harvard architectures the format string must lie in the program
 *      memory.
 */

#ifndef PRINT_H_
#define PRINT_H_

#include <stdarg.h>

class Print {
public:
        void printf(const char *format, ...);
        void vprintf(const char *format, va_list args);
        virtual void putc(char c) = 0;
};

#endif
