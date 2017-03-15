/**
 * @file i2c_BMP180.c
 * @brief Bosch BMP180 over I2C
 * @author Xiangyu Guo
 * @note datasheet https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf
 *
 *   The BMP180 consists of a piezo-resistive sensor, an analog to digital converter
 *   and a control unit with E2PROM and a serial I2C interface. 
 *   The BMP180 delivers the uncompensated value of pressure and temperature. 
 *   The E2PROM has stored 176 bit of individual calibration data.
 *   This is used to compensate offset, temperature dependence and other parameters of the sensor.
 *   UP = pressure data (16 to 19 bit)
 *   UT = temperature data (16 bit)
 */
#include <math.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPiI2C.h>

#include "i2c_BMP180.h"

#define DEBUG 0

#define DEVICE_ADDRESS  0x77     // device address (command: "i2cdetect -y 1")

#define ULTRA_LOW_POWER         0
#define STANDARD                1
#define HIG_RESOLUTION          2
#define ULTRA_HIGH_RESOLUTION   3

#define A1_MSB                  0xAA
#define A1_LSB                  0xAB
#define A2_MSB                  0xAC
#define A2_LSB                  0xAD
#define A3_MSB                  0xAE
#define A3_LSB                  0xAF
#define A4_MSB                  0xB0
#define A4_LSB                  0xB1
#define A5_MSB                  0xB2
#define A5_LSB                  0xB3
#define A6_MSB                  0xB4
#define A6_LSB                  0xB5
#define B1_MSB                  0xB6
#define B1_LSB                  0xB7
#define B2_MSB                  0xB8
#define B2_LSB                  0xB9
#define MB_MSB                  0xBA
#define MB_LSB                  0xBB
#define MC_MSB                  0xBC
#define MC_LSB                  0xBD
#define MD_MSB                  0xBE
#define MD_LSB                  0xBF

// calibration data, read the datasheet for more detail.
struct bmp180_module
{
    int fd;
    short A1, A2, A3;
    unsigned short A4, A5, A6;
    short B1, B2, MB, MC, MD;
    short OSS;
};

// Oversampling Setting. See below for details.
const int g_conversion_time_table[4] = {4500, 7500, 13500, 25500};

static int s_get_conversion_time(short OSS);
static void i2c_write_8bits(int fd, int reg, int value);
static int i2c_read_8bits(int fd, int reg);
static void s_read_calibration_data(int fd, bmp180_module_st *bmp180);
static long s_read_raw_temperature(bmp180_module_st *bmp180);
static long s_read_raw_pressure(bmp180_module_st *bmp180);

/**
 * @brief Calculate true values from uncompensated values.
 * @param bmp180, initialized module
 * @param UT, 
 * @param UP, 
 * @param *temp, true temperature
 * @param *pressure, true pressure
 * @note See Figure 4 in the datasheet for reference.
 */
int bmp180_read_data(bmp180_module_st *bmp180, 
                                    double *temperature,
                                    long *pressure,
                                    double *altitude) {
    long UT, UP, temp;
    long X1, X2, X3, B3, B5, B6, p;
    unsigned long B4, B7;
    
    // read uncompensated temperature
    UT = s_read_raw_temperature(bmp180);
    // read uncompensated pressure
    UP = s_read_raw_pressure(bmp180);

    X1 = ((UT - bmp180->A6) * bmp180->A5) >> 15;
    X2 = (bmp180->MC << 11) / (X1 + bmp180->MD);
    B5 = X1 + X2;
    temp = (B5 + 8) >> 4;
    *temperature = (double)temp/10;

    B6 = B5 - 4000;
    X1 = (bmp180->B2 * ((B6 * B6) >> 12)) >> 11;
    X2 = (bmp180->A2 * B6) >> 11;
    X3 = X1 + X2;
    B3 = (((bmp180->A1*4+X3) << bmp180->OSS) + 2) >> 2;
    X1 = (bmp180->A3 * B6) >> 13;
    X2 = (bmp180->B1 * (B6 * B6 >> 12)) >> 16;
    X3 = ((X1 + X2) + 2) >> 2;
    B4 = (bmp180->A4 * (unsigned long)(X3 + 32768)) >> 15;
    B7 = ((unsigned long)UP - B3) * (50000 >> bmp180->OSS);
    p = B7 < 0x80000000 ? (B7 * 2) / B4 : (B7 / B4) * 2;

    X1 = (p >> 8) * (p >> 8);
    X1 = (X1 * 3038) >> 16;
    X2 = (-7357 * p) >> 16;
    *pressure = p + ((X1 + X2 + 3791) >> 4);
    *altitude = 44330.0 * (1.0 - pow(((double)*pressure/101325), (1/5.255)));
    return 1;
}

bmp180_module_st *bmp180_module_init(short OSS) {
    int fd;

    if (OSS < ULTRA_LOW_POWER || OSS > ULTRA_HIGH_RESOLUTION)
        OSS = ULTRA_LOW_POWER;

    if ((fd = wiringPiI2CSetup(DEVICE_ADDRESS)) < 0)
        exit(errno);

    bmp180_module_st *data = (bmp180_module_st *)malloc(sizeof(bmp180_module_st));
    if (data == NULL)
        exit(ENOMEM);

    data->fd = fd;
    data->OSS = OSS;

    // read calibration data from the E2PROM in BMP180.
    s_read_calibration_data(fd, data);

    return data;
}

