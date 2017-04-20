/**
 * @file push_button.c
 * @brief flash the led from lower to higher
 *        press the button change the direction.
 * @author Xiangyu Guo
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>

#define MAX_LIGHT_BOUNDRY   (4)     // LED from 0 - 7, 8 in total.
#define BUTTON_PORT         (4)     // Button input on 8.
#define BLINK_INTERVAL      (100)   // Blink delay time
#define INTERRUPT_INTERVAL  (200)   // Bouncing Interval

/**
 * @brief LED direction, 
 * @var 0 as default(from lower pin to higher)
 * @var 1 as reverse(from higher pin to lower)
 * @note volatile tell the compiler not to cache the variable.
 */
static volatile int g_direction = 0;

/**
 * @brief led number lookup table.
 */
const static unsigned int g_led_numbers[MAX_LIGHT_BOUNDRY + 1] = {7, 0, 2, 3, 25};

/**
 * @brief Reset All LEDs to 0.
 * @param sig valid signal number
 */
void resetAllLEDs (int sig) {
    int i;
    for (i = 0; i <= MAX_LIGHT_BOUNDRY; ++i) {
        pinMode(g_led_numbers[i], OUTPUT);
        digitalWrite(g_led_numbers[i], 0);
    }
    exit(sig);
}

/**
 * @brief Interrupt handler
 */
void isrButton (void) {
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt < INTERRUPT_INTERVAL)
        return;
    // Change direction
    g_direction = ~g_direction;
    // Update time
    s_last_interrupt = current_interrupt;
}

/**
 * @brief Main Function
 * @return 0 for success, otherwise errno.
 */
int main (void)
{
    int i, led;

    signal(SIGINT, &resetAllLEDs);

    printf("Raspberry Pi - LED and Button\n");
    printf("==============================\n");

    // Initialize, setup all ports and register interrupt handler.
    if (wiringPiSetup() < 0)
        return errno;

    if (wiringPiISR(BUTTON_PORT, INT_EDGE_FALLING, &isrButton) < 0)
        return errno;

    for (i = 0; i <= MAX_LIGHT_BOUNDRY; ++i) {
        led = g_led_numbers[i];
        pinMode(led, OUTPUT);
        digitalWrite(led, 0);
    }

    // Main Loop
    for (;;)
    {
        for (i = 0; i <= MAX_LIGHT_BOUNDRY; ++i)
        {
            led = i;
            // If it is reversed direction(g_direction != 0)
            // get a complement of the led number.
            // otherwise, keep the original number.
            if (g_direction != 0)
                led = MAX_LIGHT_BOUNDRY - i;
            led = g_led_numbers[led];
            // blink the LED
            digitalWrite(led, 1);
            delay(BLINK_INTERVAL);
            digitalWrite(led, 0);
        }
    }
    return 0;
}