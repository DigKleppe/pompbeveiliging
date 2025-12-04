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
#include "ledTask.h"
#include "nvs_flash.h"
#include "pompbeveiliging.h"
#include "sensorTask.h"
#include "softAP.h"
#include "udp.h"

static const char *TAG = "main";
static const int UDPPORT = 5000; // info

myKey_t getKeyPins(void) { return (myKey_t)gpio_get_level(KEY_PIN); }

void keysTimerHandler(TimerHandle_t xTimer) { keysTimerHandler_ms(10); }

lampState_t lampState = LAMP_OFF;
state_t state = NORMAL;
volatile bool buzzerOn = false;

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
			ESP_LOGI("TASK", "buzzer");
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
	bool buzzerAccepted = false;
	state_t lastState = NORMAL;
	;
	while (true) {
		if (ADCValue[ADC_5V] < ERRORLEVEL_5V) {
			//	ESP_LOGI(TAG, "No mains voltage %2.1f", ADCValue[ADC_5V]);
			mainsPresent = false;
			if (state == NORMAL)
				state = NOMAINS;
		} else {
			mainsPresent = true;
			if (state == NOMAINS)
				state = NORMAL;
		}
		if (ADCValue[ADC_BATT] < BATLOWLEVEL) {
			//	ESP_LOGI(TAG, "Low bat %2.1f", ADCValue[ADC_BATT]);
			batteryLow = true;
		} else
			batteryLow = false;

		if (ADCValue[ADC_CURRENT] > PUMPONLEVEL) {
			//	ESP_LOGI(TAG, "Pump on %2.2f", ADCValue[ADC_CURRENT]);
			if (state != PUMPWASACTIVEACCEPTED)
				state = PUMPWASACTIVE;
		}

		if (mainsPresent)
			gpio_set_level(GREEN_LED_PIN, 1);
		else
			gpio_set_level(GREEN_LED_PIN, 0);

		if (batteryLow) {
			if (state < PUMPWASACTIVE) {
				state = BATTERYLOW;
			}
		} else {
			if (state == BATTERYLOW)
				state = NORMAL;
		}

		if (sensorMssg.state == 1) {
			state = LEVEL_ALARM;
		}

		if (sensorMssg.voltageL == 999)
			state = LEVELSENSOR_TIMEOUT;

		if (state == LEVELSENSOR_TIMEOUT) {
			if (sensorMssg.voltageL != 999)
				state = NORMAL;
		}

		redLEDnrFlashes = state;

		if (state != lastState) {
			lastState = state;
			buzzerAccepted = false;
		}

		switch (state) {
		case NORMAL:
			lampState = LAMP_FLASH_SLOW;
			gpio_set_level(GREEN_LED_PIN, 1);
			gpio_set_level(KEY_LED_PIN, 0);
			if (key(KEY)) {
				state = TEST;
			}
			buzzerOn = false;
			buzzerAccepted = false;
			break;
		case NOMAINS:
			gpio_set_level(GREEN_LED_PIN, 0);
			lampState = LAMP_FLASH_FAST;
			if (!buzzerAccepted)
				buzzerOn = true;
			if (key(KEY)) {
				buzzerOn = false;
				buzzerAccepted = true;
			}
			break;
		case BATTERYLOW:
			lampState = LAMP_OFF;
			buzzerOn = false;
			break;

		case PUMPWASACTIVE:
			lampState = LAMP_BLINK;
			if (!buzzerAccepted)
				buzzerOn = true;
			gpio_set_level(KEY_LED_PIN, 1);
			if (key(KEY)) {
				buzzerOn = false;
				buzzerAccepted = true;
				if (state == PUMPWASACTIVE)
					state = PUMPWASACTIVEACCEPTED;
			}
			break;

		case LEVEL_ALARM:
		case LEVELSENSOR_TIMEOUT:
			lampState = LAMP_BLINK;
			if (!buzzerAccepted) {
				gpio_set_level(KEY_LED_PIN, 1);
				vTaskDelay(100 / portTICK_PERIOD_MS);
				gpio_set_level(KEY_LED_PIN, 0);
				vTaskDelay(100 / portTICK_PERIOD_MS);
				buzzerOn = true;
			}

			if (key(KEY)) {
				buzzerOn = false;
				buzzerAccepted = true;
			}
			if (buzzerAccepted && (sensorMssg.state == 0)) // leave after acknowledge and key pressed
				state = NORMAL;

			break;

		case PUMPWASACTIVEACCEPTED:
			gpio_set_level(KEY_LED_PIN, 1);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			gpio_set_level(KEY_LED_PIN, 0);
			vTaskDelay(100 / portTICK_PERIOD_MS);

			lampState = LAMP_BLINK;
			if (key(KEY)) {
				state = NORMAL;
				buzzerAccepted = false;
			}
			break;

		case TEST:
			gpio_set_level(KEY_LED_PIN, 1);
			gpio_set_level(LAMP_PIN, 1);
			lampState = LAMP_ON;
			buzzerOn = true;
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			buzzerOn = false;
			vTaskDelay(3000 / portTICK_PERIOD_MS);
			state = NORMAL;
			break;

		default:
			break;
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void udpTask(void *pvParameter) {
	ESP_LOGI("TASK", "udpTask");
	char *buf = (char *)malloc(100);
	int cnts = 0;
	if (buf == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for UDP buffer");
		vTaskDelete(NULL);
		return;
	}

	while (true) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		cnts++;
		sprintf(buf, "C:%2.1fA 5V:%2.1fV Bat:%2.1fV LevelL:%2.1fV,LevelH:%2.1fV,LevelS:%d, State:%d hr:%d\n\r", ADCValue[ADC_CURRENT],
				ADCValue[ADC_5V], ADCValue[ADC_BATT], sensorMssg.voltageL, sensorMssg.voltageH, sensorMssg.state, (int)state, cnts / (3600));
		udpSend(buf, UDPPORT);
		ESP_LOGI(TAG, "UDP: %s", buf);
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

	gpio_set_direction(KEY_PIN, GPIO_MODE_INPUT);
	keyTimer = xTimerCreate("keysTimer", 10, pdTRUE, NULL, keysTimerHandler);
	xTimerStart(keyTimer, 0);

	//	xTaskCreate(&testTask, "testTask", 1024 * 4, NULL, 0, NULL);
	//	xTaskCreate(&testHardwareTask, "testHWTask", 1024 * 4, NULL, 0, NULL);
	vTaskDelay(1000 / portTICK_PERIOD_MS); // let the ADC task start

	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
	wifi_init_softap();

	xTaskCreate(&lampTask, "lampTask", 1024 * 2, NULL, 0, NULL);
	xTaskCreate(&buzzerTask, "buzzerTask", 1024 * 2, NULL, 0, NULL);
	xTaskCreate(&udpTask, "udpTask", 1024 * 4, NULL, 0, NULL);

	xTaskCreate(&sensorTask, "sensorTask", configMINIMAL_STACK_SIZE * 5, NULL, 1, NULL);
	xTaskCreate(&mainTask, "mainTask", 1024 * 2, NULL, 0, NULL);
	xTaskCreate(LEDtask, "LEDtask", 3000, NULL, tskIDLE_PRIORITY, NULL);

	while (true) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
}
