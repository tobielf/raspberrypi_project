#ifndef __I2C_BMP180_H__
#define __I2C_BMP180_H__

typedef struct bmp180_module bmp180_module_st;
struct bmp180_module;

bmp180_module_st *bmp180_module_init(short OSS);
void bmp180_module_fini(bmp180_module_st *data);

int bmp180_read_data(bmp180_module_st *data, 
							double *temperature,
							long *pressure,
							double *altitude);

#endif