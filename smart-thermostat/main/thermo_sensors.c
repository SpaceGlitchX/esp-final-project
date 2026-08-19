#include "thermo_sensors.h"

#include <math.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define THERMISTOR_NOMINAL 10000.0f
#define TEMPERATURE_NOMINAL 25.0f
#define BETA_COEFFICIENT 3950.0f
#define SERIES_RESISTOR 10000.0f
#define VCC_VOLTAGE 3300.0f

#define ADC_SAMPLES 10

static const char *TAG = "THERMO_SENSORS";

static SensorData temp;

static adc_oneshot_unit_handle_t adc_handle = NULL;

static adc_cali_handle_t cali_handle = NULL;

static bool has_calibration = false;


/* ============================================================
 * ADC CALIBRATION
 * ============================================================ */

static bool init_adc_calibration(
	adc_unit_t unit,
	adc_atten_t atten,
	adc_cali_handle_t *out_handle)
{
	adc_cali_handle_t handle = NULL;

	esp_err_t ret = ESP_FAIL;

	bool calibrated = false;

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED

	adc_cali_line_fitting_config_t cali_config = {
		.unit_id = unit,
		.atten = atten,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};

	ret = adc_cali_create_scheme_line_fitting(
		&cali_config,
		&handle
	);

	if (ret == ESP_OK) {
		calibrated = true;
	}

#endif

	*out_handle = handle;

	return calibrated;
}


/* ============================================================
 * ADC TO TEMPERATURE
 * ============================================================ */

static float adc_to_temperature(int raw_adc)
{
	int voltage_mv = 0;

	if (has_calibration) {

		esp_err_t result =
			adc_cali_raw_to_voltage(
				cali_handle,
				raw_adc,
				&voltage_mv
			);

		if (result != ESP_OK) {
			return 0.0f;
		}
	}
	else {

		voltage_mv =
			(raw_adc * 3300) / 4095;
	}


	if (voltage_mv <= 0 ||
		voltage_mv >= VCC_VOLTAGE) {

		return 0.0f;
	}


	/*
	 * Thermistor connected to GND:
	 *
	 * R = R_fixed *
	 *     Vout / (Vcc - Vout)
	 */

	float resistance =
		SERIES_RESISTOR *
		(
			(float)voltage_mv /
			(VCC_VOLTAGE - (float)voltage_mv)
		);


	/*
	 * Beta equation
	 */

	float steinhart =
		resistance /
		THERMISTOR_NOMINAL;

	steinhart =
		logf(steinhart);

	steinhart /=
		BETA_COEFFICIENT;

	steinhart +=
		1.0f /
		(TEMPERATURE_NOMINAL + 273.15f);

	steinhart =
		1.0f /
		steinhart;


	return steinhart - 273.15f;
}


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

void thermo_sensors_init(void)
{
	adc_oneshot_unit_init_cfg_t adc_unit_config = {
		.unit_id = ADC_UNIT_1
	};

	ESP_ERROR_CHECK(
		adc_oneshot_new_unit(
			&adc_unit_config,
			&adc_handle
		)
	);


	adc_oneshot_chan_cfg_t adc_channel_config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC_ATTEN_DB_12
	};


	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(
			adc_handle,
			INDOOR_TEMP_CHANNEL,
			&adc_channel_config
		)
	);


	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(
			adc_handle,
			OUTDOOR_TEMP_CHANNEL,
			&adc_channel_config
		)
	);


	has_calibration =
		init_adc_calibration(
			ADC_UNIT_1,
			ADC_ATTEN_DB_12,
			&cali_handle
		);


	if (has_calibration) {

		ESP_LOGI(
			TAG,
			"ADC calibration enabled"
		);
	}
	else {

		ESP_LOGW(
			TAG,
			"ADC calibration unavailable"
		);
	}


	temp.indoor_temp = 0;
	temp.outdoor_temp = 0;


	BaseType_t result = xTaskCreate(
		temperature_sensor_task,
		"temperature_sensor",
		3072,
		NULL,
		2,
		NULL
	);


	if (result != pdPASS) {

		printf(
			"Failed to create temperature sensor task\n"
		);

		return;
	}


	printf(
		"Temperature sensor task started\n"
	);
}


/* ============================================================
 * SENSOR TASK
 * ============================================================ */

void temperature_sensor_task(void *pvParameters)
{
	(void)pvParameters;

	while (1) {

		int indoor_total = 0;

		int outdoor_total = 0;


		for (int i = 0; i < ADC_SAMPLES; i++) {

			int indoor_raw = 0;

			int outdoor_raw = 0;


			adc_oneshot_read(
				adc_handle,
				INDOOR_TEMP_CHANNEL,
				&indoor_raw
			);


			adc_oneshot_read(
				adc_handle,
				OUTDOOR_TEMP_CHANNEL,
				&outdoor_raw
			);


			indoor_total += indoor_raw;

			outdoor_total += outdoor_raw;


			vTaskDelay(
				pdMS_TO_TICKS(10)
			);
		}


		temp.indoor_temp =
			indoor_total / ADC_SAMPLES;

		temp.outdoor_temp =
			outdoor_total / ADC_SAMPLES;


		float indoor_temperature =
			adc_to_temperature(
				temp.indoor_temp
			);

		float outdoor_temperature =
			adc_to_temperature(
				temp.outdoor_temp
			);


		printf(
			"Sensor: Indoor %.2f C | Outdoor %.2f C\n",
			indoor_temperature,
			outdoor_temperature
		);


		vTaskDelay(
			pdMS_TO_TICKS(
				SENSOR_PERIOD_MS
			)
		);
	}
}


/* ============================================================
 * GET INDOOR TEMPERATURE
 * ============================================================ */

float thermo_sensors_get_indoor_temperature(void)
{
	return adc_to_temperature(
		temp.indoor_temp
	);
}


/* ============================================================
 * GET OUTDOOR TEMPERATURE
 * ============================================================ */

float thermo_sensors_get_outdoor_temperature(void)
{
	return adc_to_temperature(
		temp.outdoor_temp
	);
}


/* ============================================================
 * GET INDOOR ADC
 * ============================================================ */

int thermo_sensors_get_indoor_adc(void)
{
	return temp.indoor_temp;
}


/* ============================================================
 * GET OUTDOOR ADC
 * ============================================================ */

int thermo_sensors_get_outdoor_adc(void)
{
	return temp.outdoor_temp;
}