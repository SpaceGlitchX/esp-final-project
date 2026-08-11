#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "sensor_manager.h"
#include "flame_sensor.h"
#include "tach_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include <stdio.h>

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