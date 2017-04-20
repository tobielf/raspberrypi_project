/**
 * @file screen.c
 * @brief screen operation implementation
 * @author Xiangyu Guo
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>
#include <event2/event.h>

#include "screen.h"

#include "i2c/i2c_lcd1620.h"
#include "i2c/i2c_bmp180.h"
#include "pin/pin_dht_11.h"
#include "spi/spi_mcp3208.h"

#define BUTTON_UP       (6)             /**< The pin of up button*/
#define BUTTON_DOWN     (5)             /**< The pin of down button*/
#define BUTTON_LEFT     (26)            /**< The pin of left button*/
#define BUTTON_RIGHT    (4)             /**< The pin of right button*/

#define INTERRUPT_INTERVAL  (200)       /**< Bouncing Interval */

#define TOTAL_PAGES     (4)             /**< Total avaiable pages*/

struct screen_module {
    int index;                              /**< Current page number */
    char info[LCD1620_CHARS_PER_LINE + 1];  /**< First line info */
    char msg[LCD1620_CHARS_PER_LINE + 1];   /**< Second line message */
    lcd1620_module_st *screen_display;      /**< LCD Display object */
};

typedef void (*display_cb)(int);        /**< Call back function on display */

typedef struct screen_page {
    display_cb call_back;               /**< Call back function of this page */
    int data;                           /**< Data for the call back function */
} screen_page_st;

static screen_module_st *instance = NULL;

/**
 * @brief Inner initializing function of the screen display
 * @return a initialized instance
 */
static screen_module_st *s_screen_display_init();

/* ====================
 * Interrupt functions.
 * ==================== */

/**
 * @brief interrupt service handler of button up event
 */
static void isr_button_up(void);

/**
 * @brief interrupt service handler of button up event
 */
static void isr_button_down(void);

/**
 * @brief interrupt service handler of button up event
 */
static void isr_button_left(void);

/**
 * @brief interrupt service handler of button up event
 */
static void isr_button_right(void);

/* ==========================================
 * call back function on specify data display
 * ========================================== */
/**
 * @brief call back function on time display.
 * @param data, the auxiliary data passing to the call back.
 */
static void display_time(int data);

/**
 * @brief call back function on BMP180 display.
 * @param data, the auxiliary data passing to the call back.
 */
static void display_bmp180(int data);

/**
 * @brief call back function on DHT-11 display.
 * @param data, the auxiliary data passing to the call back.
 */
static void display_dht11(int data);

/**
 * @brief call back function on MCP3208 display.
 * @param data, the auxiliary data passing to the call back.
 */
static void display_mcp3208(int data);

/**
 * @brief combined all pages together.
 */
static screen_page_st g_pages[] = { { display_time   , 0 },
                                    { display_bmp180 , 0 },
                                    { display_dht11  , 0 },
                                    { display_mcp3208, 0 } };

/**
 * @brief get one screen display instance
 * @return a initialized screen display instance
 */
screen_module_st *screen_display_get_instance() {
    if (instance == NULL) {
        instance = s_screen_display_init();
    }
    return instance;
}

/**
 * @brief clean up the screen display module.
 */
void screen_display_clean_up() {
    if (instance != NULL) {
        lcd1620_module_fini(instance->screen_display);
        free(instance);
    }
}

void screen_update_display() {
    static int last_index = 0;
    // use the callback func in g_display_menu
    if (instance != NULL) {
        g_pages[instance->index].call_back(g_pages[instance->index].data);

        if (last_index != instance->index ||
                g_pages[instance->index].data != 0) {
            last_index = instance->index;
            lcd1620_module_clear(instance->screen_display);
        }

        g_pages[instance->index].data = 0;

        // pass the screen_display and data to callback func.
        lcd1620_module_write_string(instance->screen_display, 0, 0, 
                                    instance->info, strlen(instance->info));
        lcd1620_module_write_string(instance->screen_display, 0, 1, 
                                instance->msg, strlen(instance->msg));
    }
}

static screen_module_st *s_screen_display_init() {
    instance = (screen_module_st *)malloc(sizeof(screen_module_st));
    if (instance == NULL)
        exit(ENOMEM);

    instance->index = 0;
    memset(instance->info, 0, LCD1620_CHARS_PER_LINE);
    instance->screen_display = lcd1620_module_init();
    if (instance->screen_display == NULL)
        exit(ENOMEM);

    if (wiringPiSetup() < LOW)
        exit(errno);

    if (wiringPiISR(BUTTON_UP, INT_EDGE_FALLING, &isr_button_up) < 0)
        exit(errno);
    
    if (wiringPiISR(BUTTON_DOWN, INT_EDGE_FALLING, &isr_button_down) < 0)
        exit(errno);

    if (wiringPiISR(BUTTON_LEFT, INT_EDGE_FALLING, &isr_button_left) < 0)
        exit(errno);

    if (wiringPiISR(BUTTON_RIGHT, INT_EDGE_FALLING, &isr_button_right) < 0)
        exit(errno);

    return instance;
}

