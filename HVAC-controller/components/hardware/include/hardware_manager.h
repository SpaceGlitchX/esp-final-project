#ifndef HVAC_HARDWARE_H
#define HVAC_HARDWARE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "hvac_states.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

/* ============================================================
 * Hardware Pins
 * ============================================================ */

#define HEATER_PIN GPIO_NUM_19
#define FAN_PIN    GPIO_NUM_25

/* ============================================================
 * Hardware Manager
 * ============================================================ */

esp_err_t hardware_manager_init(void);

/* ============================================================
 * Actuator Control
 * ============================================================ */

void set_heater_state(int level);
void set_fan_state(int level);

/* ============================================================
 * Actuator Status
 * ============================================================ */

int get_fan_state(void);
int get_heater_state(void);

#endif /* HVAC_HARDWARE_H */