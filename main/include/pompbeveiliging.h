#include "driver/gpio.h"

const gpio_num_t RED_LED_PIN = GPIO_NUM_4;
const gpio_num_t GREEN_LED_PIN = GPIO_NUM_5;
const gpio_num_t KEY_LED_PIN = GPIO_NUM_6;
const gpio_num_t BLUE_LED_PIN = GPIO_NUM_8;
const gpio_num_t LAMP_PIN = GPIO_NUM_8;
const gpio_num_t BUZZER_PIN = GPIO_NUM_10;
const gpio_num_t KEY_PIN = GPIO_NUM_20;

#define PUMPONLEVEL 1.0 // todo: set to correct level
#define ERRORLEVEL_5V 4.0 // if this voltage is low, we don't have mains volatage
#define ERRORLEVEL_18V 16.0 // if this voltage is low, we don't have battery voltage




     
