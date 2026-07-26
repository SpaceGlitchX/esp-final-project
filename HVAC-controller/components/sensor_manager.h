#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <stdint.h>

// Hardware pin configuration
#define FLAME_ADC_CHANNEL ADC_CHANNEL_6 // GPIO 34
#define FLAME_ADC_UNIT ADC_UNIT_1
#define FLAME_THRESHOLD 500

#define FAN_TACH_PIN 16

#define TEMP_SENSE_PIN 4

void init_sensors(void);
void start_tach_counting(void);
uint32_t get_tach_count(void);

#endif