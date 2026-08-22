#include "user_input.h"

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "sensor_manager.h"
#include "thermo_comms.h"
#include "thermo_logic.h"

Settings settings = {0};
QueueHandle_t input_queue = NULL;
static int fan_on;
static int system_on;


static void IRAM_ATTR isr_handler(void *arg) {
	int button = (int)arg;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(input_queue, &button, &xHigherPriorityTaskWoken);

	if (xHigherPriorityTaskWoken) {
		portYIELD_FROM_ISR();
	}
}

void set_setpoint(struct Settings* self, float level) {
	if (!self) return;
	self->setpoint = self->setpoint + level;
}

void user_input_task(void *pvParameters)
{
	(void)pvParameters;
	int button_id;
	int fan_on;
	int system_on;

	while (1) {

		if (xQueueReceive(input_queue, &button_id, portMAX_DELAY) == pdTRUE) {
		
			switch (button_id) {

				case TEMP_UP:
					set_setpoint(settings, INC_TEMP);
					break;

				case TEMP_DOWN:
					set_setpoint(settings, DEC_TEMP);
					break;

				case POWER:
					system_on = !system_on;
					self->power_mode = system_on;
					break;

				case FAN_MODE:
					fan_on = !fan_on;
					self->fan_mode = fan_on;
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

void user_input_init(struct Settings* self)
{
	if (!self) return;

	self->setpoint = 20.0;
	self->fan_mode = 0;
	self->power_mode = 0;

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

	xTaskCreate(user_input_task, "button_task", 4096, NULL, 4, NULL);
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