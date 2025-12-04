/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
// #include "common.h"
// #include "esp_sleep.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "udp.h"
#include "wifiConnect.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const static char *TAG = "main";
static const int UDPPORT = 5001;

bool levelSens(void); // returns true if water is sensed
bool alarm;			  // set when waterlevel too high
void initADC();

extern float sensVoltageL, sensVoltageH;
const int wakeup_time_sec = 10;


void mainTask(void *pvParameter) {
char str[32];
	initADC();
	while (1) {
		alarm = levelSens();
		if (connectStatus == IP_RECEIVED) {
			sprintf(str, "S,%2.2f,%2.2f,%d", sensVoltageL, sensVoltageH, alarm);
			udpSend(str, UDPPORT);
			ESP_LOGI(TAG, "udp %s", str);
		}
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}


extern "C" void app_main(void) {
	/* Local variables */
	int rc = 0;
	esp_err_t ret = ESP_OK;

	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		ESP_LOGI(TAG, "nvs flash erased");
	}
	ESP_ERROR_CHECK(err);

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	// err = init_spiffs();
	// if (err != ESP_OK) {
	// 	ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(err));
	// 	return;
	// }
	xTaskCreate(&mainTask, "mainTask", 1024 * 5, NULL, 0, NULL);
	wifiConnect();

	// printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
	// ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
	// esp_deep_sleep_start();

	return;
}
