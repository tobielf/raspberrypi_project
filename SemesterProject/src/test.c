#include <stdio.h>
#include <string.h>
#include <wiringPi.h>

#include "i2c/i2c_lcd1620.h"
#include "i2c/i2c_bmp180.h"
#include "spi/spi_mcp3208.h"

#define SPI_CHIP_NUMBER     (0)         /**< Chip Select on Raspberrypi(0-1). */
#define SPI_CHIP_CHANNEL    (7)         /**< Channel on ADC(0-7). */
#define SPI_CHIP_SPEED      (100000)    /**< Frequency in Hz. */
#define SPI_CHIP_MAX_VALUE  (4095)      /**< Max digital value read from ADC */

#define SLEEP_1S            (1000000)   /**< Sleep 1s in us. */

#define VCC_3V              (3.3)       /**< Vcc high voltage 3.3v */

int main() {
    bmp180_module_st *bmp180 = bmp180_module_init(3);
    lcd1620_module_st *lcd = lcd1620_module_init();
    mcp3208_module_st *mcp3208 = mcp3208_module_init(0, 100000);
    bmp180_data_st result;
    char str[1024];
    int i, value = 0;

    bmp180_read_data(bmp180, &result);
    lcd1620_module_write_string(lcd, 0, 0, "Temperature:", strlen("Temperature:"));
    sprintf(str, "%.1f *C", result.temperature);    
    lcd1620_module_write_string(lcd, 0, 1, str, strlen(str));
    delay(5000);
    lcd1620_module_clear(lcd);
    for (i = 0; i <= SPI_CHIP_CHANNEL; ++i)
    {
        value = mcp3208_read_data(mcp3208, i);
        sprintf(str, "Channel %d:", i);
        lcd1620_module_write_string(lcd, 0, 0, str, strlen(str));
        sprintf(str, "ADC Value: %04u", value);
        lcd1620_module_write_string(lcd, 0, 1, str, strlen(str));
        delay(5000);
        lcd1620_module_clear(lcd);
    }
    printf("1\n");
    mcp3208_module_fini(mcp3208);
    bmp180_module_fini(bmp180);
    lcd1620_module_fini(lcd);
    return 0;
}