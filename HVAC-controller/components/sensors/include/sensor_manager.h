#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "tach_sensor.h"
#include "flame_sensor.h"
#include "hvac_states.h"

/* Latest sensor values */
extern uint32_t current_rpm;
extern uint16_t current_adc;

/* Sensor manager initialization */
esp_err_t sensor_manager_init(void);

/* Flame monitoring */
void start_flame_check(void);
void stop_flame_check(void);

/* Fan/tach monitoring */
void start_fan_check(void);
void stop_fan_check(void);

/* Sensor values */
uint32_t sensor_get_rpm(void);
uint16_t sensor_get_flame_adc(void);

#endif /* SENSOR_MANAGER_H */