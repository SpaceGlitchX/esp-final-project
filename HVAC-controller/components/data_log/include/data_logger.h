// data_logger.h

#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdint.h>
#include "hvac_states.h"

typedef struct {
    hvac_state_t state;
    hvac_flt_t fault;
    int fan_state;
    int heater_state;
    float fan_rpm;
    float flame_value;
} hvac_log_data_t;

typedef enum {
    LOG_EVENT_PERIODIC,
    LOG_EVENT_STATE_CHANGE,
    LOG_EVENT_FAULT,
    LOG_EVENT_COMMAND,
    LOG_EVENT_SENSOR
} log_event_type_t;

typedef struct {
    log_event_type_t type;
} log_msg_t;

void data_logger_init(void);
void data_logger_log_event(log_event_type_t type);

#endif