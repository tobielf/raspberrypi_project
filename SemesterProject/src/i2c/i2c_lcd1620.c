/**
 * @file i2c_lcd1620.c
 * @brief lcd1620 driver over i2c communication
 * @author Xiangyu Guo
 */
#include <math.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "i2c_lib.h"
#include "i2c_lcd1620.h"
#include "i2c_lcd1620_macro.h"

struct lcd1620_module
{
    int fd;
};

/**
 * @brief write one word to the device
 * @param data the data going to send out.
 */
static void s_lcd1620_write_word(lcd1620_module_st *lcd1620, int data);

/**
 * @brief send data to the device
 * @param data the data going to send out.
 * @param rs indicate it's data(1) or command(0)
 */
static void s_lcd1620_send_data(lcd1620_module_st *lcd1620, int data, int rs);

/* =================
    device function 
   ================= */
/**
 * @brief clear the display
 * @param lcd1620 a valid module
 */
void lcd1620_module_clear(lcd1620_module_st *lcd1620) {
    i2c_write(lcd1620->fd, LCD_CLEARDISPLAY);
    i2c_write(lcd1620->fd, LCD_RETURNHOME);
    s_lcd1620_send_data(lcd1620, LCD_CLEARDISPLAY, 0);
}

/**
 * @brief write strinng to the screen
 * @param lcd1620 a valid module
 * @param x column number
 * @param y line number
 * @param string the string going to output.
 * @param length the length of the output string.
 * @return the length successfully write to the display.
 */
int lcd1620_module_write_string(lcd1620_module_st *lcd1620, 
                                int x, int y, char *str, int length) {
    int addr, i = 0;
    x = x & 15;
    y = y & 1;

    addr = LCD_SETDDRAMADDR + LCD_SETCGRAMADDR * y + x;
    s_lcd1620_send_data(lcd1620, addr, 0);

    for (i = 0; i < length && i <= 15; ++i) {
        s_lcd1620_send_data(lcd1620, (int)str[i], Rs);
    }
    return i;
}

/* =============================================
    device module initialize and finish function 
   ============================================= */
/**
 * @brief initializing the module
 * @return a valid module
 */
lcd1620_module_st *lcd1620_module_init() {
    int fd;

    if ((fd = wiringPiI2CSetup(LCD_ADDRESS)) < 0)
        exit(errno);

    lcd1620_module_st *lcd1620 = (lcd1620_module_st *)malloc(sizeof(lcd1620_module_st));
    if (lcd1620 == NULL)
        exit(ENOMEM);

    lcd1620->fd = fd;

    s_lcd1620_send_data(lcd1620, 0x33, 0);
    s_lcd1620_send_data(lcd1620, 0x32, 0);
    s_lcd1620_send_data(lcd1620, LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE, 0);
    s_lcd1620_send_data(lcd1620, LCD_DISPLAYCONTROL | LCD_DISPLAYON, 0);
    s_lcd1620_send_data(lcd1620, LCD_CLEARDISPLAY, 0);
    //i2c_write(fd, 0x08);

    delay(200);

    return lcd1620;
}

/**
 * @brief clean up the module
 * @param lcd1620 a valid module
 */
void lcd1620_module_fini(lcd1620_module_st *lcd1620) {
    if (lcd1620 != NULL) {
        close(lcd1620->fd);
        free(lcd1620);
    }
}

static void s_lcd1620_write_word(lcd1620_module_st *lcd1620, int data) {
    data |= LCD_BACKLIGHT;
    i2c_write(lcd1620->fd, data);
}

static void s_lcd1620_send_data(lcd1620_module_st *lcd1620, int data, int rs) {
    int buf;

    buf = data & 0xF0;
    buf |= En | rs;
    s_lcd1620_write_word(lcd1620, buf);
    delay(2);
    buf &= (~En);
    s_lcd1620_write_word(lcd1620, buf);

    buf = (data & 0x0F) << 4;
    buf |= En | rs;
    s_lcd1620_write_word(lcd1620, buf);
    delay(2);
    buf &= (~En);
    s_lcd1620_write_word(lcd1620, buf);
}

#ifdef XTEST

int main() {
    lcd1620_module_st *lcd1620 = lcd1620_module_init();
    lcd1620_module_write_string(lcd1620, 0, 0, "Hello:", strlen("Hello:"));
    lcd1620_module_write_string(lcd1620, 3, 1, "World!", strlen("World!"));
    lcd1620_module_fini(lcd1620);
    return 0;
}

#endif