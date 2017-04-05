/**
 * @file spi.c
 * @brief spi communication with MCP3208 ADC.
 * @author Xiangyu Guo
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "spi_mcp3208.h"

#define SPI_CHIP_NUMBER     (0)         /**< Chip Select on Raspberrypi(0-1). */
#define SPI_CHIP_CHANNEL    (7)         /**< Channel on ADC(0-7). */
#define SPI_CHIP_SPEED      (100000)    /**< Frequency in Hz. */
#define SPI_CHIP_MAX_VALUE  (4095)      /**< Max digital value read from ADC */

#define SLEEP_1S            (1000000)   /**< Sleep 1s in us. */

#define VCC_3V              (3.3)       /**< Vcc high voltage 3.3v */

/**
 * @brief Read digital value from MCP3208 using wiringPi.
 * @return 0 on success; otherwise errno.
 * @note Reference MCP3208 Datasheet.
 */
int main(void)
{
    int value = 0;
    int i;
    spi_mcp3208_st *module_mcp3208;

    module_mcp3208 = mcp3208_module_init(SPI_CHIP_NUMBER, SPI_CHIP_SPEED);

    for (i = 0; i <= SPI_CHIP_CHANNEL; ++i)
    {
        value = mcp3208_read_data(module_mcp3208, i);
        printf("MCP3208 channel %d ADC Value = %04u, Voltage = %.3f\n",
                i, value, (VCC_3V/SPI_CHIP_MAX_VALUE) * value);
    }

    return 0;
}
