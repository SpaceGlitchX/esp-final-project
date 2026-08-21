#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H
#include "temperature_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

extern TimerHandle_t temp_sample_timer;
extern TempSensor temp_sensor;
extern QueueHandle_t temp_queue;
void sensor_manager_init(void);

#endif