#ifndef THERMO_LOGIC_H
#define THERMO_LOGIC_H

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define CONTROL_LOOP_PERIOD_MS 1000

#define SETPOINT_START 20.0f
#define SETPOINT_STEP 0.5f

#define MIN_SETPOINT 10.0f
#define MAX_SETPOINT 30.0f

#define DEADBAND_C 0.5f

void thermo_logic_init(void);

void set_setpoint(int level);

float get_setpoint(void);

float get_indoor_temperature(void);

float get_outdoor_temperature(void);

bool heating_required(void);

void thermo_control_task(void *pvParameters);

#endif