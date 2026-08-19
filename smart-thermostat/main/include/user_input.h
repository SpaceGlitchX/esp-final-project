#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#define TEMP_UP GPIO_NUM_4
#define TEMP_DOWN GPIO_NUM_0
#define POWER GPIO_NUM_2
#define FAN_MODE GPIO_NUM_15

void user_input_init(void);

void user_input_monitor(void *pvParameters);

#endif