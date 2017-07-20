
#ifndef __LED_CTRL_H__
#define __LED_CTRL_H__

#include "freertos/FreeRTOS.h"

void led_ctrl_init(void);
void led_ctrl_open(void);

void led_ctrl_blue_led(unsigned char value);
void led_ctrl_blue_blink(unsigned char value);
void led_ctrl_red_led(unsigned char value);
void led_ctrl_red_blink(unsigned char value);

void *led_ctrl_get_task_handler(void);
void led_ctrl_gpio_init(void);

#endif