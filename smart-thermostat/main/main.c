#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "thermo_sensors.h"
#include "thermo_logic.h"
#include "thermo_comms.h"
#include "user_input.h"


void app_main(void)
{
	printf(
		"\n"
		"============================================\n"
		"       THERMOSTAT SYSTEM STARTING          \n"
		"============================================\n"
	);


	thermo_sensors_init();
	thermo_comms_init();
	thermo_logic_init();
	user_input_init();

	printf("Thermostat app_main initialization complete\n");


	while (1) {
		vTaskDelay(
			pdMS_TO_TICKS(1000)
		);
	}
}