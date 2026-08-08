#ifndef TACH_SENSOR_H
#define TACH_SENSOR_H
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define DEFAULT_SAMPLE_PERIOD_MS 1000.0f // 1 second measurement interval
#define PULSES_PER_REV 2 // Standard 3-wire fans emit 2 pulses per revolution
#define FAN_TACH_PIN 18

typedef struct TachSensor {
    uint32_t fan_rpm;
    pcnt_unit_handle_t pcnt_unit;
    pcnt_channel_handle_t pcnt_chan;
    void (*init)(struct TachSensor* self);
    void (*read)(struct TachSensor* self);

} TachSensor;

void tach_sensor_read(TachSensor* self);
void tach_sensor_init(TachSensor* self);

#endif 