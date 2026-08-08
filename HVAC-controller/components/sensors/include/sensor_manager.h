#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_adc/adc_oneshot.h"
#include "hardware_manager.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include <stdio.h>
#include "driver/pulse_cnt.h"
#include "flame_sensor.h"
#include "tach_sensor.h"

// Timer handles (external reference)
extern TimerHandle_t analog_sample_timer;
extern TimerHandle_t temp_sample_timer;
extern TimerHandle_t tach_monitor_timer;

// Sensor object declarations (external reference)
extern FlameSensor flame_sensor;
extern TachSensor tach_sensor;

// API Prototypes
void sensor_manager_init(void);
void start_flame_proving_monitor(void);
void stop_flame_proving_monitor(void);
void start_tach_monitoring(void);
void stop_tach_monitoring(void);


#endif