/**
 * @file motor.c
 * @brief Drive the motor with L293D
 * @author Xiangyu Guo
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>

#define MOTOR_1_LEFT        4       // Control pin 7 on L293D,using GPIO 4
#define MOTOR_1_RIGTH       5       // Control pin 2 on L293D,using GPIO 5
#define MOTOR_1_ENABLE      6       // Control pin 1 on L293D,using GPIO 6
#define DIRECTION_BUTTON    15      // Using GPIO 15 as direction control
#define STARTSTOP_BUTTON    16      // Using GPIO 16 as start/stop control
#define INTERRUPT_INTERVAL  200     // Bouncing Interval

/**
 * @brief New motor direction, 
 * @var 0 as default(from pin2 to pin7)
 * @var 1 as reverse(from pin7 to pin2)
 * @note volatile tell the compiler not to cache the variable.
 */
volatile int g_new_direction = 1;
/**
 * @brief Current motor direction,
 * @var 0 as default(from pin2 to pin 7)
 * @var 1 as reverse(from pin7 to pin 2)
 */
int g_current_direction = 0;

/**
 * @brief Interrupt handler
 */
void isrDirectionButton (void) {
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();
    // Deal with bouncing issue
    if (current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) {
        // Change direction
        g_new_direction = (g_new_direction + 1) & 1;
        // Update time
        s_last_interrupt = current_interrupt;
    }
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
        digitalWrite(MOTOR_1_ENABLE, s_started);
        // Update time
        s_last_interrupt = current_interrupt;
    }
}

/**
 * @brief Reset All GPIO pins to 0.
 * @param sig valid signal number
 */
void resetAllGPIO (int sig) {
    digitalWrite(MOTOR_1_LEFT, 0);
    digitalWrite(MOTOR_1_RIGTH, 0);
    digitalWrite(MOTOR_1_ENABLE, 0);
    exit(sig);
}

int main()
{
    signal(SIGINT, &resetAllGPIO);

    printf("Raspberry Pi - Motor and Button\n");
    printf("==============================\n");

    // Initialize, setup all ports and register interrupt handler.
    if (wiringPiSetup() < 0)
        return errno;

    if (wiringPiISR(DIRECTION_BUTTON, INT_EDGE_FALLING, &isrDirectionButton) < 0)
        return errno;

    if (wiringPiISR(STARTSTOP_BUTTON, INT_EDGE_FALLING, &isrStartStopButton) < 0)
        return errno;

    // Setting related pin mode to output
    pinMode(MOTOR_1_LEFT, OUTPUT);
    pinMode(MOTOR_1_RIGTH, OUTPUT);
    pinMode(MOTOR_1_ENABLE, OUTPUT);
    // Initialize to "low" output
    digitalWrite(MOTOR_1_LEFT, 0);
    digitalWrite(MOTOR_1_RIGTH, 0);
    digitalWrite(MOTOR_1_ENABLE, 0);

    for (;;) {
        if (g_current_direction != g_new_direction) {
            // Reset to 0 first, avoid setting both pin 2 and pin 7 as high
            // in the same time, it may burn down the motor.
            digitalWrite(MOTOR_1_LEFT, 0);
            digitalWrite(MOTOR_1_RIGTH, 0);
            digitalWrite(g_new_direction ? MOTOR_1_LEFT : MOTOR_1_RIGTH, 1);
            g_current_direction = g_new_direction;
        }
    }
    return 0;
}