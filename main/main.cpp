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
	while (true) {
		ESP_LOGI(TAG, "AN: %2.2f %2.2f %2.2f", ADCValue[0], ADCValue[1], ADCValue[2]); //
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void testHardwareTask(void *pvParameter) {
	bool stop = false;
	ESP_LOGI("TASK", "HardwareTest");
	while (!stop) {
		gpio_set_level(LAMP_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(LAMP_PIN, 0);

		gpio_set_level(GREEN_LED_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(GREEN_LED_PIN, 0);

		gpio_set_level(RED_LED_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(RED_LED_PIN, 0);

		gpio_set_level(KEY_LED_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(KEY_LED_PIN, 0);

		gpio_set_level(BUZZER_PIN, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		gpio_set_level(BUZZER_PIN, 0);

		if (key(KEY)) { // stop = true;
			ESP_LOGI("TASK", "Key pressed");
		}
	}
	vTaskDelete(NULL);
}

extern "C" {
void app_main(void) {
	TimerHandle_t keyTimer;
	gpio_reset_pin(GPIO_NUM_1);
	gpio_reset_pin(GPIO_NUM_2); // disable jtag pins
	gpio_reset_pin(GPIO_NUM_3);
	gpio_reset_pin(GPIO_NUM_4);
	gpio_reset_pin(GREEN_LED_PIN);
	gpio_reset_pin(RED_LED_PIN);
	gpio_reset_pin(BLUE_LED_PIN);
	gpio_reset_pin(BUZZER_PIN);
	gpio_reset_pin(KEY_PIN);
	gpio_reset_pin(KEY_LED_PIN);

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

	xTaskCreate(&testTask, "testTask", 1024 * 4, NULL, 0, NULL);
	xTaskCreate(&ADCTask, "adcTask", 1024 * 8, NULL, 0, NULL);
	xTaskCreate(&testHardwareTask, "testHWTask", 1024 * 4, NULL, 0, NULL);

	keyTimer = xTimerCreate("keysTimer", 10, pdTRUE, NULL, keysTimerHandler);
	xTimerStart(keyTimer, 0);
	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
}
