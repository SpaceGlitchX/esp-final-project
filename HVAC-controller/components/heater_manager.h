#ifndef HEATER_MANAGER_H
#define HEATER_MANAGER_H

#include "esp_err.h"

#define HEATER_OUTPUT_PIN 25

typedef enum {
    STATE_OFF,
    STATE_IGNITING,
    STATE_WAIT_FOR_FAN.
    STATE_RUNNING,
    STATE_FAULT
} state_t;

esp_err_t heater_manager_init(void);
void heater_manager_set_output(void);
void heater_manager_start_ignition(void);
void heater_manager_process_event(state_t *current_state, in *attempts,uint32_t *fan_timer_ms);

#endif