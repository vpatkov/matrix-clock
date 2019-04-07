#include <cstdio>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;

const unsigned int sync_period = 60;    /* Seconds */

static bool stop = false;

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

/* SIGTERM handler: stop the daemon */
static void stop_daemon(int unused)
{
        stop = true;
}

/* SIGUSR1 handler: wake up from sleep, that cause forced sync */
static void nothing(int unused)
{
        /* nothing */
}

int main(int argc, char **argv)
{
        /* Set signal handlers */
        for (int s = 1; s <= _NSIG; s++) {
                if (s == SIGTERM || s == SIGINT)
                        signal(s, stop_daemon);
                else if (s == SIGUSR1)
                        signal(s, nothing);
                else
                        signal(s, SIG_IGN);
        }

        /* Open serial port */
        const char *uart_name = (argc > 1) ? argv[1] : "/dev/ttyUSB0";
        int uart = open(uart_name, O_RDWR | O_NOCTTY | O_NDELAY);
        if (uart < 0) {
                fprintf(stderr, "Can't open %s: %s\n", uart_name, strerror(errno));
                return 1;
        }

        /* Configure for 9600-8N1 */
        if (!configure_port(uart)) {
                fprintf(stderr, "Can't configure port %s: %s\n", uart_name, strerror(errno));
                return 1;
        }

        for (; !stop; sleep(sync_period))
        {
                /* Get current time */
                time_t now = time(0);
                tm *t = localtime(&now);

                /* Send packet */
                uint8_t d[5] = {0xaa, t->tm_hour, t->tm_min, t->tm_sec,
                        ~(t->tm_hour ^ t->tm_min ^ t->tm_sec)};
                if (write(uart, d, 5) < 5) {
                        fprintf(stderr, "Can't write to %s: %s\n", uart_name, strerror(errno));
                        continue;
                }

                /* Check ack (checksum echo) */
                if (read(uart, d, 1) < 1) {
                        fprintf(stderr, "No ack from pc-link: %s\n", strerror(errno));
                        continue;
                }
                if (d[0] != d[4]) {
                        fprintf(stderr, "Bad connection: xmit checksum 0x%02x, recv checksum 0x%02x\n",
                                d[4], d[0]);
                        continue;
                }
        }

        tcflush(uart, TCIOFLUSH);
        close(uart);

        return 0;
}
