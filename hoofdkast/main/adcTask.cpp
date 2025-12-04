/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "averager.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

#define ADC_ATTEN ADC_ATTEN_DB_12 // only 1 for this chip!
#define EXAMPLE_ADC_UNIT ADC_UNIT_1

#define EXAMPLE_ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_BIT_WIDTH SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define EXAMPLE_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define EXAMPLE_ADC_GET_CHANNEL(p_data) ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data) ((p_data)->type1.data)
#else
#define EXAMPLE_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define EXAMPLE_ADC_GET_CHANNEL(p_data) ((p_data)->type2.channel)
#define EXAMPLE_ADC_GET_DATA(p_data) ((p_data)->type2.data)
#endif

#define EXAMPLE_READ_LEN 128

// static const float CALFACTOR[] = {8.0/140.0, 5.18 / 3625.0, 17.0 / 2125.0}; 
static const float CALFACTOR[] = {8.0/140.0, 5.3/3734.0, 17.6/2178};

static adc_channel_t channel[] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_3};
static Averager ADC0averager(512);
static Averager ADC1averager(EXAMPLE_READ_LEN);
static Averager ADC2averager(EXAMPLE_READ_LEN);

Averager *pADCaverager[] = {&ADC0averager, &ADC1averager, &ADC2averager};

static TaskHandle_t s_task_handle;
static const char *TAG = "ADCTask";
float ADCValue[3];
uint32_t ADCsamples;

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) {
	BaseType_t mustYield = pdFALSE;
	// Notify that ADC continuous driver has done enough number of conversions
	vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

	return (mustYield == pdTRUE);
}
static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle) {
	adc_continuous_handle_t handle = NULL;
	esp_err_t err = ESP_OK;

	adc_continuous_handle_cfg_t adc_config = {
		.max_store_buf_size = 1024,
		.conv_frame_size = EXAMPLE_READ_LEN,
		.flags = 0,
	};
	err = adc_continuous_new_handle(&adc_config, &handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Create ADC continuous handle failed: %s", esp_err_to_name(err));
		return;
	}

	adc_continuous_config_t dig_cfg = {
		.sample_freq_hz = 1000,
		.conv_mode = EXAMPLE_ADC_CONV_MODE,
		.format = EXAMPLE_ADC_OUTPUT_TYPE,
	};

	adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
	dig_cfg.pattern_num = channel_num;
	for (int i = 0; i < channel_num; i++) {
		adc_pattern[i].atten = ADC_ATTEN;
		adc_pattern[i].channel = channel[i] & 0x7;
		adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
		adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

		// ESP_LOGI(TAG, "adc_pattern[%d].atten is :%" PRIx8, i, adc_pattern[i].atten);
		// ESP_LOGI(TAG, "adc_pattern[%d].channel is :%" PRIx8, i, adc_pattern[i].channel);
		// ESP_LOGI(TAG, "adc_pattern[%d].unit is :%" PRIx8, i, adc_pattern[i].unit);
	}
	dig_cfg.adc_pattern = adc_pattern;
	err = adc_continuous_config(handle, &dig_cfg);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ADC continuous config failed: %s", esp_err_to_name(err));

		return;
	}

	*out_handle = handle;
}
void ADCTask(void *pvParameter) {
	esp_err_t err;
	uint32_t ret_num = 0;
	uint8_t result[EXAMPLE_READ_LEN] = {0};
	memset(result, 0xcc, EXAMPLE_READ_LEN);

	s_task_handle = xTaskGetCurrentTaskHandle();

	adc_continuous_handle_t handle = NULL;
	continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

	adc_continuous_evt_cbs_t cbs = {
		.on_conv_done = s_conv_done_cb,
	};
	err = adc_continuous_register_event_callbacks(handle, &cbs, NULL);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Register ADC continuous event callbacks failed: %s", esp_err_to_name(err));
		vTaskDelete(NULL);
	}
	err = adc_continuous_start(handle);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ADC continuous start failed: %s", esp_err_to_name(err));
		vTaskDelete(NULL);
	}

	while (1) {

		/**
		 * This is to show you the way to use the ADC continuous mode driver event callback.
		 * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
		 * However in this example, the data processing (print) is slow, so you barely block here.
		 *
		 * Without using this event callback (to notify this task), you can still just call
		 * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
		 */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		while (1) {
			err = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
			if (err == ESP_OK) {
				//	ESP_LOGI(TAG, "ret is %x, ret_num is %" PRIu32 " bytes", err, ret_num);
				for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
					adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
					uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
					uint32_t data = EXAMPLE_ADC_GET_DATA(p);
					for (int idx = 0; idx < sizeof(channel) / sizeof(adc_channel_t); idx++) {
						if (channel[idx] == chan_num) { // find index
							Averager *ADCaverager = pADCaverager[idx];
							ADCaverager->write(data);
							ADCValue[idx] = CALFACTOR[idx] * ADCaverager->average();
							//	ESP_LOGI(TAG, "Channel: %d, Value: %2.1f data: %d", idx, ADCValue[idx],(int)data);
							ADCsamples++;
							break;
						}
					}
				}
			}
			vTaskDelay(1 / portTICK_PERIOD_MS);
		}
	}
	ESP_ERROR_CHECK(adc_continuous_stop(handle));
	ESP_ERROR_CHECK(adc_continuous_deinit(handle));
	ESP_LOGI("TASK", "ADC continuous mode driver is stopped");
	vTaskDelete(NULL);
}
