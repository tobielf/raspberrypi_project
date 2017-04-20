/**
 * @file pin_dht_11.h
 * @brief DHT_11 function declearation
 * @author Xiangyu Guo
 */
#ifndef __PIN_DHT_11_H__
#define __PIN_DHT_11_H__

typedef struct dht_data {
    double temperature;             /**< temperature data */
    double humidity;                /**< humidity data */
} dht_data_st;

/**
 * @brief read data from the module.
 * @param data [out] a valid output buffer.
 * @return 0 success, otherwise ENODATA
 */
int pin_dht_11_read(dht_data_st *data);

/**
 * @brief initialize the module.
 */
void pin_dht_11_init();
#endif