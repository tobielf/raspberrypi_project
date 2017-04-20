/**
 * @file screen.h
 * @brief screen operation declearation
 * @author Xiangyu Guo
 */
#ifndef __SCREEN_H__
#define __SCREEN_H__

typedef struct screen_module screen_module_st;
struct screen_module;

/**
 * @brief get one screen display instance.
 * @return a initialized screen display instance
 */
screen_module_st *screen_display_get_instance();

/**
 * @brief clean up the screen display module.
 */
void screen_display_clean_up();

/**
 * @brief updating the display.
 */
void screen_update_display();

#endif