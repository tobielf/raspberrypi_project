/**
 * @file i2c_BMP180.c
 * @brief Bosch BMP180 experimental driver over I2
 * @author Xiangyu Guo
 * @note datasheet https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf
 */
#include <math.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPiI2C.h>

#define DEBUG 0
#define DEVICE_ADDRESS 0x77     // device address

//set the oversampling value (OSS 0..3) - this affects the precision of pressure measurement and
//the current required to get one reading
// OSS value | +/- pressure (hPa) | +/- altitude (m) | av. current at 1 sample/sec (uA) | no. of samples
//         0         |             0.06                 |             0.5                |                             3                                    |                 1
//         1         |             0.05                 |             0.4                |                             5                                    |                 2
//         2         |             0.04                 |             0.3                |                             7                                    |                 4
//         3         |             0.03                 |             0.25             |                             12                                 |                 8
#define OSS 0

// see BMP085 data sheet
short AC1, AC2, AC3;
unsigned short AC4, AC5, AC6;
short B1, B2, MB, MC, MD;
long B5, UP, UT, temp, pressure;

// each OSS value requires a different max timing (in us)
// this usually could be done quicker - see data sheet
int get_timing() {
    if ( OSS == 0 ) return 4500;    
    if ( OSS == 1 ) return 7500;
    if ( OSS == 2 ) return 13500;
    if ( OSS == 3 ) return 25500;
    return 25500;
}

// reading factory calibration data - call this first
// 11 16 bit words from 2 x 8 bit registers each, MSB comes first
void read_calibration_values(int fd) {
    AC1 = (wiringPiI2CReadReg8 (fd, 0xAA) << 8) + wiringPiI2CReadReg8 (fd, 0xAB);
    AC2 = (wiringPiI2CReadReg8 (fd, 0xAC) << 8) + wiringPiI2CReadReg8 (fd, 0xAD);
    AC3 = (wiringPiI2CReadReg8 (fd, 0xAE) << 8) + wiringPiI2CReadReg8 (fd, 0xAF);
    AC4 = (wiringPiI2CReadReg8 (fd, 0xB0) << 8) + wiringPiI2CReadReg8 (fd, 0xB1);
    AC5 = (wiringPiI2CReadReg8 (fd, 0xB2) << 8) + wiringPiI2CReadReg8 (fd, 0xB3);
    AC6 = (wiringPiI2CReadReg8 (fd, 0xB4) << 8) + wiringPiI2CReadReg8 (fd, 0xB5);
    B1  = (wiringPiI2CReadReg8 (fd, 0xB6) << 8) + wiringPiI2CReadReg8 (fd, 0xB7);
    B2  = (wiringPiI2CReadReg8 (fd, 0xB8) << 8) + wiringPiI2CReadReg8 (fd, 0xB9);
    MB  = (wiringPiI2CReadReg8 (fd, 0xBA) << 8) + wiringPiI2CReadReg8 (fd, 0xBB);
    MC  = (wiringPiI2CReadReg8 (fd, 0xBC) << 8) + wiringPiI2CReadReg8 (fd, 0xBD);
    MD  = (wiringPiI2CReadReg8 (fd, 0xBE) << 8) + wiringPiI2CReadReg8 (fd, 0xBF);
    if ( DEBUG == 1 ) printf("AC1 = %d \nAC2 = %d \nAC3 = %d \nAC4 = %d \nAC5 = %d \nAC6 = %d \nB1 = %d \nB2 = %d \nMB = %d \nMC = %d \nMD = %d \n", 
         AC1, AC2, AC3, AC4, AC5, AC6, B1, B2, MB, MC, MD);
}


// the pressure measurement depends on the temperature measurement
// hence it's done in the same method
void calculate_true_value(long UT, long UP) {
    long X1, X2, X3, B3, B6, p;
    unsigned long B4, B7;

    X1 = ((UT - AC6) * AC5) >> 15;
    X2 = (MC << 11) / (X1 + MD);
    B5 = X1 + X2;
    temp = (B5 + 8) >> 4;
    
    B6 = B5 - 4000;
    X1 = (B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = (AC2 * B6) >> 11;
    X3 = X1 + X2;
    B3 = (((AC1*4+X3) << OSS) + 2) >> 2;
    X1 = (AC3 * B6) >> 13;
    X2 = (B1 * (B6 * B6 >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    B4 = (AC4 * (unsigned long)(X3 + 32768)) >> 15;
    B7 = ((unsigned long)UP - B3) * (50000 >> OSS);
    p = B7 < 0x80000000 ? (B7 * 2) / B4 : (B7 / B4) * 2;

    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    pressure = p + ((X1 + X2 + 3791) >> 4);
}

int main(int argc, char **argv) {
    int fd;
    int MSB, LSB, XLSB, timing;
    double temperature, altitude;

    if ((fd = wiringPiI2CSetup(DEVICE_ADDRESS)) < 0)
        return errno;

    read_calibration_values(fd);

    //read uncompensated temperature value
    wiringPiI2CWriteReg8(fd, 0xF4, 0x2E);    
    usleep(4500);
    MSB = wiringPiI2CReadReg8(fd, 0xF6);
    LSB = wiringPiI2CReadReg8(fd, 0xF7);
    UT = (MSB << 8) + LSB;

    //read uncompensated pressure value
    timing = get_timing();
    wiringPiI2CWriteReg8(fd, 0xF4, 0x34 + (OSS << 6));
    usleep(timing);
    MSB = wiringPiI2CReadReg8(fd, 0xF6);
    LSB = wiringPiI2CReadReg8(fd, 0xF7);
    XLSB = wiringPiI2CReadReg8(fd, 0xF8);
    UP = ((MSB << 16) + (LSB << 8) + XLSB) >> (8 - OSS);

    calculate_true_value(UT, UP);
    temperature = temp/10;
    altitude = 44330.0 * (1.0 - pow(((double)pressure/101325), (1/5.255)));
    printf("Temperature: %.1f *C\nPressure: %d Pa\n", temperature, (int) pressure);
    printf("Altitude: %.2lf Meter\n", altitude);
    return 0;
}
