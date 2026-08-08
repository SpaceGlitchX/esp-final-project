#ifndef HVAC_STATES_H
#define HVAC_STATES_H

// State machine blocks
typedef enum {
    STATE_IDLE = 0,
    STATE_FAN_CIRCULATE,
    STATE_IGNITION,
    STATE_WARMUP,
    STATE_VERIFY_RPM,
    STATE_RUNNING,
    STATE_FAULT,
} hvac_state_t;

// State machine events
typedef enum {
    CMD_OFF = 0,
    CMD_FAN_ONLY,
    CMD_HEAT,
    CMD_FLAME_DETECTED,
    CMD_FLAME_TIMEOUT,
    CMD_WARMUP_DONE,
    CMD_TACH_TIMEOUT
} hvac_cmd_t;

// Fault flag
typedef enum {
    FLT_FLAME,
    FLT_FAN,
    FLT_OTHER
} hvac_flt_t;

#endif