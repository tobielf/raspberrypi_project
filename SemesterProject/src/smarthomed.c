/**
 * @file smarthomed.c
 * @brief Smart Home
 *      1. Alarm System (Motion detector, Mecury switch, MCP3208 ADC[Brightness, Threshold])
 *      2. Screen Display (Update display)
 *      3. Temperature setup_motor_event(base);
 *      5. Web Server for Siri. (LED lights, MCP3208 ADC[status])
    web_server_init(base);
 * @author Xiangyu Guo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include <event2/event.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <wiringPi.h>

#include "spi/spi_mcp3208.h"
#include "i2c/i2c_bmp180.h"
#include "i2c/i2c_lcd1620.h"
#include "pin/pin_motor.h"
#include "pin/pin_dht_11.h"

#include "screen.h"
#include "web_server.h"

#define MOTION_DETECTOR     (29)        /**< */
#define MECURY_SWITCH       (27)        /**< */

#define INTERRUPT_INTERVAL  (200)       /**< Bouncing Interval */

#define TIMEOUT_SEC         (3)         /**< Temperature Event Time out */

#define TEMPERATURE_LOWEST  (10)        /**< Lowest temperature to turn on the fan */
#define TEMPERATURE_RANGE   (30)        /**< Temperature range conver from ADC */

/**
 * @brief Interrupt handler
 */
void isr_motion_detector (void) {
    int brightness;
    int brightness_threshold;
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();

    mcp3208_module_st *mcp3208 = NULL;
    
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) {
        mcp3208 = mcp3208_module_get_instance();

        // Receive the brightness (Ch5)
        brightness = mcp3208_read_data(mcp3208, MCP3208_CHANNEL_5);

        // Compare with the threshold (Ch6)
        brightness_threshold = mcp3208_read_data(mcp3208, MCP3208_CHANNEL_6);

        printf("====Motion====\n");
        printf("Mercury Switcher: %d\nBrightness: %d\nBrightness threshold: %d\n", 
                digitalRead(MECURY_SWITCH), brightness, brightness_threshold);
        // Check with Mercury Switcher.
        if (digitalRead(MECURY_SWITCH) == 0 && brightness < brightness_threshold) {
            digitalWrite(24, 1);
        } else {
            digitalWrite(24, 0);
        }

        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void setup_alram_system() {

    if (wiringPiSetup() < LOW)
        exit(errno);

    pinMode(MECURY_SWITCH, INPUT);
    pinMode(24, OUTPUT);

    if (wiringPiISR(MOTION_DETECTOR, INT_EDGE_FALLING, &isr_motion_detector) < 0)
        exit(errno);
}

static void 
update_display_callback(evutil_socket_t fd, short flags, void *data) {
    screen_update_display();
}

static void 
check_temperature_callback(evutil_socket_t fd, short flags, void *data) {
    mcp3208_module_st *mcp3208 = mcp3208_module_get_instance();
    bmp180_module_st *bmp180 = bmp180_module_init(BMP180_ULTRA_HIGH_RESOLUTION);
    bmp180_data_st value;
    double temperature_threshold = 0;

    bmp180_read_data(bmp180, &value);
    
    // Read temerpature thresh_hold (Ch7).
    temperature_threshold = mcp3208_read_data(mcp3208, MCP3208_CHANNEL_7);
    temperature_threshold = temperature_threshold / MCP3208_MAX_VALUE *
                                     TEMPERATURE_RANGE + TEMPERATURE_LOWEST;

    printf("====Temperature====\n");
    printf("Current:%.2f\n", value.temperature);
    printf("Setting:%.2f\n", temperature_threshold);
    if (value.temperature > temperature_threshold)
        motor_turn_on();
    else
        motor_turn_off();

    bmp180_module_fini(bmp180);
}

static void setup_motor_event(struct event_base *base) {
    struct timeval tv;
    struct event *temperature_check_event;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    temperature_check_event = event_new(base, -1, 
                EV_TIMEOUT | EV_PERSIST, check_temperature_callback, NULL);

    if (!temperature_check_event) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        exit(errno);
    }
    event_add(temperature_check_event, &tv);

    motor_setup_up();
}

static void setup_update_event(struct event_base *base) {
    struct timeval tv;
    struct event *update_display_event;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    update_display_event = event_new(base, -1, 
                EV_TIMEOUT | EV_PERSIST, update_display_callback, NULL);

    if (!update_display_event) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        exit(errno);
    }
    event_add(update_display_event, &tv);
}

int main(int argc, char **argv)
{
    struct event_base *base;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return errno;

    setup_alram_system();

    screen_display_get_instance();

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        return 1;
    }

    setup_motor_event(base);

    setup_update_event(base);

    web_server_init(base);

    event_base_dispatch(base);

    mcp3208_module_clean_up();

    return 0;
}
