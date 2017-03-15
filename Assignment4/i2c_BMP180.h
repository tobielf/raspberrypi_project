#ifndef __I2C_BMP180_H__
#define __I2C_BMP180_H__

typedef struct bmp180_data bmp180_data_st;
struct bmp180_data;

bmp180_data_st *bmp180_module_init(short OSS);
void bmp180_module_fini(bmp180_data_st *data);

double bmp180_read_temperature(bmp180_data_st *data);
long bmp180_read_pressure_altitude(bmp180_data_st *data, double *altitude);

#endif