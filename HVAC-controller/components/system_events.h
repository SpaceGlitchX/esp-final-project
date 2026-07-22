#ifndef SYSTEM_EVENTS_H
#define SYSTEM_EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
    EVENT_FLAME_DETECTED,
    EVENT_FLAME_LOST,
    EVENT_FAN_OK,
    EVENT_FAN_FAILED,

    CMD_HEATER_ON,
    CMD_HEATER_OFF,
    CMD_FAN_ON,
    CMD_FAN_AUTO
} event_id_t;

extern QueueHandle_t system_event_queue;

#endif