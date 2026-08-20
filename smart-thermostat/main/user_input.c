#include "user_input.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "sensor_manager.h"
#include "thermo_comms.h"
#include "thermo_logic.h"

static QueueHandle_t thermo_queue = NULL;

static int fan_mode = 0;
static int system_on = 0;
extern hvac_cmd_t thermostat_command;

/* ============================================================
 * BUTTON ISR
 * ============================================================ */

static void IRAM_ATTR isr_handler(void *arg)
{
	int button = (int)arg;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xQueueSendFromISR(
		thermo_queue,
		&button,
		&xHigherPriorityTaskWoken
	);

	if (xHigherPriorityTaskWoken) {
		portYIELD_FROM_ISR();
	}
}


/* ============================================================
 * USER INPUT TASK
 * ============================================================ */

void user_input_monitor(void *pvParameters)
{
	(void)pvParameters;

	int button_id;

	while (1) {

		if (xQueueReceive(
			thermo_queue,
			&button_id,
			portMAX_DELAY
		) == pdTRUE) {

			switch (button_id) {

				/* ------------------------------------------------
				 * TEMPERATURE UP
				 * ------------------------------------------------ */

				case TEMP_UP:

					set_setpoint(1);

					printf(
						"Setpoint increased: %.1f C\n",
						get_setpoint()
					);

					break;


				/* ------------------------------------------------
				 * TEMPERATURE DOWN
				 * ------------------------------------------------ */

				case TEMP_DOWN:

					set_setpoint(0);

					printf(
						"Setpoint decreased: %.1f C\n",
						get_setpoint()
					);

					break;


				/* ------------------------------------------------
				 * POWER BUTTON
				 * ------------------------------------------------ */

				case POWER:

					system_on = !system_on;

					if (system_on) {

						thermostat_command = CMD_HEAT;

						printf(
							"System ON\n"
						);

					} else {

						thermostat_command = CMD_OFF;

						printf(
							"System OFF\n"
						);
					}

					break;


				/* ------------------------------------------------
				 * FAN MODE
				 * ------------------------------------------------ */

				case FAN_MODE:

					fan_mode = !fan_mode;

					if (fan_mode == 0) {

						thermostat_command =
							CMD_FAN_AUTO;

						printf(
							"Fan mode: AUTO\n"
						);

					} else {

						thermostat_command =
							CMD_FAN_ON;

						printf(
							"Fan mode: ON\n"
						);
					}

					break;


				default:

					break;
			}
		}
	}
}


/* ============================================================
 * INITIALIZATION
 * ============================================================ */

void user_input_init(void)
{
	thermo_queue = xQueueCreate(
		10,
		sizeof(int)
	);

	if (thermo_queue == NULL) {

		printf(
			"Failed to create user input queue\n"
		);

		return;
	}


	/* --------------------------------------------------------
	 * Initial values
	 * -------------------------------------------------------- */

	fan_mode = 0;
	system_on = 0;

	thermostat_command = CMD_OFF;


	/* --------------------------------------------------------
	 * Configure buttons
	 * -------------------------------------------------------- */

	uint64_t input_buttons =
		(1ULL << TEMP_UP) |
		(1ULL << TEMP_DOWN) |
		(1ULL << POWER) |
		(1ULL << FAN_MODE);


	gpio_config_t button_config = {
		.intr_type = GPIO_INTR_NEGEDGE,
		.mode = GPIO_MODE_INPUT,
		.pin_bit_mask = input_buttons,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en = GPIO_PULLUP_ENABLE
	};


	ESP_ERROR_CHECK(
		gpio_config(
			&button_config
		)
	);


	/* --------------------------------------------------------
	 * Install ISR service
	 * -------------------------------------------------------- */

	ESP_ERROR_CHECK(
		gpio_install_isr_service(0)
	);


	/* --------------------------------------------------------
	 * Add button interrupts
	 * -------------------------------------------------------- */

	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			TEMP_UP,
			isr_handler,
			(void *)TEMP_UP
		)
	);


	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			TEMP_DOWN,
			isr_handler,
			(void *)TEMP_DOWN
		)
	);


	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			POWER,
			isr_handler,
			(void *)POWER
		)
	);


	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			FAN_MODE,
			isr_handler,
			(void *)FAN_MODE
		)
	);


	/* --------------------------------------------------------
	 * Create user input task
	 * -------------------------------------------------------- */

	BaseType_t result = xTaskCreate(
		user_input_monitor,
		"user_input",
		3072,
		NULL,
		5,
		NULL
	);


	if (result != pdPASS) {

		printf(
			"Failed to create user input task\n"
		);

		return;
	}


	printf(
		"User input initialized\n"
	);
}