#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

/* ============================================================
 * ADC CONFIGURATION
 * ============================================================ */

#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_6       // GPIO34
#define ADC_ATTEN       ADC_ATTEN_DB_12

/* ============================================================
 * THERMISTOR CONFIGURATION
 * ============================================================ */

#define THERMISTOR_NOMINAL   10000.0
#define TEMPERATURE_NOMINAL  25.0
#define BETA_COEFFICIENT     3950.0
#define SERIES_RESISTOR      10000.0
#define VCC_VOLTAGE          3300.0

/* ============================================================
 * TEMPERATURE SENSOR STRUCT
 * ============================================================ */

typedef struct TempSensor TempSensor;

struct TempSensor {

    adc_oneshot_unit_handle_t adc_handle;

    adc_cali_handle_t cali_handle;

    bool has_calibration;

    int raw_reading;

    int voltage_mv;

    float resistance;

    float temp;

    void (*init)(TempSensor *self);

    void (*read)(TempSensor *self);
};

/* Global sensor */

extern TempSensor temp_sensor;

/* Functions */

void temp_sensor_init(TempSensor *self);

void temp_sensor_read(TempSensor *self);

#endif