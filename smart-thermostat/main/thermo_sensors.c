#include "thermo_sensors.h"

#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "THERMO_SENSORS";

static adc_oneshot_unit_handle_t adc_handle = NULL;

static SensorData temp;


/* ============================================================
 * INITIALIZE TEMPERATURE SENSORS
 * ============================================================ */

void thermo_sensors_init(void)
{
	/* --------------------------------------------------------
	 * Initialize ADC Unit 1
	 * -------------------------------------------------------- */

	adc_oneshot_unit_init_cfg_t adc_unit_config = {
		.unit_id = ADC_UNIT_1
	};

	ESP_ERROR_CHECK(
		adc_oneshot_new_unit(
			&adc_unit_config,
			&adc_handle
		)
	);


	/* --------------------------------------------------------
	 * ADC configuration shared by both sensors
	 * -------------------------------------------------------- */

	adc_oneshot_chan_cfg_t adc_channel_config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = ADC_ATTEN_DB_12
	};


	/* --------------------------------------------------------
	 * Indoor temperature sensor
	 *
	 * GPIO36 = ADC1_CHANNEL_0
	 * -------------------------------------------------------- */

	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(
			adc_handle,
			INDOOR_TEMP_CHANNEL,
			&adc_channel_config
		)
	);


	/* --------------------------------------------------------
	 * Outdoor temperature sensor
	 *
	 * GPIO39 = ADC1_CHANNEL_3
	 * -------------------------------------------------------- */

	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(
			adc_handle,
			OUTDOOR_TEMP_CHANNEL,
			&adc_channel_config
		)
	);


	/* --------------------------------------------------------
	 * Initialize stored readings
	 * -------------------------------------------------------- */

	temp.indoor_temp = 0;
	temp.outdoor_temp = 0;


	printf("Temperature sensors initialized\n");
	printf("Indoor sensor: GPIO36\n");
	printf("Outdoor sensor: GPIO39\n");


	/* --------------------------------------------------------
	 * Create temperature sensor task
	 * -------------------------------------------------------- */

	BaseType_t result = xTaskCreate(
		temperature_sensor_task,
		"temperature_sensor",
		3072,
		NULL,
		2,
		NULL
	);


	if (result != pdPASS) {

		ESP_LOGE(
			TAG,
			"Failed to create temperature sensor task"
		);

		return;
	}


	printf("Temperature sensor task started\n");
}


/* ============================================================
 * TEMPERATURE SENSOR TASK
 * ============================================================ */

void temperature_sensor_task(void *pvParameters)
{
	(void)pvParameters;

	while (1) {

		int indoor_reading = 0;
		int outdoor_reading = 0;


		/* ----------------------------------------------------
		 * Read indoor temperature sensor
		 * ---------------------------------------------------- */

		esp_err_t indoor_result =
			adc_oneshot_read(
				adc_handle,
				INDOOR_TEMP_CHANNEL,
				&indoor_reading
			);


		/* ----------------------------------------------------
		 * Read outdoor temperature sensor
		 * ---------------------------------------------------- */

		esp_err_t outdoor_result =
			adc_oneshot_read(
				adc_handle,
				OUTDOOR_TEMP_CHANNEL,
				&outdoor_reading
			);


		/* ----------------------------------------------------
		 * Check readings
		 * ---------------------------------------------------- */

		if (indoor_result == ESP_OK &&
		    outdoor_result == ESP_OK) {

			temp.indoor_temp = indoor_reading;
			temp.outdoor_temp = outdoor_reading;


			printf(
				"Indoor ADC: %d | Outdoor ADC: %d\n",
				temp.indoor_temp,
				temp.outdoor_temp
			);

		} else {

			printf(
				"Failed to read temperature sensors\n"
			);
		}


		vTaskDelay(
			pdMS_TO_TICKS(SENSOR_PERIOD_MS)
		);
	}
}


/* ============================================================
 * GET INDOOR ADC
 * ============================================================ */

int get_indoor_adc(void)
{
	return temp.indoor_temp;
}


/* ============================================================
 * GET OUTDOOR ADC
 * ============================================================ */

int get_outdoor_adc(void)
{
	return temp.outdoor_temp;
}