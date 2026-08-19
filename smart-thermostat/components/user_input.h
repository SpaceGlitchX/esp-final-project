#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <stdio.h>
#include "thermo_comms.h"
#include "thermo_logic.h"
#define FAN_MODE GPIO_NUM_4
#define POWER GPIO_NUM_0
#define TEMP_UP GPIO_NUM_2
#define TEMP_DOWN GPIO_NUM_15

static int user_setpoint;
int fan_mode;
uint64_t input_buttons;
extern void user_input_init(void);

void get_ui_update(TimerHandle_t xTimer);

#endif