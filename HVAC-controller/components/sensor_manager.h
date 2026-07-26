#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <stdint.h>

// Hardware pin configuration
#define FLAME_ADC_CHANNEL ADC_CHANNEL_6 // GPIO 34
#define FLAME_THRESHOLD 500

#define FAN_TACH_PIN 16

#define TEMP_SENSE_PIN 4

// ADC and timer handles
extern adc_oneshot_unit_handle_t adc_handle;
extern TimerHandle_t analog_sample_timer;
extern TimerHandle_t temp_sample_timer;

void init_sensors(void);
void start_flame_proving_monitor(void);
void stop_flame_proving_monitor(void);
void start_tach_counting(void);
uint32_t get_tach_count(void);

#endif