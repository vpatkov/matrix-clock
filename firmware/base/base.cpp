/* Main program */

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "common.hpp"
#include "delay.hpp"
#include "shared.hpp"
#include "gpio.hpp"
#include "spi-hardware.hpp"
#include "matrix.hpp"
#include "max7221.hpp"
#include "dht22.hpp"
#include "i2c-hardware.hpp"
#include "bmp085.hpp"
#include "nrf24.hpp"

/* Peripherals are configured for 8 MHz system clock */
static_assert(F_CPU == 8e6, "");

constexpr Gpio::Pin max_cs = Gpio::B1;
constexpr Gpio::Pin nrf_csn = Gpio::D7, nrf_ce = Gpio::B2, nrf_irq = Gpio::D6;
constexpr Gpio::Pin dht_data = Gpio::D5;
constexpr Gpio::Pin enc_a = Gpio::D2, enc_b = Gpio::D3, enc_button = Gpio::D4;
constexpr Gpio::Pin mosi = Gpio::B3, miso = Gpio::B4, sck = Gpio::B5;

constexpr uint8_t light_adc_channel = 0;

constexpr uint32_t i2c_freq = 200e3;        /* I2C frequency (Hz) */

constexpr uint8_t full_brightness = 160;    /* Ambient light level for full matrix brightness (0..255) */
constexpr uint8_t hysteresis = 16;          /* Hysteresis of the automatic brightness control (0..255) */

constexpr int32_t outdoor_reliable_time = 10*60;        /* How long the outdoor temperature is reliable (s) */
constexpr int32_t clock_reliable_time = 7*24*3600ul;    /* How long the clock is reliable (s) */
constexpr uint8_t measure_indoor_interval = 10;         /* Interval for indoor weather measurement (s) */
constexpr uint8_t reset_screen_timeout = 10;            /* Timeout to reset screen (s) */

constexpr uint8_t history_size = 128;   /* How many weather records per day */

constexpr uint8_t low_battery_level = 3.6/4.2*255;  /* Threshold for low battery warning (0..255, 255 is 4.2 V) */

/* NRF24 network adresses and payload lengths */
constexpr uint8_t pc_link_addr[] = {0xe7, 0x4f, 0xec, 0xe8, 0x37};  /* Pipe 0 */
constexpr uint8_t pc_link_payload_length = 3;
constexpr uint8_t outdoor_addr[] = {0xc8, 0xb4, 0xe1, 0x65, 0x3b};  /* Pipe 1 */
constexpr uint8_t outdoor_payload_length = 2;

struct Weather {
        int8_t temperature_outdoor;     /* Outdoor temperature (°C) */
        int8_t temperature_indoor;      /* Indoor temperature (°C) */
        uint8_t humidity;               /* Relative humidity (%) */
        uint16_t pressure;              /* Air pressure (mmHg) */
};

struct Time {
        int8_t h, m, s;
};

enum class ScreenX: uint8_t {
        clock,
        temperature_outdoor,
        temperature_indoor,
        humidity,
        pressure,
        nr_screens
};

enum class ScreenY: uint8_t {
        current,
        change,
        minimal,
        maximal,
        nr_screens
};

struct Screen {
        ScreenX x;
        ScreenY y;
};

/* Faulty values */
constexpr int8_t bad_temperature = INT8_MAX;
constexpr uint16_t bad_pressure = UINT16_MAX;
constexpr uint8_t bad_humidity = UINT8_MAX;
constexpr Weather bad_weather = {
        .temperature_outdoor = bad_temperature,
        .temperature_indoor = bad_temperature,
        .humidity = bad_humidity,
        .pressure = bad_pressure,
};

