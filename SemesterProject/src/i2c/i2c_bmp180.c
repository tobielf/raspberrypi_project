/**
 * @file i2c_BMP180.c
 * @brief Bosch BMP180 over I2C
 * @author Xiangyu Guo
 * @see https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf
 * @note
 *   The BMP180 consists of a piezo-resistive sensor, an analog to digital 
 *   converter and a control unit with E2PROM and a serial I2C interface. 
 *   The BMP180 delivers the uncompensated value of pressure and temperature. 
 *   The E2PROM has stored 176 bit of individual calibration data.
 *   This is used to compensate offset, temperature dependence and other 
 *   parameters of the sensor.
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

#include "i2c_lib.h"
#include "i2c_bmp180.h"
#include "i2c_bmp180_macro.h"

/** 
 * @brief calibration data, file descriptor, and OSS
 * 
 * Read the datasheet for more detail.
 */
struct bmp180_module
{
    int fd;                         /**< file descriptor of the device */
    short A1, A2, A3;
    unsigned short A4, A5, A6;
    short B1, B2, MB, MC, MD;
    short OSS;                      /**< Oversampling Settings         */
};

/*
 * Oversampling Setting. See below for details.
 */
const int g_conversion_time_table[4] = {OVERSAMPLING_TIME_0, 
                                        OVERSAMPLING_TIME_1,
                                        OVERSAMPLING_TIME_2,
                                        OVERSAMPLING_TIME_3};
/* ================================
    Declaration of inner functions
   ================================ */
static int s_get_conversion_time(short OSS);
static void s_read_calibration_data(int fd, bmp180_module_st *bmp180);
static long s_read_raw_temperature(bmp180_module_st *bmp180);
static long s_read_raw_pressure(bmp180_module_st *bmp180);

/**
 * @brief Calculate true values from uncompensated values.
 * @param bmp180 [in] a initialized module
 * @param data [out] a valid data struct needs to be filled.
 * @return 0 on success; otherwise an errno will be return.
 * @note See Figure 4 in the datasheet for reference.
 */
int bmp180_read_data(bmp180_module_st *bmp180, bmp180_data_st *data) {
    long UT, UP, temp;
    long X1, X2, X3, B3, B5, B6, p;
    unsigned long B4, B7;
    
    // check parameters, invalid address will return EFAULT(14) bad address.
    if (bmp180 == NULL || data == NULL)
        return EFAULT;

    // read uncompensated temperature
    UT = s_read_raw_temperature(bmp180);
    // read uncompensated pressure
    UP = s_read_raw_pressure(bmp180);

    // calculate true temperature value
    X1 = ((UT - bmp180->A6) * bmp180->A5) >> SHIFT_15BITS;
    X2 = (bmp180->MC << SHIFT_11BITS) / (X1 + bmp180->MD);
    B5 = X1 + X2;
    temp = (B5 + BMP180_CALCULATE_TRUE_T) >> SHIFT_04BITS;
    data->temperature = (double)temp/BASE_OF_TEN;

    // calculate true pressure value
    B6 = B5 - 4000;
    X1 = (bmp180->B2 * ((B6 * B6) >> SHIFT_12BITS)) >> SHIFT_11BITS;
    X2 = (bmp180->A2 * B6) >> SHIFT_11BITS;
    X3 = X1 + X2;
    B3 = ((((bmp180->A1 << SHIFT_02BITS)+X3) << bmp180->OSS) + 2) >> SHIFT_02BITS;
    X1 = (bmp180->A3 * B6) >> SHIFT_13BITS;
    X2 = (bmp180->B1 * (B6 * B6 >> SHIFT_12BITS)) >> SHIFT_16BITS;
    X3 = ((X1 + X2) + 2) >> SHIFT_02BITS;
    B4 = (bmp180->A4 * (unsigned long)(X3 + (1 << SHIFT_15BITS))) >> SHIFT_15BITS;
    B7 = ((unsigned long)UP - B3) * (50000 >> bmp180->OSS);
    p = B7 < OVERFLOW_BIT ? (B7 << SHIFT_01BITS) / B4 : (B7 / B4) << SHIFT_01BITS;

    X1 = (p >> SHIFT_08BITS) * (p >> SHIFT_08BITS);
    X1 = (X1 * BMP180_PARAM_MG) >> SHIFT_16BITS;
    X2 = (BMP180_PARAM_MH * p) >> SHIFT_16BITS;
    data->pressure = p + ((X1 + X2 + BMP180_PARAM_MI) >> SHIFT_04BITS);

    // convert pressure to altitude
    data->altitude = PRESSURE_TO_ALTITUDE_CONSTANT * 
                        (1.0 - pow(((double)data->pressure/STANDARD_PRESSURE), 
                                    PRESSURE_TO_ALTITUDE_INDEX));
    return 0;
}

/**
 * @brief Initialize the module BMP180
 * @param OSS oversampling setting.
 * @return bmp180 a initialized, valid module.
 */
