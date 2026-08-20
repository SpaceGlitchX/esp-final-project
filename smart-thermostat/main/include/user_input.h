#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hvac_states.h"
#include "driver/gpio.h"

#define TEMP_UP GPIO_NUM_26
#define TEMP_DOWN GPIO_NUM_25
#define POWER GPIO_NUM_33
#define FAN_MODE GPIO_NUM_27

extern hvac_cmd_t thermostat_command;

void user_input_init(void);

void user_input_monitor(void *pvParameters);

#endif