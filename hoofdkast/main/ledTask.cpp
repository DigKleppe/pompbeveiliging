#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#include "esp_log.h"

#include "ledTask.h"
#include "pompbeveiliging.h"

static const char *TAG = "ledTask";

volatile int redLEDnrFlashes;

void LEDtask(void *pvParameters) {
	int flashes = 0;
	
	gpio_set_level(RED_LED_PIN, 0);

	while (1) {
		if (flashes == 0) {
			flashes = redLEDnrFlashes;
		}
		while (flashes > 0) {
			gpio_set_level(RED_LED_PIN, 1);
			vTaskDelay(pdMS_TO_TICKS(300));
			gpio_set_level(RED_LED_PIN, 0);
			vTaskDelay(pdMS_TO_TICKS(300));
			flashes--;
		}
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}


