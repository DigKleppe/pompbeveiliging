#include <stdio.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_freertos_hooks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "ADCTask.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "keys.h"
#include "pompbeveiliging.h"

static const char *TAG = "main";

myKey_t getKeyPins(void) { return (myKey_t)gpio_get_level(KEY_PIN); }

void keysTimerHandler(TimerHandle_t xTimer) { keysTimerHandler_ms(10); }

void testTask(void *pvParameter) {
	gpio_set_direction(BLUE_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(BLUE_LED_PIN, 0);
	while (true) {
		gpio_set_level(BLUE_LED_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(BLUE_LED_PIN, 0);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

void testHardwareTask(void *pvParameter) {
	bool stop = false;
	while (!stop) {
		gpio_set_level(LAMP_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(LAMP_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(GREEN_LED_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(GREEN_LED_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(RED_LED_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(RED_LED_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(KEY_LED_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(KEY_LED_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(BUZZER_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(BUZZER_PIN, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		if (key(KEY))
			stop = true;
	}
	vTaskDelete(NULL);
}

extern "C" {
void app_main(void) {
	TimerHandle_t keyTimer;
	gpio_set_direction(BLUE_LED_PIN, GPIO_MODE_OUTPUT); // on board led
	gpio_set_level(BLUE_LED_PIN, 0);
	gpio_set_direction(GREEN_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(GREEN_LED_PIN, 0);
	gpio_set_direction(RED_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(RED_LED_PIN, 0);
	gpio_set_direction(KEY_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(KEY_LED_PIN, 0);
	gpio_set_direction(LAMP_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LAMP_PIN, 0);
	gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT); // same pin as blue led
	gpio_set_level(BUZZER_PIN, 0);

	gpio_set_direction(KEY_PIN, GPIO_MODE_INPUT);

	///   xTaskCreate (&testTask, "testTask", 1024 * 1, NULL, 0, NULL);
	xTaskCreate(&ADCTask, "adcTask", 1024 * 8, NULL, 0, NULL);
	xTaskCreate(&testHardwareTask, "testHWTask", 1024 * 1, NULL, 0, NULL);

	keyTimer = xTimerCreate("keysTimer", 10, pdTRUE, NULL, keysTimerHandler);
	xTimerStart(keyTimer, 0);
	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
}