/* Flags to control the main loop */
union Flags {
        uint16_t all;
        struct {
                bool enc_rotated_right: 1;      /* Rotary encoder was rotated right */
                bool enc_rotated_left: 1;       /* Rotary encoder was rotated left */
                bool nrf24_irq: 1;              /* NRF24 interrupt */
                bool measure_indoor: 1;         /* Measure indoor weather */
                bool refresh_screen: 1;         /* Refresh the screen */
                bool light_changed: 1;          /* Ambient light level was changed */
                bool reset_screen: 1;           /* Set screen to default (clock, current weather) */
                bool update_history: 1;         /* Save current weather to the history */
        };
};

static SpiHardware spi {mosi, miso, sck};
static Max7221 max_chain {spi, max_cs};
static Matrix matrix {max_chain};
static Nrf24 nrf24 {spi, nrf_csn, nrf_ce};
static Dht22 dht22 {dht_data};
static I2cHardware i2c;
static Bmp085 bmp085 {i2c};

static Weather weather = bad_weather;           /* Current weather */
static uint8_t battery_level;                   /* Outdoor battery level (0..255, 255 is 4.2 V) */

/* Recent update time (s_uptime) */
static int32_t outdoor_recent = -outdoor_reliable_time - 1;
static int32_t clock_recent = -clock_reliable_time - 1;

/* Weather history for the last day */
static struct {
        Weather weather[history_size];
        uint8_t current;
} history;

/* Shared variables (changed in interrupts) */
static Flags s_flags;                           /* Flags to control the main loop */
static Time s_time;                             /* Current time */
static int32_t s_uptime;                        /* Uptime (s) */
static uint8_t s_inactivity_timer;              /* User inactivity timer (s) */
static uint8_t s_light;                         /* Ambient light level (0..255) */

/* 1 Hz interrupt (RTC, timers) */
ISR(TIMER2_OVF_vect)
{
        /* RTC */
        if (++s_time.s >= 60) {
                s_time.s -= 60;
                if (++s_time.m >= 60) {
                        s_time.m -= 60;
                        if (++s_time.h >= 24)
                                s_time.h -= 24;
                }
        }

        /* Uptime */
        s_uptime++;

        /* Refresh the screen every second */
        s_flags.refresh_screen = 1;     

        /* Measure indoor weather */
        {
                static uint8_t cnt = 0;
                if (++cnt == measure_indoor_interval) {
                        cnt = 0;
                        s_flags.measure_indoor = 1;
                }
        }

        /* Update history */
        static_assert(86400 % history_size == 0, "");
        if (s_uptime % (86400/history_size) == 0)
                s_flags.update_history = 1;

        /* Reset screen by inactivity */
        if (++s_inactivity_timer == reset_screen_timeout)
                s_flags.reset_screen = 1;
}

/* ~500 Hz general-purpose interrupt */
ISR(TIMER0_OVF_vect)
{
        /* Rotary encoder */
        {
                static uint8_t cnt = 0;
                static bool prev_state = 1;
                if (Gpio::read(enc_a) != prev_state) {
                        if (++cnt > 1) {
                                if (prev_state != 0) {  /* Negative edge */
                                        if (Gpio::read(enc_b))
                                                s_flags.enc_rotated_left = 1;
                                        else
                                                s_flags.enc_rotated_right = 1;
                                }
                                prev_state = !prev_state;
                        }
                } else
                        cnt = 0;
        }

        /* NRF24 interrupt */
        if (Gpio::read(nrf_irq) == 0)
                s_flags.nrf24_irq = 1;
        
        /* Ambient light level */
        {
                static uint8_t cnt = 0;
                static uint16_t acc = 0;
                static uint8_t prev_value = 0;

                acc += ADCH;
                if (++cnt == 0) {  /* Accumulate ~0.5 seconds */
                        s_light = acc/256;
                        if (s_light != prev_value) {
                                s_flags.light_changed = 1;
                                prev_value = s_light;
                        }
                        acc = 0;
                }

                /* Start the next conversion (1.1 V reference, 62.5 kHz clock) */
                static_assert(light_adc_channel <= 7, "");
                ADMUX = 1<<REFS1 | 1<<REFS0 | 1<<ADLAR | light_adc_channel;
                ADCSRA = 1<<ADEN | 1<<ADSC | 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0;
        }
}

