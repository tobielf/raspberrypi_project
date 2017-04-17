/**
 * @file screen.h
 * @brief screen operation declearation
 * @author Xiangyu Guo
 */
#ifndef __SCREEN_H__
#define __SCREEN_H__

typedef struct screen_module screen_module_st;
struct screen_module;

screen_module_st *screen_display_get_instance();
void screen_update_display();

#endif