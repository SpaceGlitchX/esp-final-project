#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "thermo_sensors.h"
#include "thermo_logic.h"
#include "thermo_comms.h"
#include "ui.h"


void app_main(void)
{
	printf(
		"\n"
		"============================================\n"
		"       THERMOSTAT SYSTEM STARTING          \n"
		"============================================\n"
	);


	/* --------------------------------------------------------
	 * Initialize temperature sensors
	 * -------------------------------------------------------- */

	thermo_sensors_init();


	/* --------------------------------------------------------
	 * Initialize UART communication
	 * -------------------------------------------------------- */

	thermo_comms_init();


	/* --------------------------------------------------------
	 * Initialize buttons
	 * -------------------------------------------------------- */

	ui_init();


	/* --------------------------------------------------------
	 * Initialize thermostat logic
	 * -------------------------------------------------------- */

	thermo_logic_init();


	printf(
		"\n"
		"Thermostat initialization complete\n"
	);


	while (1) {
		vTaskDelay(
			pdMS_TO_TICKS(1000)
		);
	}
}