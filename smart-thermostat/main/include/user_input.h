#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hvac_states.h"
#include "driver/gpio.h"
#include "thermo_logic.h"

#define TEMP_UP GPIO_NUM_26
#define TEMP_DOWN GPIO_NUM_25
#define POWER GPIO_NUM_33
#define FAN_MODE GPIO_NUM_27
#define INC_TEMP 0.5
#define DEC_TEMP -0.5
typedef struct Settings {
	float setpoint;
	int fan_mode;
	int power_mode;
} Settings;

extern hvac_cmd_t thermostat_command;

extern void user_input_init(void);
extern QueueHandle_t input_queue;
void user_input_task(void *pvParameters);

#endif