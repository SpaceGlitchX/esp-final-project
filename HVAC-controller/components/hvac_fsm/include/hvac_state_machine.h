#ifndef HVAC_STATE_MACHINE_H
#define HVAC_STATE_MACHINE_H

#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/task.h"

#include "hvac_states.h"

/* ============================================================
 * Global Handles
 * ============================================================ */

extern QueueHandle_t hvac_queue;
extern SemaphoreHandle_t hvac_state_mutex;

/* ============================================================
 * State Machine Timers
 * ============================================================ */

extern TimerHandle_t fan_warmup_timer;
extern TimerHandle_t fan_cooldown_timer;

/* ============================================================
 * Initialization
 * ============================================================ */

esp_err_t state_machine_init(void);

/* ============================================================
 * State / Fault Getters
 * ============================================================ */

hvac_state_t hvac_get_state(void);
hvac_flt_t hvac_get_fault(void);
hvac_cmd_t hvac_get_command(void);

#endif /* HVAC_STATE_MACHINE_H */