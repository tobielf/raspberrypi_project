/**
 * @file i2c_BMP180.h
 * @brief Public function declaration and communication data.
 * @author Xiangyu Guo
 */
#ifndef __I2C_BMP180_H__
#define __I2C_BMP180_H__

#define BMP180_ULTRA_LOW_POWER             (0)     /**< Ultra low power mode       */
#define BMP180_STANDARD                    (1)     /**< Standard mode              */
#define BMP180_HIGH_RESOLUTION             (2)     /**< High resolution mode       */
#define BMP180_ULTRA_HIGH_RESOLUTION       (3)     /**< Ultra high resolution mode */

/**
 * @brief communication data structure.
 */
typedef struct bmp180_data {
    double temperature;         /**< temperature data */
    double altitude;            /**< altitude data    */
    double pressure;            /**< pressure data    */
} bmp180_data_st;

/**
 * @brief module structure, hiding the detail to the public
 */
typedef struct bmp180_module bmp180_module_st;
struct bmp180_module;

/* =============================================
	sensor module initialize and finish function 
   ============================================= */
/**
 * @brief Initialize the module BMP180
 * @param OSS oversampling setting.
 * @return bmp180 a initialized, valid module_st.
 */
bmp180_module_st *bmp180_module_init(short OSS);
/**
 * @brief Clean up the module BMP180
 * @param bmp180 a valid module.
 */
void bmp180_module_fini(bmp180_module_st *module);

/* =================
    sensor function 
   ================= */
/**
 * @brief Calculate true values from uncompensated values.
 * @param bmp180 [in] initialized module
 * @param data [out] valid data struct needs to be filled.
 * @return 0 on success; otherwise an errno will be return.
 * @note See Figure 4 in the datasheet for reference.
 */
int bmp180_read_data(bmp180_module_st *module, bmp180_data_st *data);

#endif