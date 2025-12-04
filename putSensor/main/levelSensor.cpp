#include "averager.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const gpio_num_t TX_PIN = GPIO_NUM_4;

const static char *TAG = "LevelSensor";

#define NRSAMPLES 50
#define ADC_CHAN ADC_CHANNEL_0

#define EXAMPLE_ADC_ATTEN ADC_ATTEN_DB_12

#define ALARMLEVEL 0.25 // V
#define ZEROLEVEL 1.6  //

static int adc_raw[10];
static int voltage[10];
Averager averagerH, averagerL;

float sensVoltageL, sensVoltageH;

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
bool do_calibration1_chan0;
adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_chan0_handle;

void initADC(void) {

	//-------------ADC1 Init---------------//

	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	//-------------ADC1 Config---------------//
	adc_oneshot_chan_cfg_t config = {
		.atten = EXAMPLE_ADC_ATTEN,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHAN, &config));

	//-------------ADC1 Calibration Init---------------//

	do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, ADC_CHAN, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
	averagerL.setAverages(NRSAMPLES);
    averagerH.setAverages(NRSAMPLES);
}

bool levelSens(void) {

	gpio_set_direction(TX_PIN, GPIO_MODE_OUTPUT);

	for (int samples = 0; samples < NRSAMPLES; samples++) {

		gpio_set_level(TX_PIN, 1);
		vTaskDelay(1);
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHAN, &adc_raw[0]));
		//	ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC_CHAN, adc_raw[0]);
		if (do_calibration1_chan0) {
			ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0], &voltage[0]));
		//	ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADC_CHAN, voltage[0]);
		}
		averagerH.write(voltage[0]);

		gpio_set_level(TX_PIN, 0);
		vTaskDelay(1);
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHAN, &adc_raw[0]));
		//	ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADC_CHAN, adc_raw[0]);
		if (do_calibration1_chan0) {
			ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0], &voltage[0]));
			//ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADC_CHAN, voltage[0]);
		}
		averagerL.write(voltage[0]);
	}

	sensVoltageH = averagerH.average() / 1000.0;
	sensVoltageL = averagerL.average() / 1000.0;
  //  ESP_LOGI(TAG, "SensL: %2.1f H:%2.1fV", sensVoltageL, sensVoltageH);


	if ((sensVoltageL < (ZEROLEVEL - ALARMLEVEL)) || (sensVoltageH > (ZEROLEVEL + ALARMLEVEL)))
		return true;

	return false;
}

/*---------------------------------------------------------------
		ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	if (!calibrated) {
		ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
		adc_cali_curve_fitting_config_t cali_config = {
			.unit_id = unit,
			.chan = channel,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	if (!calibrated) {
		ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
		adc_cali_line_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}
#endif

	*out_handle = handle;
	if (ret == ESP_OK) {
		ESP_LOGI(TAG, "Calibration Success");
	} else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
		ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
	} else {
		ESP_LOGE(TAG, "Invalid arg or no memory");
	}

	return calibrated;
}