void bmp180_module_fini(bmp180_module_st *bmp180) {
    if (bmp180 != NULL) {
        close(bmp180->fd);
        free(bmp180);
        bmp180 = NULL;
    }
}

/**
 * @brief Get conversion time based on the OSS bit.
 * @return Time in [us].
 * @note Datasheet section 3.3.1, Table 3.
 *     Mode                OSS      Conversion time [ms]
 * ultra low power          0           4.5
 * standard                 1           7.5
 * high resolution          2           13.5
 * ultra high resolution    3           25.5
 */
static int s_get_conversion_time(short OSS) {
    if (OSS < ULTRA_LOW_POWER || OSS > ULTRA_HIGH_RESOLUTION)
        OSS = ULTRA_LOW_POWER;
    return g_conversion_time_table[OSS];
}

/**
 * @brief Write value to a 8bits register in i2c device.
 * @param fd, file descriptor to the device.
 * @param reg, register address to write.
 * @param value, the value going to write.
 * @note  wiringPiI2CWriteReg8 will finally return ioctl
 *        base on the manual page of ioctl, 0 indicate success
 *        otherwise non-zero means fail.
 */
static void i2c_write_8bits(int fd, int reg, int value) {
    if (wiringPiI2CWriteReg8(fd, reg, value)) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
}

/**
 * @brief Read value from a 8bits register in i2c device.
 * @param fd, file descriptor to the device.
 * @param reg, register address going to read.
 * @return value, the value read from the device.
 * @note  wiringPiI2CReadReg8 will finally return ioctl
 *        and return -1 indicate fail, otherwise means success.
 */
static int i2c_read_8bits(int fd, int reg) {
    int ret_val;

    if ((ret_val = wiringPiI2CReadReg8(fd, reg)) == -1) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
    return ret_val;
}

/**
 * @breif Read calibration data from the E2PROM.
 * @param fd, file descriptor to BMP180 device.
 * @note Quote from the Datasheet.
 * The E2PROM has stored 176 bit of individual calibration data.
 * This is used to compensate offset, temperature dependence and other parameters of the sensor.
 */
static void s_read_calibration_data(int fd, bmp180_module_st *bmp180) {
    bmp180->A1 = (i2c_read_8bits(fd, A1_MSB) << 8) + i2c_read_8bits(fd, A1_LSB);
    bmp180->A2 = (i2c_read_8bits(fd, A2_MSB) << 8) + i2c_read_8bits(fd, A2_LSB);
    bmp180->A3 = (i2c_read_8bits(fd, A3_MSB) << 8) + i2c_read_8bits(fd, A3_LSB);
    bmp180->A4 = (i2c_read_8bits(fd, A4_MSB) << 8) + i2c_read_8bits(fd, A4_LSB);
    bmp180->A5 = (i2c_read_8bits(fd, A5_MSB) << 8) + i2c_read_8bits(fd, A5_LSB);
    bmp180->A6 = (i2c_read_8bits(fd, A6_MSB) << 8) + i2c_read_8bits(fd, A6_LSB);
    bmp180->B1  = (i2c_read_8bits(fd, B1_MSB) << 8) + i2c_read_8bits(fd, B1_LSB);
    bmp180->B2  = (i2c_read_8bits(fd, B2_MSB) << 8) + i2c_read_8bits(fd, B2_LSB);
    bmp180->MB  = (i2c_read_8bits(fd, MB_MSB) << 8) + i2c_read_8bits(fd, MB_LSB);
    bmp180->MC  = (i2c_read_8bits(fd, MC_MSB) << 8) + i2c_read_8bits(fd, MC_LSB);
    bmp180->MD  = (i2c_read_8bits(fd, MD_MSB) << 8) + i2c_read_8bits(fd, MD_LSB);
    if ( DEBUG == 1 ) printf("A1 = %d \nA2 = %d \nA3 = %d \nA4 = %d \nA5 = %d \nA6 = %d \nB1 = %d \nB2 = %d \nMB = %d \nMC = %d \nMD = %d \n", 
     bmp180->A1, bmp180->A2, bmp180->A3, bmp180->A4, bmp180->A5, bmp180->A6, bmp180->B1, bmp180->B2, bmp180->MB, bmp180->MC, bmp180->MD);
}

static long s_read_raw_temperature(bmp180_module_st *bmp180) {
    int MSB, LSB;
    long UT;
    // read uncompensated temperature value.
    i2c_write_8bits(bmp180->fd, 0xF4, 0x2E);    
    usleep(4500);
    MSB = i2c_read_8bits(bmp180->fd, 0xF6);
    LSB = i2c_read_8bits(bmp180->fd, 0xF7);
    UT = (MSB << 8) + LSB;
    return UT;
}

static long s_read_raw_pressure(bmp180_module_st *bmp180) {
    int MSB, LSB, XLSB, timing;
    long UP;
    // read uncompensated pressure value.
    timing = s_get_conversion_time(bmp180->OSS);
    i2c_write_8bits(bmp180->fd, 0xF4, 0x34 + (bmp180->OSS << 6));
    usleep(timing);
    MSB = i2c_read_8bits(bmp180->fd, 0xF6);
    LSB = i2c_read_8bits(bmp180->fd, 0xF7);
    XLSB = i2c_read_8bits(bmp180->fd, 0xF8);
    UP = ((MSB << 16) + (LSB << 8) + XLSB) >> (8 - bmp180->OSS);
    return UP;
}
