/**
 * @file i2c_lib.h
 * @brief definition of I2C operations.
 * @author Xiangyu Guo
 */
#ifndef __I2C_LIB_H__
#define __I2C_LIB_H__

/**
 * @brief Write value to a 8bits register in i2c device 
 *        without specified register address.
 * @param fd file descriptor to the device.
 * @param value the value going to write.
 * @note  wiringPiI2CWriteReg8 will finally return ioctl
 *        base on the manual page of ioctl, 0 indicate success
 *        otherwise non-zero means fail.
 */
void i2c_write(int fd, int value);

/**
 * @brief Read value from a 8bits register in i2c device
 *        without specified register address.
 * @param fd file descriptor to the device.
 * @param reg register address going to read.
 * @return value the value read from the device.
 * @note  wiringPiI2CReadReg8 will finally return ioctl
 *        and return -1 indicate fail, otherwise means success.
 */
int i2c_read(int fd);

/**
 * @brief Write value to a 8bits register in i2c device.
 * @param fd file descriptor to the device.
 * @param reg register address to write.
 * @param value the value going to write.
 * @note  wiringPiI2CWriteReg8 will finally return ioctl
 *        base on the manual page of ioctl, 0 indicate success
 *        otherwise non-zero means fail.
 */
void i2c_write_8bits(int fd, int reg, int value);

/**
 * @brief Read value from a 8bits register in i2c device.
 * @param fd file descriptor to the device.
 * @param reg register address going to read.
 * @return value the value read from the device.
 * @note  wiringPiI2CReadReg8 will finally return ioctl
 *        and return -1 indicate fail, otherwise means success.
 */
int i2c_read_8bits(int fd, int reg);

#endif