bmp180_module_st *bmp180_module_init(short OSS) {
    int fd;

    // check parameters, if out of range, set to default(0)
    if (OSS < BMP180_ULTRA_LOW_POWER || OSS > BMP180_ULTRA_HIGH_RESOLUTION)
        OSS = BMP180_ULTRA_LOW_POWER;

    if ((fd = wiringPiI2CSetup(DEVICE_ADDRESS)) < 0)
        exit(errno);

    bmp180_module_st *bmp180 = (bmp180_module_st *)malloc(sizeof(bmp180_module_st));
    if (bmp180 == NULL)
        exit(ENOMEM);

    bmp180->fd = fd;
    bmp180->OSS = OSS;

    // read calibration data from the E2PROM in BMP180.
    s_read_calibration_data(fd, bmp180);

    return bmp180;
}

/**
 * @brief Clean up the module BMP180
 * @param bmp180 a valid module.
 */
void bmp180_module_fini(bmp180_module_st *bmp180) {
    if (bmp180 != NULL) {
        close(bmp180->fd);
        free(bmp180);
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
    if (OSS < BMP180_ULTRA_LOW_POWER || OSS > BMP180_ULTRA_HIGH_RESOLUTION)
        OSS = BMP180_ULTRA_LOW_POWER;
    return g_conversion_time_table[OSS];
}

/**
 * @breif Read calibration data from the E2PROM.
 * @param fd file descriptor to BMP180 device.
 * @param bmp180 a valid bmp180 struct.
 * @note Quote from the Datasheet.
 * The E2PROM has stored 176 bit of individual calibration data.
 * This is used to compensate offset, temperature dependence and other parameters of the sensor.
 */
static void s_read_calibration_data(int fd, bmp180_module_st *bmp180) {
    bmp180->A1 = (i2c_read_8bits(fd, A1_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, A1_LSB);
    bmp180->A2 = (i2c_read_8bits(fd, A2_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, A2_LSB);
    bmp180->A3 = (i2c_read_8bits(fd, A3_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, A3_LSB);
    bmp180->A4 = (i2c_read_8bits(fd, A4_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, A4_LSB);
    bmp180->A5 = (i2c_read_8bits(fd, A5_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, A5_LSB);
    bmp180->A6 = (i2c_read_8bits(fd, A6_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, A6_LSB);
    bmp180->B1 = (i2c_read_8bits(fd, B1_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, B1_LSB);
    bmp180->B2 = (i2c_read_8bits(fd, B2_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, B2_LSB);
    bmp180->MB = (i2c_read_8bits(fd, MB_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, MB_LSB);
    bmp180->MC = (i2c_read_8bits(fd, MC_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, MC_LSB);
    bmp180->MD = (i2c_read_8bits(fd, MD_MSB) << SHIFT_08BITS) + i2c_read_8bits(fd, MD_LSB);
}

/**
 * @breif Read raw temperature data from the BMP180.
 * @param bmp180 a valid bmp180 struct.
 * @return UT uncompensated temperature value.
 * @note Reference from the Section3.5 in Datasheet.
 */
static long s_read_raw_temperature(bmp180_module_st *bmp180) {
    int MSB, LSB;
    long UT;
    // read uncompensated temperature value.
    i2c_write_8bits(bmp180->fd, BMP180_CTRL_MSG_REG, BMP180_READ_TEMPERATURE);    
    usleep(OVERSAMPLING_TIME_0);
    MSB = i2c_read_8bits(bmp180->fd, BMP180_ADC_OUT_MSB_REG);
    LSB = i2c_read_8bits(bmp180->fd, BMP180_ADC_OUT_LSB_REG);
    UT = (MSB << SHIFT_08BITS) + LSB;
    return UT;
}

/**
 * @breif Read raw pressure data from the BMP180.
 * @param bmp180 a valid bmp180 struct.
 * @return UP uncompensated pressure value.
 * @note Reference from the Section3.5 in Datasheet.
 */
static long s_read_raw_pressure(bmp180_module_st *bmp180) {
    int MSB, LSB, XLSB, timing;
    long UP;
    // read uncompensated pressure value.
    timing = s_get_conversion_time(bmp180->OSS);
    i2c_write_8bits(bmp180->fd, BMP180_CTRL_MSG_REG, 
                    BMP180_READ_PRESSURE + (bmp180->OSS << SHIFT_06BITS));
    usleep(timing);
    MSB  = i2c_read_8bits(bmp180->fd, BMP180_ADC_OUT_MSB_REG);
    LSB  = i2c_read_8bits(bmp180->fd, BMP180_ADC_OUT_LSB_REG);
    XLSB = i2c_read_8bits(bmp180->fd, BMP180_ADC_OUT_XLSB_REG);
    UP = ((MSB << SHIFT_16BITS) + (LSB << SHIFT_08BITS) + XLSB) >> 
                                        (BMP180_CALCULATE_TRUE_P - bmp180->OSS);
    return UP;
}

#ifdef XTEST

int main() {
    bmp180_data_st value;
    bmp180_module_st *test_bmp180 = bmp180_module_init(0);
    if (bmp180_read_data(test_bmp180, &value) == 0) {
        printf("SUCCESS!\n");
        printf("Temperature: %.2f\nAltitude: %.2f\nPressure: %.2f\n",
            value.temperature, value.altitude, value.pressure);
    } else {
        printf("FAILED\n");
    }

    bmp180_module_fini(test_bmp180);

    return 0;
}

#endif
