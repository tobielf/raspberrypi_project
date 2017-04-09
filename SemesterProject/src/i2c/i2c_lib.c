/**
 * @file i2c_lib.c
 * @brief implemetation of I2C operations
 * @author Xiangyu Guo
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPiI2C.h>

#include "i2c_lib.h"

/**
 * @brief Write value to a 8bits register in i2c device 
 *        without specified register address.
 * @param fd file descriptor to the device.
 * @param value the value going to write.
 * @note  wiringPiI2CWriteReg8 will finally return ioctl
 *        base on the manual page of ioctl, 0 indicate success
 *        otherwise non-zero means fail.
 */
void i2c_write(int fd, int value) {
    if (wiringPiI2CWrite(fd, value) == -1) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
}

/**
 * @brief Read value from a 8bits register in i2c device
 *        without specified register address.
 * @param fd file descriptor to the device.
 * @param reg register address going to read.
 * @return value the value read from the device.
 * @note  wiringPiI2CReadReg8 will finally return ioctl
 *        and return -1 indicate fail, otherwise means success.
 */
int i2c_read(int fd) {
    int ret_val;

    if ((ret_val = wiringPiI2CRead(fd)) == -1) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
    return ret_val;
}

/**
 * @brief Write value to a 8bits register in i2c device.
 * @param fd file descriptor to the device.
 * @param reg register address to write.
 * @param value the value going to write.
 * @note  wiringPiI2CWriteReg8 will finally return ioctl
 *        base on the manual page of ioctl, 0 indicate success
 *        otherwise non-zero means fail.
 */
void i2c_write_8bits(int fd, int reg, int value) {
    if (wiringPiI2CWriteReg8(fd, reg, value) == -1) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
}

/**
 * @brief Read value from a 8bits register in i2c device.
 * @param fd file descriptor to the device.
 * @param reg register address going to read.
 * @return value the value read from the device.
 * @note  wiringPiI2CReadReg8 will finally return ioctl
 *        and return -1 indicate fail, otherwise means success.
 */
int i2c_read_8bits(int fd, int reg) {
    int ret_val;

    if ((ret_val = wiringPiI2CReadReg8(fd, reg)) == -1) {
        printf("%s\n", strerror(errno));
        exit(errno);
    }
    return ret_val;
}