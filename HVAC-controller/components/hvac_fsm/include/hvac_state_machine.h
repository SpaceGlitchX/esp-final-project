#ifndef HVAC_STATE_MACHINE_H
#define HVAC_STATE_MACHINE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_err.h"
#include "hvac_states.h"

// Command queue for sending events/commands to the state machine task
extern QueueHandle_t hvac_queue;

// Exported globally for testing and monitoring
extern hvac_state_t g_current_hvac_state;
extern hvac_flt_t g_current_hvac_fault;

/**
 * @brief Hardware Emergency Interrupt Handler
 */
void fault_isr_handler(void *arg);

/**
 * @brief Initializes state variables, queues, and timer resources.
 * @return esp_err_t ESP_OK on success, ESP_FAIL on failure.
 */
esp_err_t hvac_state_machine_init(void);

/**
 * @brief Main HVAC Finite State Machine Task
 */
void hvac_state_machine_task(void *pvParameters);

void get_hvac_state(void);
void get_hvac_fault(void);
#endif // HVAC_STATE_MACHINE_H