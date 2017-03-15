/**
 * @file i2c.c
 * @brief i2c communication with BMP180
 * @author Xiangyu Guo
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "i2c_BMP180.h"

/**
 * @brief Read temperature and pressure from BMP180 using wiringPi.
 * @param argc numbers of arguments.
 * @param argv arguments vector.
 * @note Algorithm reference BMP180 Datasheet, Section 3.5, 3.6.
 */
int main(int argc, char **argv) {
    bmp180_module_st *bmp180;
    short OSS = 0;
    long pressure;

    double temperature, altitude;

    if (argc >= 2)
        OSS = (short)strtol(argv[1], NULL, 10);

    if (OSS < 0 || OSS > 3)
        OSS = 0; 
    
    bmp180 = bmp180_module_init(OSS);

    bmp180_read_data(bmp180, &temperature, &pressure, &altitude);

    printf("Temperature: %.1f *C\n", temperature);
    printf("Pressure: %ld Pa\n", pressure);
    printf("Altitude: %.2lf Meter\n", altitude);

    bmp180_module_fini(bmp180);

    return 0;
}