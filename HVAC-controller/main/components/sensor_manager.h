#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_adc/adc_oneshot.h"
#include "hvac_hardware.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdio.h>
#include "driver/pulse_cnt.h"
#include "flame_sensor.h"
#include "tach_sensor.h"

// timer handles
extern TimerHandle_t analog_sample_timer;
extern TimerHandle_t temp_sample_timer;
extern TimerHandle_t tach_monitor_timer;

extern FlameSensor flame_sense;
extern TachSensor tach_sense;

void sensor_manager_init(void);
void start_flame_proving_monitor(void);
void stop_flame_proving_monitor(void);


#endif