void nrf24_setup()
{
        /* 3-byte address, 2402 MHz, 1 Mbit/s */
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_CONFIG,
                Nrf24::MASK_TX_DS | Nrf24::MASK_MAX_RT | Nrf24::EN_CRC |
                Nrf24::PWR_UP | Nrf24::CRC0 | Nrf24::PRIM_RX);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RF_SETUP,
                Nrf24::RF_DR_1Mbps | Nrf24::RF_PWR_0dBm);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_FEATURE, Nrf24::EN_DYN_ACK);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_SETUP_AW, Nrf24::AW_5_BYTES);

        /* Enable data pipes 0 and 1 */
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_EN_RXADDR, Nrf24::ERX_P0 | Nrf24::ERX_P1);

        /* Set the payload lengths */
        static_assert(pc_link_payload_length < 32, "");
        static_assert(outdoor_payload_length < 32, "");
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RX_PW_P0, pc_link_payload_length);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RX_PW_P1, outdoor_payload_length);

        /* Set the addresses */
        static_assert(size(pc_link_addr) == 5, "");
        static_assert(size(outdoor_addr) == 5, "");
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RX_ADDR_P0, pc_link_addr, size(pc_link_addr));
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_RX_ADDR_P1, outdoor_addr, size(outdoor_addr));

        /* Start the receiver */
        delay_ms(Nrf24::tpd2stby);
        nrf24.set_ce(1);
}

void nrf24_receive()
{
        uint8_t d[3];

        /* Stop the receiver */
        nrf24.set_ce(0);

        /* Get the received data */
        switch (nrf24.status() & Nrf24::RX_P_NO) {
        case Nrf24::RX_P_NO_0:  /* pc-link */
                nrf24.read(Nrf24::CMD_R_RX_PAYLOAD, d, 3);      /* Hours, minutes, seconds */
                atomic_block {
                        s_time.h = d[0];
                        s_time.m = d[1];
                        s_time.s = d[2];
                        clock_recent = s_uptime;
                }
                break;
        case Nrf24::RX_P_NO_1:  /* outdoor */
                nrf24.read(Nrf24::CMD_R_RX_PAYLOAD, d, 2);      /* Temperature, battery level */
                weather.temperature_outdoor = d[0];
                battery_level = d[1];
                outdoor_recent = atomic_read(s_uptime);
                break;
        }

        /* Flush the receiver and clear the interrupt flags */
        nrf24.write(Nrf24::CMD_FLUSH_RX);
        nrf24.write(Nrf24::CMD_W_REGISTER | Nrf24::REG_STATUS,
                Nrf24::RX_DR | Nrf24::TX_DS | Nrf24::MAX_RT);

        /* Start the receiver */
        nrf24.set_ce(1);
}

