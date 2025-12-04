/*
 * sensorTask.cpp
 *
 *  Created on: june 28 2025
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "freertos/task.h"
#include "sensorTask.h"
#include "udpServer.h"

//#define LOG_LOCAL_LEVEL ESP_LOG_ERROR
#include "esp_log.h"

#include <cstring>

sensorMssg_t sensorMssg;

static const char *TAG = "sensorTask";

// creates UDP task for receiving sensordata
// parses the messages

char buffer[ 1024];

void sensorTask(void *pvParameters) {
	udpMssg_t udpMssg;
	int mssgCntr = 0;
	int sensorTimeoutTimer = SENSOR_TIMEOUT * 10;

	const udpTaskParams_t udpTaskParams = {.port = UDPSENSORPORT, .maxLen = MAXLEN};

	xTaskCreate(&udpServerTask, "udpServerTask", configMINIMAL_STACK_SIZE * 5, (void *)&udpTaskParams, 5, NULL);
	vTaskDelay(100);

	while (1) {
		if (xQueueReceive(udpMssgBox, &udpMssg, 0)) {
			if (udpMssg.mssg) {
				if (sscanf(udpMssg.mssg, "S,%f,%f,%d", &sensorMssg.voltageL, &sensorMssg.voltageH, &sensorMssg.state) == 3) {
					sensorMssg.mssgCntr = mssgCntr++;
					sensorTimeoutTimer = SENSOR_TIMEOUT *10;
					ESP_LOGI(TAG, "rec: %s %d", udpMssg.mssg, mssgCntr );
				}
				free(udpMssg.mssg);
			}
		}

		if (sensorTimeoutTimer > 0) {
			sensorTimeoutTimer--;
		} else {
			sensorMssg.voltageL = 999;
			sensorMssg.voltageH = 999;
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		
	}
}