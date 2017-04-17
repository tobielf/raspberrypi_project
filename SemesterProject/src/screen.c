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

#define BUTTON_UP       (6)
#define BUTTON_DOWN     (5)
#define BUTTON_LEFT     (26)
#define BUTTON_RIGHT    (4)

#define CHARS_PER_LINE  (16)

//static int g_display_menu 
//register callback func and its data.

struct screen_module {
    int index;
    char info[CHARS_PER_LINE];
    lcd1620_module_st *screen_display;
};

typedef void (*display_cb)(void *);

typedef struct screen_page {
    display_cb call_back;
    void *data;
} screen_page_st;

static screen_module_st *instance = NULL;

static screen_module_st *s_screen_display_init();
static void isr_button_up(void);
static void isr_button_down(void);
static void isr_button_left(void);
static void isr_button_right(void);

static void display_temp(void *data);
static void display_humi(void *data);

static screen_page_st pages[] = { { display_temp, NULL },
                                  { display_humi, NULL } };

screen_module_st *screen_display_get_instance() {
    if (instance == NULL) {
        instance = s_screen_display_init();
    }
    return instance;
}

void screen_update_display() {
    // use the callback func in g_display_menu
    if (instance != NULL) {
        // instance->screen_display;
        // pass the screen_display and data to callback func.
    }
}

static screen_module_st *s_screen_display_init() {
    instance = (screen_module_st *)malloc(sizeof(screen_module_st));
    if (instance == NULL)
        exit(ENOMEM);

    instance->index = 0;
    memset(instance->info, 0, CHARS_PER_LINE);
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
    if (instance != NULL) {
    }
}

static void isr_button_down(void) {
    if (instance != NULL) {
    }
}

static void isr_button_left(void) {
    if (instance != NULL) {
        instance->index--;
        if (instance->index < 0)
            instance->index = 1;
        pages[instance->index].call_back(NULL);
    }
}

static void isr_button_right(void) {
    if (instance != NULL) {
        instance->index++;
        if (instance->index > 1)
            instance->index = 0;
        pages[instance->index].call_back(NULL);
    }
}

static void display_temp(void *data) {
    bmp180_data_st temp_data;
    bmp180_module_st *bmp180;

    if (instance == NULL)
        return;
    
    lcd1620_module_clear(instance->screen_display);

    bmp180 = bmp180_module_init(0);
    if (bmp180_read_data(bmp180, &temp_data) == 0) {
        snprintf(instance->info, CHARS_PER_LINE, "Temperature:");
        lcd1620_module_write_string(instance->screen_display, 0, 0, 
                                    instance->info, strlen(instance->info));
        snprintf(instance->info, CHARS_PER_LINE, "%.2f *C", temp_data.temperature);
        lcd1620_module_write_string(instance->screen_display, 3, 1, 
                                    instance->info, strlen(instance->info));
    } else {
        snprintf(instance->info, CHARS_PER_LINE, "No Data");
        lcd1620_module_write_string(instance->screen_display, 0, 0, 
                                    instance->info, strlen(instance->info));
    }
}

static void display_humi(void *data) {
    dht_data_st value;

    if (instance == NULL)
        return;

    lcd1620_module_clear(instance->screen_display);

    pin_dht_11_init();
    pin_dht_11_read(&value);
    snprintf(instance->info, CHARS_PER_LINE, "Humidity:");
    lcd1620_module_write_string(instance->screen_display, 0, 0, 
                                instance->info, strlen(instance->info));
    snprintf(instance->info, CHARS_PER_LINE, "%.2f %%", value.humidity);
    lcd1620_module_write_string(instance->screen_display, 3, 1, 
                                instance->info, strlen(instance->info));
}
