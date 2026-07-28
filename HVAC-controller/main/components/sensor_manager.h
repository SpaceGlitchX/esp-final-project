#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_adc/adc_oneshot.h"
#include <stdint.h>

// Hardware pin configuration
#define ADC_UNIT    ADC_UNIT_1
#define ADC_CHAN    ADC_CHANNEL_3 // Pin 34
#define ADC_ATTEN   ADC_ATTEN_DB_12

#define FAN_TACH_PIN 16

#define TEMP_SENSE_PIN 4

// timer handles
extern TimerHandle_t analog_sample_timer;
extern TimerHandle_t temp_sample_timer;

void init_sensors(void);
void start_flame_proving_monitor(void);
void stop_flame_proving_monitor(void);
void start_tach_counting(void);
uint32_t get_tach_count(void);

#endif