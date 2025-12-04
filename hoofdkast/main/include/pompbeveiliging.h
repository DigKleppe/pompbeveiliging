#include "driver/gpio.h"

const gpio_num_t RED_LED_PIN = GPIO_NUM_4;
const gpio_num_t GREEN_LED_PIN = GPIO_NUM_5;
const gpio_num_t KEY_LED_PIN = GPIO_NUM_6;
const gpio_num_t BLUE_LED_PIN = GPIO_NUM_8;
const gpio_num_t LAMP_PIN = GPIO_NUM_8;
const gpio_num_t BUZZER_PIN = GPIO_NUM_10;
const gpio_num_t KEY_PIN = GPIO_NUM_20;

#define PUMPONLEVEL 2.0		// Amps  above this current, the pump is on
#define ERRORLEVEL_5V 4.0	// if this voltage is low, we don't have mains volatage
#define BATLOWLEVEL 16.0  

typedef enum {
	NORMAL,
	NOMAINS,
    BATTERYLOW,
	PUMPWASACTIVE,
	PUMPWASACTIVEACCEPTED,
	LEVEL_ALARM, //levelsensor
	LEVELSENSOR_TIMEOUT,
	TEST
} state_t;

typedef enum { LAMP_OFF, LAMP_ON, LAMP_FLASH_FAST, LAMP_FLASH_SLOW, LAMP_BLINK } lampState_t;
