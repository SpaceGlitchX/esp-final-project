#include "ui.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "thermo_logic.h"
#include "thermo_comms.h"


static QueueHandle_t thermo_queue = NULL;

static int fan_mode = 0;


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

static void user_input_task(void *pvParameters)
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

				case TEMP_UP:

					set_setpoint(1);

					printf(
						"Setpoint: %.1f C\n",
						get_setpoint()
					);

					break;


				case TEMP_DOWN:

					set_setpoint(0);

					printf(
						"Setpoint: %.1f C\n",
						get_setpoint()
					);

					break;


				case POWER:

					thermostat_command = CMD_OFF;

					printf(
						"Power: OFF\n"
					);

					break;


				case FAN_MODE:

					fan_mode = !fan_mode;

					if (fan_mode == 0) {

						thermostat_command = CMD_FAN_AUTO;

						printf(
							"Fan mode: AUTO\n"
						);
					}
					else {

						thermostat_command = CMD_FAN_ON;

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

void ui_init(void)
{
	thermo_queue = xQueueCreate(
		10,
		sizeof(int)
	);

	if (thermo_queue == NULL) {

		printf(
			"Failed to create UI queue\n"
		);

		return;
	}


	/* --------------------------------------------------------
	 * Button GPIO configuration
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
	 * Install GPIO ISR service
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
		user_input_task,
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