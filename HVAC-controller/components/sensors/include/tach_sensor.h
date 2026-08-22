#ifndef TACH_SENSOR_H
#define TACH_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

/* ============================================================
 * Tachometer Configuration
 * ============================================================ */

#define TACH_GPIO_PIN        GPIO_NUM_18

#define TACH_PULSES_PER_REV  2

#define PCNT_HIGH_LIMIT      10000
#define PCNT_LOW_LIMIT       -1

#define RPM_SAMPLE_TIME_MS   pdMS_TO_TICKS(100)

/* ============================================================
 * Fan Validation
 * ============================================================ */

/* Minimum RPM required for fan to be considered operational */
#define FAN_MIN_RPM          1200.0f

/* ============================================================
 * Latest RPM
 * ============================================================ */

extern uint32_t g_latest_rpm;

/* ============================================================
 * Public API
 * ============================================================ */

esp_err_t tach_sensor_init(void);

uint32_t get_tach_sensor_rpm(void);

#endif /* TACH_SENSOR_H */