/**
 * @file spi_mcp3208.c
 * @brief Microchip MCP3208 over SPI
 * @author Xiangyu Guo
 * @see http://ww1.microchip.com/downloads/en/DeviceDoc/21298c.pdf
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <wiringPiSPI.h>

#include "spi_mcp3208.h"

#define MCP3208_MIN_SPEED           (100000)/**< MCP3208 Minium Frequency */
#define MCP3208_CHANNEL_NUMBERS     (0x07)  /**< MCP3208 total channels */
#define MCP3208_START_BIT           (0x04)  /**< MCP3208 Start signal */
#define MCP3208_SINGLE_BIT          (0x02)  /**< MCP3208 Single mode */
#define MCP3208_MASK_04BITS         (0x0F)  /**< MCP3208 lower 4 bits mask */

#define SHIFT_02BITS                (2)     /**< Shifting 02 bits */
#define SHIFT_06BITS                (6)     /**< Shifting 06 bits */
#define SHIFT_08BITS                (8)     /**< Shifting 08 bits */

/** 
 * @brief chip number, file descriptor, and speed
 * 
 * Read the datasheet for more detail.
 */
struct mcp3208_module
{
    int fd;                         /**< file descriptor of the device */
    unsigned int chip_number;       /**< number on the Raspberrypi(0-1) */
    unsigned int speed;             /**< communication frequency */
};

/**
 * @brief Initialize the module MCP3208
 * @param chip_number chip number on Raspberrypi pin, 0 or 1.
 * @param speed chip communication speed, in Hz.
 * @return mcp3208 a initialized, valid mcp3208_module_st.
 */
mcp3208_module_st *mcp3208_module_init(unsigned int chip_number, unsigned int speed) {
    int fd;
    mcp3208_module_st *mcp3208;

    if (speed < MCP3208_MIN_SPEED)
        speed = MCP3208_MIN_SPEED;

    chip_number &= 1;

    if ((fd = wiringPiSPISetup(chip_number, speed)) == -1) {
        fprintf(stderr, "wiringPiSPISetup Failed: %s\n", strerror(errno));
        exit(errno);
    }

    mcp3208 = (mcp3208_module_st *)malloc(sizeof(mcp3208_module_st));

    if (mcp3208 == NULL) {
        fprintf(stderr, "%s\n", strerror(ENOMEM));
        exit(ENOMEM);
    }

    mcp3208->fd = fd;
    mcp3208->chip_number = chip_number;
    mcp3208->speed = speed;

    return mcp3208;
}

/**
 * @brief Clean up the module mcp3208
 * @param mcp3208 a valid module.
 */
void mcp3208_module_fini(mcp3208_module_st *mcp3208) {
    if (mcp3208 != NULL) {
        close(mcp3208->fd);
        free(mcp3208);
    }
}

/**
 * @brief Read the value from ADC based on the specified channel.
 * @param mcp3208 initialized module.
 * @param channel valid channel number[0-7].
 * @return 0-4096 on success; otherwise exit with an error number.
 */
int mcp3208_read_data(mcp3208_module_st *mcp3208, unsigned int channel) {
    unsigned char buff[3];
    int ret = 0;

    if (mcp3208 == NULL)
        return 0;

    channel &= MCP3208_CHANNEL_NUMBERS;

    buff[0] = MCP3208_START_BIT | MCP3208_SINGLE_BIT | (channel >> SHIFT_02BITS);
    buff[1] = channel << SHIFT_06BITS;
    buff[2] = 0;

    if(wiringPiSPIDataRW(mcp3208->chip_number, buff, 3) == -1) {
        fprintf(stderr, "%s\n", strerror(errno));
        exit(errno);
    }

    buff[1] = MCP3208_MASK_04BITS & buff[1];
    ret = (buff[1] << SHIFT_08BITS) | buff[2];

    return ret;
}
