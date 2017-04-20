/**
 * @file i2c_lcd1620.h
 * @brief Public function declaration and communication data.
 * @author Xiangyu Guo
 */
#ifndef __I2C_LCD1620_H__
#define __I2C_LCD1620_H__

#define LCD1620_CHARS_PER_LINE  (16)            /**< Maximum Chars per line */

/**
 * @brief module structure, hiding the detail to the public
 */
struct lcd1620_module;
typedef struct lcd1620_module lcd1620_module_st;

/* =============================================
    device module initialize and finish function 
   ============================================= */
/**
 * @brief initializing the module
 * @return a valid module
 */
lcd1620_module_st *lcd1620_module_init();

/**
 * @brief clean up the module
 * @param lcd1620 a valid module
 */
void lcd1620_module_fini(lcd1620_module_st *lcd1620);

/* =================
    device function 
   ================= */
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
						int x, int y, char *string, int length);

/**
 * @brief clear the display
 * @param lcd1620 a valid module
 */
void lcd1620_module_clear(lcd1620_module_st *lcd1620);

#endif