static void isr_button_up(void) {
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();

    // Deal with bouncing issue
    if ((current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) &&
        instance != NULL) {

        g_pages[instance->index].data = 1;
        
        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void isr_button_down(void) {
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();

    // Deal with bouncing issue
    if ((current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) &&
        instance != NULL) {

        g_pages[instance->index].data = -1;
        
        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void isr_button_left(void) {
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();

    // Deal with bouncing issue
    if ((current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) &&
        instance != NULL) {

        instance->index--;
        if (instance->index < 0)
            instance->index = TOTAL_PAGES - 1;
        g_pages[instance->index].data = 0;

        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void isr_button_right(void) {
    static unsigned long s_last_interrupt = 0;
    unsigned long current_interrupt = millis();

    // Deal with bouncing issue
    if ((current_interrupt - s_last_interrupt > INTERRUPT_INTERVAL) &&
        instance != NULL) {

        instance->index++;
        if (instance->index >= TOTAL_PAGES)
            instance->index = 0;
        g_pages[instance->index].data = 0;

        // Update time
        s_last_interrupt = current_interrupt;
    }
}

static void display_bmp180(int data) {
    static const char *info[] = {"Temperature:", "Altitude:", "Pressure:"};
    static const char *surfix[] = {"*C", "m", "Pa"};
    static int item = 0;
    bmp180_data_st value;
    bmp180_module_st *bmp180;

    if (instance == NULL)
        return;

    item += data;

    if (item < 0)
        item = 2;
    if (item > 2)
        item = 0;

    bmp180 = bmp180_module_init(0);
    if (bmp180_read_data(bmp180, &value) == 0) {
        snprintf(instance->info, LCD1620_CHARS_PER_LINE, "%s", info[item]);
        snprintf(instance->msg, LCD1620_CHARS_PER_LINE, "%.2f %s",
                                    *(&(value.temperature) + item),
                                    surfix[item]);
    } else {
        snprintf(instance->info, LCD1620_CHARS_PER_LINE, "No Data");
    }
}

static void display_dht11(int data) {
    static const char *info[] = {"Temperature:", "Humidity:"};
    static const char *surfix[] = {"*C", "%"};
    static int item = 0;

    dht_data_st value;

    if (instance == NULL)
        return;

    item += data;

    if (item < 0)
        item = 1;
    if (item > 1)
        item = 0;

    pin_dht_11_init();
    pin_dht_11_read(&value);
    snprintf(instance->info, LCD1620_CHARS_PER_LINE, "%s", info[item]);
    snprintf(instance->msg, LCD1620_CHARS_PER_LINE, "%.2f %s",
                                *(&(value.temperature) + item),
                                surfix[item]);
}

static void display_mcp3208(int data) {
    static int channel = 0;
    int value;
    mcp3208_module_st *mcp3208;

    if (instance == NULL)
        return;

    channel += data;

    if (channel < MCP3208_CHANNEL_0)
        channel = MCP3208_CHANNEL_7;
    if (channel > MCP3208_CHANNEL_7)
        channel = MCP3208_CHANNEL_0;

    mcp3208 = mcp3208_module_get_instance();
    value = mcp3208_read_data(mcp3208, channel);
    snprintf(instance->info, LCD1620_CHARS_PER_LINE, "Channel: %d", channel);
    snprintf(instance->msg, LCD1620_CHARS_PER_LINE, "Value: %04d", value);
}

static void display_time(int data) {
    time_t current_time = time(NULL);
    struct tm *format_time = NULL;

    if (instance == NULL)
        return;

    format_time = localtime(&current_time);
    snprintf(instance->info, LCD1620_CHARS_PER_LINE, "Time:%02d:%02d:%02d", 
                            format_time->tm_hour,
                            format_time->tm_min,
                            format_time->tm_sec);
    snprintf(instance->msg, LCD1620_CHARS_PER_LINE, "Date:%02d/%02d/%04d",
                            format_time->tm_mon + 1,
                            format_time->tm_mday,
                            format_time->tm_year + 1900);
}

#ifdef YTEST

int main() {
    screen_display_get_instance();
    delay(10000);
    screen_display_clean_up();
    return 0;
}

#endif
