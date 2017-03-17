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
#include "i2c_BMP180_macro.h"

/**
 * @brief Read temperature and pressure from BMP180 using wiringPi.
 * @param argc numbers of arguments.
 * @param argv arguments vector.
 * @return 0, on success; otherwise errno.
 * @note Algorithm reference BMP180 Datasheet, Section 3.5, 3.6.
 */
int main(int argc, char **argv) {
    bmp180_module_st *bmp180;
    bmp180_data_st data;
    short OSS = ULTRA_LOW_POWER;

    if (argc >= 2)
        OSS = (short)strtol(argv[1], NULL, BASE_OF_TEN);
    else
        printf("You can setup OSS by calling with a param [0-3].\n");

    if (OSS < ULTRA_LOW_POWER || OSS > ULTRA_HIGH_RESOLUTION)
        OSS = ULTRA_LOW_POWER; 
    
    bmp180 = bmp180_module_init(OSS);

    if (bmp180_read_data(bmp180, &data) == 0) {
        printf("Temperature: %.1f *C\n", data.temperature);
        printf("Pressure: %ld Pa\n", data.pressure);
        printf("Altitude: %.2lf Meter\n", data.altitude);
    }

    bmp180_module_fini(bmp180);

    return 0;
}
