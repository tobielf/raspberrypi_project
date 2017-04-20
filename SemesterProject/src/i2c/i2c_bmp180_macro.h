/**
 * @file i2c_BMP180_macro.h
 * @brief All the macro definations about the module.
 * @author Xiangyu Guo
 */
#ifndef __I2C_BMP180_MACRO_H__
#define __I2C_BMP180_MACRO_H__

/* ===========
    Constants
   =========== */
#define DEVICE_ADDRESS              (0x77)  /**< device address */

#define SHIFT_01BITS                (1)     /**< Shifting 01 bits */
#define SHIFT_02BITS                (2)     /**< Shifting 02 bits */
#define SHIFT_04BITS                (4)     /**< Shifting 04 bits */
#define SHIFT_06BITS                (6)     /**< Shifting 06 bits */
#define SHIFT_08BITS                (8)     /**< Shifting 08 bits */
#define SHIFT_11BITS                (11)    /**< Shifting 11 bits */
#define SHIFT_12BITS                (12)    /**< Shifting 12 bits */
#define SHIFT_13BITS                (13)    /**< Shifting 13 bits */
#define SHIFT_15BITS                (15)    /**< Shifting 15 bits */
#define SHIFT_16BITS                (16)    /**< Shifting 16 bits */

#define STANDARD_PRESSURE           (101325)    /**< Sea level, 15*C */

#define BASE_OF_TEN                 (10)    /**< Decimal conversion  */

/* ==============================
    Sensor related Magic Numbers
   ============================== */
#define OVERSAMPLING_TIME_0         (4500)  /**< 1 sampling cycle  */
#define OVERSAMPLING_TIME_1         (7500)  /**< 2 sampling cycles */
#define OVERSAMPLING_TIME_2         (13500) /**< 4 sampling cycles */
#define OVERSAMPLING_TIME_3         (25500) /**< 8 sampling cycles */

#define OVERFLOW_BIT                (0x80000000)

#define PRESSURE_TO_ALTITUDE_CONSTANT   (44330.0)
#define PRESSURE_TO_ALTITUDE_INDEX      (1/5.255)

#define BMP180_PARAM_MG             (3038)
#define BMP180_PARAM_MH             (-7357)
#define BMP180_PARAM_MI             (3791)

#define BMP180_CALCULATE_TRUE_T     (8)
#define BMP180_CALCULATE_TRUE_P     (8)

/* ============================
    Addresses and instructions
   ============================ */
#define BMP180_READ_TEMPERATURE     (0x2E)
#define BMP180_READ_PRESSURE        (0x34)

#define BMP180_CTRL_MSG_REG         (0xF4)

#define BMP180_ADC_OUT_MSB_REG      (0xF6)
#define BMP180_ADC_OUT_LSB_REG      (0xF7)
#define BMP180_ADC_OUT_XLSB_REG     (0xF8)

#define A1_MSB                      (0xAA)
#define A1_LSB                      (0xAB)
#define A2_MSB                      (0xAC)
#define A2_LSB                      (0xAD)
#define A3_MSB                      (0xAE)
#define A3_LSB                      (0xAF)
#define A4_MSB                      (0xB0)
#define A4_LSB                      (0xB1)
#define A5_MSB                      (0xB2)
#define A5_LSB                      (0xB3)
#define A6_MSB                      (0xB4)
#define A6_LSB                      (0xB5)
#define B1_MSB                      (0xB6)
#define B1_LSB                      (0xB7)
#define B2_MSB                      (0xB8)
#define B2_LSB                      (0xB9)
#define MB_MSB                      (0xBA)
#define MB_LSB                      (0xBB)
#define MC_MSB                      (0xBC)
#define MC_LSB                      (0xBD)
#define MD_MSB                      (0xBE)
#define MD_LSB                      (0xBF)

#endif