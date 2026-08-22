#ifndef HVAC_STATES_H
#define HVAC_STATES_H

#include <stdint.h>
#include <stdbool.h>


/* ============================================================
 * HVAC STATES
 * ============================================================ */


typedef enum {
    STATE_IDLE = 0,
    STATE_FAN_CIRCULATE,
    STATE_IGNITION,
    STATE_WARMUP,
    STATE_VERIFY_RPM,
    STATE_RUNNING,
    STATE_COOLDOWN, // Added
    STATE_FAULT
} hvac_state_t;

typedef enum {
    CMD_OFF = 0,
    CMD_HEAT,
    CMD_HEAT_OFF,
    CMD_FAN_ON,
    CMD_FAN_AUTO,
    CMD_FLAME_DETECTED,
    CMD_FLAME_TIMEOUT,
    CMD_WARMUP_DONE,
    CMD_FAN_OK,
    CMD_TACH_TIMEOUT,
    CMD_COOLDOWN_DONE // Added
} hvac_cmd_t;
/* ============================================================
 * HVAC FAULTS
 * ============================================================ */

typedef enum {

    FLT_NONE = 0,

    FLT_FLAME,

    FLT_FAN,

    FLT_SAFETY

} hvac_flt_t;


#endif