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

lampState_t lampState = LAMP_OFF;
bool buzzerOn = false;
state_t state = NORMAL;

void testTask(void *pvParameter) {
	while (true) {
		ESP_LOGI(TAG, "AN: %2.2f %2.2f %2.2f", ADCValue[0], ADCValue[1], ADCValue[2]); //
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void testHardwareTask(void *pvParameter) {
	bool stop = false;
	ESP_LOGI(TAG, "HardwareTest");
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

		//	gpio_set_level(BUZZER_PIN, 1);
		//	vTaskDelay(1000 / portTICK_PERIOD_MS);
		//	gpio_set_level(BUZZER_PIN, 0);
	}
	vTaskDelete(NULL);
}

void lampTask(void *pvParameter) {
	ESP_LOGI(TAG, "LampTask");
	while (true) {
		switch (lampState) {
		case LAMP_OFF:
			gpio_set_level(LAMP_PIN, 0);
			break;
		case LAMP_ON:
			gpio_set_level(LAMP_PIN, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			break;
		case LAMP_FLASH_FAST:
			gpio_set_level(LAMP_PIN, 1);
			vTaskDelay(10 / portTICK_PERIOD_MS);
			gpio_set_level(LAMP_PIN, 0);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			break;
		case LAMP_FLASH_SLOW:
			gpio_set_level(LAMP_PIN, 1);
			vTaskDelay(10 / portTICK_PERIOD_MS);
			gpio_set_level(LAMP_PIN, 0);
			vTaskDelay(10000 / portTICK_PERIOD_MS);
			break;
		case LAMP_BLINK:
			gpio_set_level(LAMP_PIN, 1);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			gpio_set_level(LAMP_PIN, 0);
			vTaskDelay(500 / portTICK_PERIOD_MS);
			break;

		default:
			break;
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void buzzerTask(void *pvParameter) {
	ESP_LOGI("TASK", "BuzzerTask");
	while (true) {
		if (buzzerOn) {
			gpio_set_level(BUZZER_PIN, 1);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
			gpio_set_level(BUZZER_PIN, 0);
			vTaskDelay(5000 / portTICK_PERIOD_MS);
		} else {
			gpio_set_level(BUZZER_PIN, 0);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

void mainTask(void *pvParameter) {

	ESP_LOGI("TASK", "mainTask");
	bool mainsPresent = false;
	bool batteryLow = false;

	while (true) {
		if (ADCValue[ADC_5V] < ERRORLEVEL_5V) {
			ESP_LOGI(TAG, "No mains voltage %2.1f", ADCValue[ADC_5V]);
			mainsPresent = false;
			if (state == NORMAL)
				state = NOMAINS;
		} else {
			mainsPresent = true;
			if (state == NOMAINS)
				state = NORMAL;
		}
		if (ADCValue[ADC_BATT] < BATLOWLEVEL) {
			ESP_LOGI(TAG, "Low bat %2.1f", ADCValue[ADC_BATT]);
			batteryLow = true;
		} else
			batteryLow = false;

		if (ADCValue[ADC_CURRENT] > PUMPONLEVEL) {
			ESP_LOGI(TAG, "Pump on %2.2f", ADCValue[ADC_CURRENT]);
			if (state != PUMPWASACTIVEACCEPTED)
				state = PUMPWASACTIVE;
		}

		if (mainsPresent)
			gpio_set_level(GREEN_LED_PIN, 1);
		else
			gpio_set_level(GREEN_LED_PIN, 0);

		if (batteryLow) {
			if (state < PUMPWASACTIVE) {
				gpio_set_level(RED_LED_PIN, 1);
				vTaskDelay(10 / portTICK_PERIOD_MS);
				gpio_set_level(RED_LED_PIN, 0);
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}
		}
		switch (state) {
		case NORMAL:
			lampState = LAMP_FLASH_SLOW;
			gpio_set_level(GREEN_LED_PIN, 1);
			gpio_set_level(RED_LED_PIN, 0);
			gpio_set_level(KEY_LED_PIN, 0);

			buzzerOn = false;
			break;
		case NOMAINS:
			gpio_set_level(GREEN_LED_PIN, 0);
			gpio_set_level(RED_LED_PIN, 1);

			lampState = LAMP_FLASH_FAST;
			buzzerOn = false;
			break;
		case BATTERYLOW:

			gpio_set_level(RED_LED_PIN, 1);
			vTaskDelay(10 / portTICK_PERIOD_MS);
			gpio_set_level(RED_LED_PIN, 0);
			vTaskDelay(1000 / portTICK_PERIOD_MS);

			lampState = LAMP_OFF;
			buzzerOn = false;
			break;
		case PUMPWASACTIVE:
			gpio_set_level(RED_LED_PIN, 1);
			lampState = LAMP_BLINK;
			gpio_set_level(KEY_LED_PIN, 1);
			buzzerOn = true;
			if (key(KEY)) {
				buzzerOn = false;
				state = PUMPWASACTIVEACCEPTED;
			}
			break;

		case PUMPWASACTIVEACCEPTED:
			gpio_set_level(RED_LED_PIN, 1);
			gpio_set_level(KEY_LED_PIN, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			gpio_set_level(KEY_LED_PIN, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);

			lampState = LAMP_BLINK;
			if (key(KEY)) {
				state = NORMAL;
			}
			break;

		default:
			break;
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
extern "C" {
void app_main(void) {
	TimerHandle_t keyTimer;

	xTaskCreate(&ADCTask, "adcTask", 1024 * 8, NULL, 0, NULL);

	gpio_reset_pin(GPIO_NUM_1);
	gpio_reset_pin(GPIO_NUM_2); // disable jtag pins
	gpio_reset_pin(GPIO_NUM_3);
	gpio_reset_pin(GPIO_NUM_4);
	gpio_reset_pin(GREEN_LED_PIN);
	gpio_reset_pin(RED_LED_PIN);
	gpio_reset_pin(BUZZER_PIN);
	gpio_reset_pin(KEY_PIN);
	gpio_reset_pin(KEY_LED_PIN);

	gpio_set_drive_capability(GREEN_LED_PIN, GPIO_DRIVE_CAP_0);
	gpio_set_drive_capability(RED_LED_PIN, GPIO_DRIVE_CAP_0);
	gpio_set_drive_capability(LAMP_PIN, GPIO_DRIVE_CAP_0); // weak drives for direct drive transistors
	gpio_set_drive_capability(BUZZER_PIN, GPIO_DRIVE_CAP_0);

	gpio_set_direction(GREEN_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(GREEN_LED_PIN, 1);
	gpio_set_direction(RED_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(RED_LED_PIN, 1);
	gpio_set_direction(KEY_LED_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(KEY_LED_PIN, 1);
	gpio_set_direction(LAMP_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LAMP_PIN, 1);
	gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT); // same pin as blue led
	gpio_set_level(BUZZER_PIN, 1);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	gpio_set_level(BUZZER_PIN, 0);
	gpio_set_level(GREEN_LED_PIN, 0);
	gpio_set_level(RED_LED_PIN, 0);
	gpio_set_level(LAMP_PIN, 0);
	gpio_set_level(KEY_LED_PIN, 0);

	gpio_set_direction(KEY_PIN, GPIO_MODE_INPUT);
	keyTimer = xTimerCreate("keysTimer", 10, pdTRUE, NULL, keysTimerHandler);
	xTimerStart(keyTimer, 0);

	//	xTaskCreate(&testTask, "testTask", 1024 * 4, NULL, 0, NULL);
	//	xTaskCreate(&testHardwareTask, "testHWTask", 1024 * 4, NULL, 0, NULL);
	vTaskDelay(1000 / portTICK_PERIOD_MS); // let the ADC task start

	xTaskCreate(&lampTask, "lampTask", 1024 * 2, NULL, 0, NULL);
	xTaskCreate(&buzzerTask, "buzzerTask", 1024 * 2, NULL, 0, NULL);
	xTaskCreate(&mainTask, "mainTask", 1024 * 2, NULL, 0, NULL);

	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "state: %d", (int)state);
	}
}
}
