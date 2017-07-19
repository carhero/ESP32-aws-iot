#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"    // For ESP32

#define TAG "LED"

void led_ctrl_init(void)
{
    ESP_LOGI(TAG, "led_ctrl_init()\n\r");    
}

void led_ctrl_open(void)
{
    ESP_LOGI(TAG, "led_ctrl_open()\n\r");    
}