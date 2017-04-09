/**
 * @file pin_motor.c
 */
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "pin_motor.h"

#define MOTOR_1_LEFT        21      /**< Control pin 7 on L293D,using GPIO 21 */
#define MOTOR_1_RIGTH       22      /**< Control pin 2 on L293D,using GPIO 22 */
#define MOTOR_1_ENABLE      23      /**< Control pin 1 on L293D,using GPIO 23 */

static int initialized = LOW;

void motor_setup_up() {
    // Initialize, setup all ports and register interrupt handler.
    if (wiringPiSetup() < LOW)
        exit(errno);
    
    pinMode(MOTOR_1_LEFT, OUTPUT);
    pinMode(MOTOR_1_RIGTH, OUTPUT);
    pinMode(MOTOR_1_ENABLE, OUTPUT);

    digitalWrite(MOTOR_1_LEFT, LOW);
    digitalWrite(MOTOR_1_RIGTH, LOW);
    digitalWrite(MOTOR_1_ENABLE, LOW);

    initialized = HIGH;
}

void motor_turn_on() {
    if (!initialized)
        exit(ENODEV);

    digitalWrite(MOTOR_1_LEFT, HIGH);
    digitalWrite(MOTOR_1_RIGTH, LOW);
    digitalWrite(MOTOR_1_ENABLE, HIGH);
}

void motor_turn_off() {
    if (!initialized)
        exit(ENODEV);

    digitalWrite(MOTOR_1_LEFT, LOW);
    digitalWrite(MOTOR_1_RIGTH, LOW);
    digitalWrite(MOTOR_1_ENABLE, LOW);
}