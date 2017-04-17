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
#include "pin/pin_motor.h"
#include "pin/pin_dht_11.h"

#include "screen.h"
#include "web_server.h"

#define MAX_LIGHT_BOUNDRY   (4)     // LED from 0 - 7, 8 in total.

#define MOTION_DETECTOR     (29)
#define MECURY_SWITCH       (27)

#define INTERRUPT_INTERVAL  200     // Bouncing Interval

static void
syntax(void)
{
    fprintf(stdout, "Syntax: http-server <docroot>\n");
}

/**
 * @brief Interrupt handler
 */
void isrStartStopButton (void) {
    static unsigned long s_last_interrupt = 0;
    static int s_started = 0;
    unsigned long current_interrupt = millis();
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) {
        // Start or Stop
        s_started = (s_started + 1) & 1;
        if (s_started)
            motor_turn_on();
        else
            motor_turn_off();
        // Update time
        s_last_interrupt = current_interrupt;
    }
}

/**
 * @brief Interrupt handler
 */
void isrChangeDirectionButton (void) {
    static unsigned long s_last_interrupt = 0;
    static int s_started = 0;
    unsigned long current_interrupt = millis();
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) {
        // Start or Stop
        s_started = (s_started + 1) & 1;
        if (s_started)
            motor_turn_on();
        else
            motor_turn_off();
        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void 
setupMotor() {
    motor_setup_up();

    //if (wiringPiISR(MOTION_DETECTOR, INT_EDGE_FALLING, &isrChangeDirectionButton) < 0)
    //    exit(errno);

    //if (wiringPiISR(MECURY_SWITCH, INT_EDGE_FALLING, &isrChangeDirectionButton) < 0)
    //    exit(errno);
}

int
main(int argc, char **argv)
{
    struct event_base *base;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return errno;

    if (argc < 2) {
        syntax();
        return 1;
    }

    setupMotor();
    screen_display_get_instance();

    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Couldn't create an event_base: exiting\n");
        return 1;
    }

    web_server_init(base);

    event_base_dispatch(base);

    return 0;
}