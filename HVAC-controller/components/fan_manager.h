#ifndef FAN_MANAGER_H
#define FAN_MANAGER_H

#define FAN_OUTPUT_PIN 17

typedef enum {
    STATE_OFF,
    STATE_ON,
    STATE_IDLE,
    STATE_FAULT
} state_t;

