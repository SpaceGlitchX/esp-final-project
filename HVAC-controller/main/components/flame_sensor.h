#ifndef FLAME_SENSOR_H
#define FLAME_SENSOR_H
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define ADC_UNIT    ADC_UNIT_1
#define ADC_CHAN    ADC_CHANNEL_3 // Pin 34
#define ADC_ATTEN   ADC_ATTEN_DB_12

typedef struct FlameSensor {
    uint16_t value;
    adc_oneshot_unit_handle_t adc_handle; 
    
    void (*init)(struct FlameSensor* self);
    void (*read)(struct FlameSensor* self);
} FlameSensor;

// Prototype declarations
void flame_sensor_read(FlameSensor* self);
void flame_sensor_init(FlameSensor* self);

#endif FLAME_SENSOR_H