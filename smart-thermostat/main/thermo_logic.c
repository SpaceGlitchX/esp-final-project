#include "thermo_logic.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "thermo_sensors.h"
#include "thermo_comms.h"

static SemaphoreHandle_t setpoint_mutex = NULL;

static float setpoint = SETPOINT_START;


/* ============================================================
 * SETPOINT
 * ============================================================ */

void set_setpoint(int level)
{
	if (setpoint_mutex == NULL) {
		return;
	}

	xSemaphoreTake(
		setpoint_mutex,
		portMAX_DELAY
	);

	if (level == 1) {
		setpoint += SETPOINT_STEP;

		if (setpoint > MAX_SETPOINT) {
			setpoint = MAX_SETPOINT;
		}
	}
	else {
		setpoint -= SETPOINT_STEP;

		if (setpoint < MIN_SETPOINT) {
			setpoint = MIN_SETPOINT;
		}
	}

	xSemaphoreGive(setpoint_mutex);

	printf(
		"Setpoint: %.1f C\n",
		setpoint
	);
}


/* ============================================================
 * GET SETPOINT
 * ============================================================ */

float get_setpoint(void)
{
	float current_setpoint;

	if (setpoint_mutex == NULL) {
		return setpoint;
	}

	xSemaphoreTake(
		setpoint_mutex,
		portMAX_DELAY
	);

	current_setpoint = setpoint;

	xSemaphoreGive(setpoint_mutex);

	return current_setpoint;
}


/* ============================================================
 * GET INDOOR TEMPERATURE
 * ============================================================ */

float get_indoor_temperature(void)
{
	return thermo_sensors_get_indoor_temperature();
}


/* ============================================================
 * GET OUTDOOR TEMPERATURE
 * ============================================================ */

float get_outdoor_temperature(void)
{
	return thermo_sensors_get_outdoor_temperature();
}


/* ============================================================
 * HEATING REQUIRED
 * ============================================================ */

bool heating_required(void)
{
	float indoor_temperature = get_indoor_temperature();

	float current_setpoint = get_setpoint();

	if (indoor_temperature <
		(current_setpoint - DEADBAND_C)) {

		return true;
	}

	return false;
}


/* ============================================================
 * THERMOSTAT CONTROL TASK
 * ============================================================ */

void thermo_control_task(void *pvParameters)
{
	(void)pvParameters;

	TickType_t last_wake_time =
		xTaskGetTickCount();

	while (1) {

		float indoor_temperature =
			get_indoor_temperature();

		float outdoor_temperature =
			get_outdoor_temperature();

		float current_setpoint =
			get_setpoint();


		printf(
			"\n"
			"============================================\n"
			"              THERMOSTAT DATA              \n"
			"============================================\n"
		);

		printf(
			"Indoor:   %.1f C\n",
			indoor_temperature
		);

		printf(
			"Outdoor:  %.1f C\n",
			outdoor_temperature
		);

		printf(
			"Setpoint: %.1f C\n",
			current_setpoint
		);


		/* ----------------------------------------------------
		 * Heating decision
		 * ---------------------------------------------------- */

		if (indoor_temperature <
			(current_setpoint - DEADBAND_C)) {

			printf(
				"Heating:  REQUIRED\n"
			);
		}
		else if (indoor_temperature >
				 (current_setpoint + DEADBAND_C)) {

			printf(
				"Heating:  NOT REQUIRED\n"
			);
		}
		else {

			printf(
				"Heating:  DEAD BAND\n"
			);
		}


		/* ----------------------------------------------------
		 * HVAC STATUS RECEIVED THROUGH UART
		 * ---------------------------------------------------- */

		printf(
			"\n"
			"              HVAC UART DATA              \n"
			"--------------------------------------------\n"
		);

		printf(
			"HVAC State:  %u\n",
			current_hvac_state
		);

		printf(
			"HVAC Fault:  %u\n",
			current_hvac_fault
		);

		printf(
			"Fan State:   %u\n",
			current_hvac_fan
		);

		printf(
			"Heater State: %u\n",
			current_hvac_heater
		);

		printf(
			"============================================\n"
		);


		vTaskDelayUntil(
			&last_wake_time,
			pdMS_TO_TICKS(
				CONTROL_LOOP_PERIOD_MS
			)
		);
	}
}


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

void thermo_logic_init(void)
{
	setpoint_mutex =
		xSemaphoreCreateMutex();

	if (setpoint_mutex == NULL) {

		printf(
			"Failed to create setpoint mutex\n"
		);

		return;
	}

	setpoint = SETPOINT_START;

	printf(
		"Thermostat logic initialized\n"
	);

	printf(
		"Initial setpoint: %.1f C\n",
		setpoint
	);


	BaseType_t result = xTaskCreate(
		thermo_control_task,
		"thermo_control",
		3072,
		NULL,
		2,
		NULL
	);

	if (result != pdPASS) {

		printf(
			"Failed to create thermostat control task\n"
		);

		return;
	}

	printf(
		"Thermostat control task started\n"
	);
}