#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdint.h>
#include "hvac_states.h"
#include "sensor_manager.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"

typedef struct {
    hvac_state_t state;
    hvac_flt_t fault;
    int fan_state;
    int heater_state;
    float fan_rpm;
    float flame_value;
} hvac_log_data_t;

typedef enum {
    LOG_EVENT_PEROIDIC,
    LOG_EVENT_STATE_CHANGE,
    LOG_EVENT_FAULT,
    LOG_EVENT_COMMAND,
    LOG_EVENT_SENSOR
} log_event_type_t;

typedef struct {
    log_event_type_t type;
    hvac_log_data_t data;
} log_msg_t;

void data_logger_init(void);
static void data_logger_write(const hvac_log_data_t *data, log_event_type_t type);
static void log_timer_callback(TimerHandle_t xTimer);
static void collect_log_data(hvac_log_data_t *data);
void data_logger_task(hvac_log_data_t *log_data);
void data_logger_log_event(log_event_type_t type); 
#endif