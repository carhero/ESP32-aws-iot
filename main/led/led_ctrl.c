#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"    // For ESP32

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "LED"
#define TAG_RED "RED-LED"
#define TAG_BLUE "BLUE-LED"

// GPIO PIN CONFIG
#define GPIO_OUTPUT_BLUE    18
#define GPIO_OUTPUT_RED     19
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_OUTPUT_BLUE) | (1<<GPIO_OUTPUT_RED))

#define BLINK_STACK    1024*2

typedef union _ST_LED
{
    unsigned char byte;
    struct 
    {
        unsigned char bBlueLED:1;
        unsigned char bRedLED:1;
        unsigned char :6;
    };
} ST_LED;
ST_LED sLED;

TaskHandle_t blue_led_handler = NULL;
TaskHandle_t red_led_handler = NULL;

void blue_led_blink_task(void *arg);
void red_led_blink_task(void *arg);

void led_ctrl_init(void)
{
    ESP_LOGI(TAG, "led_ctrl_init()\n\r");
}

void led_ctrl_open(void)
{
    ESP_LOGI(TAG, "led_ctrl_open()\n\r");    
}

/* BLUE LED */
void led_ctrl_blue_led(unsigned char value)
{
    // Stop blink task if it is enabled
    if(sLED.bBlueLED)
    {
        vTaskDelete(blue_led_handler);
    }
    sLED.bBlueLED = 0;

    ESP_LOGI(TAG_BLUE, "blue led %s\n", value&0x01 ? "On" : "Off");
    gpio_set_level(GPIO_OUTPUT_BLUE, value&0x01);
}

void led_ctrl_blue_blink(unsigned char value)
{
    if(!sLED.bBlueLED)
    {
        xTaskCreate(blue_led_blink_task, "Blue_LED_Task", BLINK_STACK, NULL, 1, &blue_led_handler);
    }
    sLED.bBlueLED = 1;
    ESP_LOGI(TAG_BLUE, "blue led blink start \n");
}

/* RED LED */
void led_ctrl_red_led(unsigned char value)
{
    // Stop blink task if it is enabled
    if(sLED.bRedLED)
    {    
        vTaskDelete(red_led_handler); 
    }
    sLED.bRedLED = 0;
    ESP_LOGI(TAG_RED, "red led %s\n", value&0x01 ? "On" : "Off");
    gpio_set_level(GPIO_OUTPUT_RED, value&0x01);
}

void led_ctrl_red_blink(unsigned char value)
{
    if(!sLED.bRedLED)
    {
        xTaskCreate(red_led_blink_task, "red_led_blink_task", BLINK_STACK, NULL, 1, &red_led_handler);
    }
    sLED.bRedLED = 1;
    ESP_LOGI(TAG_RED, "red led blink %d\n", value);
}

void led_ctrl_gpio_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}


void blue_led_blink_task(void *arg)
{
    // Initialize GPIO port for LED blue and red

    int led_out = 0;
    while(1)
    {
        led_out = led_out ? 0:1;

        if(sLED.bBlueLED)
        {
            gpio_set_level(GPIO_OUTPUT_BLUE, led_out);
        }
        
        vTaskDelay(500 / portTICK_RATE_MS);
    }
}

void red_led_blink_task(void *arg)
{
    // Initialize GPIO port for LED blue and red
        
    int led_out = 0;
    while(1)
    {
        led_out = led_out ? 0:1;

        if(sLED.bRedLED)
        {
            gpio_set_level(GPIO_OUTPUT_RED, led_out);
        }

        vTaskDelay(500 / portTICK_RATE_MS);
    }
    //vTaskDelete(NULL); 
}

