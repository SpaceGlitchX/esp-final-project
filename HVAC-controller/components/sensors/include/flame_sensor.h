#ifndef FLAME_SENSOR_H
#define FLAME_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

/* ============================================================
 * ADC Configuration
 * ============================================================ */

#define ADC_UNIT  ADC_UNIT_1
#define ADC_CHAN  ADC_CHANNEL_6       /* GPIO34 */
#define ADC_ATTEN ADC_ATTEN_DB_12

/* ============================================================
 * Flame Thresholds
 * ============================================================ */

/*
 * ADC range:
 * 0 - 4095
 *
 * Adjust these values based on your measured flame response.
 */

#define FLAME_DETECTED_THRESHOLD  1000
#define FLAME_LOST_THRESHOLD       200

/* Maximum time allowed to prove flame */
#define FLAME_PROVING_TIMEOUT_MS  5000

/* ============================================================
 * Latest Sensor Reading
 * ============================================================ */

extern float g_latest_flame;

/* ============================================================
 * Public API
 * ============================================================ */

esp_err_t flame_sensor_init(void);

float get_flame_sensor_adc(void);

#endif /* FLAME_SENSOR_H */