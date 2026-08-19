#include "ui.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "thermo_logic.h"


static QueueHandle_t thermo_queue = NULL;


/* ============================================================
 * BUTTON ISR
 * ============================================================ */

static void IRAM_ATTR isr_handler(void *arg)
{
	int button = (int)arg;

	BaseType_t xHigherPriorityTaskWoken =
		pdFALSE;


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

				case SET_UP:

					set_setpoint(1);

					break;


				case SET_DWN:

					set_setpoint(0);

					break;


				case SEL_UP:

					printf(
						"SEL UP pressed\n"
					);

					break;


				case SEL_DWN:

					printf(
						"SEL DOWN pressed\n"
					);

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
	thermo_queue =
		xQueueCreate(
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
	 * Configure buttons
	 * -------------------------------------------------------- */

	uint64_t input_buttons =
		(1ULL << SET_UP) |
		(1ULL << SET_DWN) |
		(1ULL << SEL_UP) |
		(1ULL << SEL_DWN);


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


	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			SET_UP,
			isr_handler,
			(void *)SET_UP
		)
	);


	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			SET_DWN,
			isr_handler,
			(void *)SET_DWN
		)
	);


	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			SEL_UP,
			isr_handler,
			(void *)SEL_UP
		)
	);


	ESP_ERROR_CHECK(
		gpio_isr_handler_add(
			SEL_DWN,
			isr_handler,
			(void *)SEL_DWN
		)
	);


	/* --------------------------------------------------------
	 * Create input task
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