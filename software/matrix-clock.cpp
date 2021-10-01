#include <cstdio>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;

/* Configure serial port for 9600-8N1 */
static bool configure_port(int uart)
{
        termios ts;

        fcntl(uart, F_SETFL, 0);

        if (tcgetattr(uart, &ts) != 0)
                return false;

        cfsetispeed(&ts, B9600);
        cfsetospeed(&ts, B9600);

        ts.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
        ts.c_oflag &= ~OPOST;
        ts.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        ts.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
        ts.c_cflag |= CS8;
        ts.c_cc[VTIME] = 5;

        tcflush(uart, TCIOFLUSH);

        if (tcsetattr(uart, TCSANOW, &ts) != 0)
                return false;

        return true;
}

int main(int argc, char **argv)
{
        if (argc <= 1 || strcmp(argv[1], "--sync-time")) {
                fprintf(stderr, "Usage: matrix-clock [--sync-time [port]]\n");
                return 1;
        }

        /* Open serial port */
        const char *uart_name = (argc > 2) ? argv[2] : "/dev/ttyUSB0";
        int uart = open(uart_name, O_RDWR | O_NOCTTY | O_NDELAY);
        if (uart < 0) {
                fprintf(stderr, "Can't open %s: %s\n", uart_name, strerror(errno));
                return 2;
        }

        /* Configure the port */
        if (!configure_port(uart)) {
                fprintf(stderr, "Can't configure %s: %s\n", uart_name, strerror(errno));
                return 3;
        }

        /* Get current time */
        time_t now = time(0);
        tm *t = localtime(&now);

        /* Send the packet */
        uint8_t d[5] = {0xaa, t->tm_hour, t->tm_min, t->tm_sec,
                ~(t->tm_hour ^ t->tm_min ^ t->tm_sec)};
        if (write(uart, d, 5) < 5) {
                fprintf(stderr, "Can't write to %s: %s\n", uart_name, strerror(errno));
                return 4;
        }

        /* Check ack (checksum echo) */
        if (read(uart, d, 1) < 1) {
                fprintf(stderr, "No ack from pc-link: %s\n", strerror(errno));
                return 5;
        }
        if (d[0] != d[4]) {
                fprintf(stderr, "Bad connection: xmit checksum 0x%02x, recv checksum 0x%02x\n",
                        d[4], d[0]);
                return 6;
        }

        /* Flush and close the port */
        tcflush(uart, TCIOFLUSH);
        close(uart);

        return 0;
}