int main()
{
        Flags flags = {};
        Screen screen = {ScreenX::clock, ScreenY::current};
        bool temperature_indoor_reliable = false;
        bool humidity_reliable = false;
        bool pressure_reliable = false;

        /* GPIO init */
        PORTB = 0b00000011;
        DDRB  = 0b00101110;
        PORTC = 0b11001110;
        DDRC  = 0b00000000;
        PORTD = 0b10000011;
        DDRD  = 0b00000000;

        /* Disable unused peripherals */
        ACSR = 1<<ACD;
        PRR = 1<<PRTIM1 | 1<<PRUSART0;

        /* 8-bit T/C0 for the ~500 Hz interrupt */
        TCCR0B = 1<<CS01 | 1<<CS00;
        TIMSK0 |= 1<<TOIE0;

        /* 8-bit T/C2 for the 1 Hz interrupt */
        ASSR = 1<<AS2;
        TCCR2B = 1<<CS22 | 1<<CS20;
        while (ASSR & (1<<TCR2BUB))
                memory_barrier();
        TIFR2 = 1<<TOV2;
        TIMSK2 |= 1<<TOIE2;

        /* Watchdog Timer (4 s) */
        WDTCSR = 1<<WDCE | 1<<WDE;
        WDTCSR = 1<<WDE | 1<<WDP3;

        /* Other setups */
        spi.init(SpiHardware::div_16);
        max_chain.init();
        matrix.init();
        nrf24.init();
        nrf24_setup();
        dht22.init();
        bool bmp085_ok = i2c.init(i2c_freq) && bmp085.init();

        /* Clear the weather history */
        for (auto &w : history.weather)
                w = bad_weather;

        /* Initial flags */
        flags.measure_indoor = 1;
        flags.light_changed = 1;

        sei();

        while (true)
        {
                atomic_block {
                        flags.all |= s_flags.all;
                        s_flags.all = 0;
                }

                /* Indoor weather measurements */
                if (flags.measure_indoor) {
                        temperature_indoor_reliable = humidity_reliable = dht22.read();
                        if (temperature_indoor_reliable) {
                                weather.temperature_indoor = dht22.get_temperature()/10;
                                weather.humidity = dht22.get_humidity()/10;
                        }

                        pressure_reliable = bmp085_ok && bmp085.read();
                        if (pressure_reliable) {
                                weather.pressure = bmp085.get_pressure()*760/101325;
                                if (!temperature_indoor_reliable) {
                                        weather.temperature_indoor = bmp085.get_temperature()/10;
                                        temperature_indoor_reliable = true;
                                }
                        }

                        flags.measure_indoor = 0;
                        flags.refresh_screen = 1;
                }

                /* Receive data from NRF24 */
                if (flags.nrf24_irq) {
                        nrf24_receive();
                        flags.nrf24_irq = 0;
                        flags.refresh_screen = 1;
                }

                /* Rotate screens */
                if (flags.enc_rotated_left || flags.enc_rotated_right) {
                        if (Gpio::read(enc_button)) {
                                constexpr auto n = static_cast<uint8_t>(ScreenX::nr_screens);
                                const auto i = static_cast<uint8_t>(screen.x);
                                screen.x = static_cast<ScreenX>(
                                        (flags.enc_rotated_right ? i + 1 : i + n - 1) % n);
                        } else {
                                constexpr auto n = static_cast<uint8_t>(ScreenY::nr_screens);
                                const auto i = static_cast<uint8_t>(screen.y);
                                screen.y = static_cast<ScreenY>(
                                        (flags.enc_rotated_right ? i + 1 : i + n - 1) % n);
                        }

                        atomic_write(s_inactivity_timer, 0);
                        flags.enc_rotated_left = 0;
                        flags.enc_rotated_right = 0;
                        flags.refresh_screen = 1;
                }

                /* Set matrix brightness */ 
                if (flags.light_changed) {
                        static uint8_t prev_brightness = 255;
                        uint8_t adc = atomic_read(s_light);

                        /* Ambient light level -> 0..6 linearly */
                        uint8_t brightness = adc*6/full_brightness;
                        brightness = min<uint8_t>(brightness, 6);

                        /* Add hysteresis */
                        int16_t diff = adc - prev_brightness*full_brightness/6;
                        if (brightness != prev_brightness && abs(diff) > hysteresis) {
                                matrix.set_brightness(brightness*2+3);  /* Map 0..6 to 3,5..15 */
                                prev_brightness = brightness;
                        }
                        flags.light_changed = 0;
                }

                /* Reset screen */
                if (flags.reset_screen) {
                        screen.x = ScreenX::clock;
                        screen.y = ScreenY::current;
                        flags.reset_screen = 0;
                        flags.refresh_screen = 1;
                }

                /* Save current weather to the history */
                if (flags.update_history) {
                        history.weather[history.current] = weather;
                        history.current = (history.current + 1) % history_size;
                        flags.update_history = 0;
                }

                /* Show clock, current weather and warning marks */
                if (flags.refresh_screen && (screen.x == ScreenX::clock || screen.y == ScreenY::current)) {
                        switch (screen.x) {
                        case ScreenX::clock: {
                                auto time = atomic_read(s_time);
                                matrix.printf(PSTR("\r%02u%02u"), time.h, time.m);
                                matrix.draw_point(11, 0, time.s % 2);
                                auto uptime = atomic_read(s_uptime);
                                matrix.draw_point(23, 0, uptime - clock_recent > clock_reliable_time);
                                break;
                        }
                        case ScreenX::temperature_outdoor:
                                if (weather.temperature_outdoor != bad_temperature) {
                                        matrix.printf(PSTR("\rO%+3d"), weather.temperature_outdoor);
                                        auto uptime = atomic_read(s_uptime);
                                        matrix.draw_point(23, 0, uptime - outdoor_recent > outdoor_reliable_time);
                                        matrix.draw_point(23, 7, battery_level < low_battery_level);
                                } else
                                        matrix.printf(PSTR("\rO---"));
                                break;
                        case ScreenX::temperature_indoor:
                                if (weather.temperature_indoor != bad_temperature) {
                                        matrix.printf(PSTR("\rI%+3d"), weather.temperature_indoor);
                                        matrix.draw_point(23, 0, !temperature_indoor_reliable);
                                } else
                                        matrix.printf(PSTR("\rI---"));
                                break;
                        case ScreenX::humidity:
                                if (weather.humidity != bad_humidity) {
                                        matrix.printf(PSTR("\rH%3u"), weather.humidity);
                                        matrix.draw_point(23, 0, !humidity_reliable);
                                } else
                                        matrix.printf(PSTR("\rH---"));
                                break;
                        case ScreenX::pressure:
                                if (weather.pressure != bad_pressure) {
                                        matrix.printf(PSTR("\rP%3u"), weather.pressure);
                                        matrix.draw_point(23, 0, !pressure_reliable);
                                } else
                                        matrix.printf(PSTR("\rP---"));
                                break;
                        default:
                                break;
                        }

                        matrix.sync();
                        flags.refresh_screen = 0;
                }

                /* Show the weather change for 24 hours */
                if (flags.refresh_screen && screen.x != ScreenX::clock && screen.y == ScreenY::change) {
                        Weather old = history.weather[(history.current + 1) % history_size];
                        int16_t diff = INT16_MAX;

                        switch (screen.x) {
                        case ScreenX::temperature_outdoor:
                                if (weather.temperature_outdoor != bad_temperature
                                                && old.temperature_outdoor != bad_temperature)
                                        diff = weather.temperature_outdoor - old.temperature_outdoor;
                                break;
                        case ScreenX::temperature_indoor:
                                if (weather.temperature_indoor != bad_temperature
                                                && old.temperature_indoor != bad_temperature)
                                        diff = weather.temperature_indoor - old.temperature_indoor;
                                break;
                        case ScreenX::humidity:
                                if (weather.humidity != bad_humidity
                                                && old.humidity != bad_humidity)
                                        diff = weather.humidity - old.humidity;
                                break;
                        case ScreenX::pressure:
                                if (weather.pressure != bad_pressure
                                                && old.pressure != bad_pressure)
                                        diff = weather.pressure - old.pressure;
                                break;
                        default:
                                break;
                        }

                        if (diff != INT16_MAX) {
                                matrix.printf(
                                        PSTR("\r%c%3u"),
                                        diff < 0 ? Matrix::special_down_arrow : Matrix::special_up_arrow,
                                        abs(diff));
                        } else
                                matrix.printf(PSTR("\r%c---"), Matrix::special_up_arrow);

                        matrix.sync();
                        flags.refresh_screen = 0;
                }

                /* Show minimal values of weather parameters for 24 hours */
                if (flags.refresh_screen && screen.x != ScreenX::clock && screen.y == ScreenY::minimal) {
                        switch (screen.x) {
                        case ScreenX::temperature_outdoor: {
                                auto m = weather.temperature_outdoor;
                                for (const auto &w : history.weather)
                                        if (w.temperature_outdoor != bad_temperature)
                                                m = min(m, w.temperature_outdoor);

                                if (m != bad_temperature)
                                        matrix.printf(PSTR("\r%c%+3d"), Matrix::special_min, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_min);
                                break;
                        }
                        case ScreenX::temperature_indoor: {
                                auto m = weather.temperature_indoor;
                                for (const auto &w : history.weather)
                                        if (w.temperature_indoor != bad_temperature)
                                                m = min(m, w.temperature_indoor);

                                if (m != bad_temperature)
                                        matrix.printf(PSTR("\r%c%+3d"), Matrix::special_min, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_min);
                                break;
                        }
                        case ScreenX::humidity: {
                                auto m = weather.humidity;
                                for (const auto &w : history.weather)
                                        if (w.humidity != bad_humidity)
                                                m = min(m, w.humidity);

                                if (m != bad_humidity)
                                        matrix.printf(PSTR("\r%c%3u"), Matrix::special_min, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_min);
                                break;
                        }
                        case ScreenX::pressure: {
                                auto m = weather.pressure;
                                for (const auto &w : history.weather)
                                        if (w.pressure != bad_pressure)
                                                m = min(m, w.pressure);

                                if (m != bad_pressure)
                                        matrix.printf(PSTR("\r%c%3u"), Matrix::special_min, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_min);
                                break;
                        }
                        default:
                                break;
                        }

                        matrix.sync();
                        flags.refresh_screen = 0;
                }

                /* Show maximal values of wether parameters for 24 hours */
                if (flags.refresh_screen && screen.x != ScreenX::clock && screen.y == ScreenY::maximal) {
                        switch (screen.x) {
                        case ScreenX::temperature_outdoor: {
                                auto m = weather.temperature_outdoor;
                                for (const auto &w : history.weather)
                                        if (w.temperature_outdoor != bad_temperature)
                                                m = max(m, w.temperature_outdoor);

                                if (m != bad_temperature)
                                        matrix.printf(PSTR("\r%c%+3d"), Matrix::special_max, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_max);
                                break;
                        }
                        case ScreenX::temperature_indoor: {
                                auto m = weather.temperature_indoor;
                                for (const auto &w : history.weather)
                                        if (w.temperature_indoor != bad_temperature)
                                                m = max(m, w.temperature_indoor);

                                if (m != bad_temperature)
                                        matrix.printf(PSTR("\r%c%+3d"), Matrix::special_max, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_max);
                                break;
                        }
                        case ScreenX::humidity: {
                                auto m = weather.humidity;
                                for (const auto &w : history.weather)
                                        if (w.humidity != bad_humidity)
                                                m = max(m, w.humidity);

                                if (m != bad_humidity)
                                        matrix.printf(PSTR("\r%c%3u"), Matrix::special_max, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_max);
                                break;
                        }
                        case ScreenX::pressure: {
                                auto m = weather.pressure;
                                for (const auto &w : history.weather)
                                        if (w.pressure != bad_pressure)
                                                m = max(m, w.pressure);

                                if (m != bad_pressure)
                                        matrix.printf(PSTR("\r%c%3u"), Matrix::special_max, m);
                                else
                                        matrix.printf(PSTR("\r%c---"), Matrix::special_max);
                                break;
                        }
                        default:
                                break;
                        }

                        matrix.sync();
                        flags.refresh_screen = 0;
                }

                asm volatile ("wdr" ::: "memory");
        }

        return 0;  /* Never be here */
}
