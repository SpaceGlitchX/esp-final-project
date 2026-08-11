#ifndef UI_H
#define UI_H

#include "thermo_sensors.h"
#include "LCD.h"

#define SET_UP GPIO_NUM_4
#define SET_DWN GPIO_NUM_0
#define SEL_UP GPIO_NUM_2
#define SEL_DWN GPIO_NUM_15

typedef struct {
    void* setting;
    void* data;
    char* top_text;
    char* bottom_text;
} view;

extern int indoor_temperature_raw;
extern int outdoor_temperature_raw;
int user_setpoint;

uint64_t input_buttons;
extern void ui_init(void);


#endif