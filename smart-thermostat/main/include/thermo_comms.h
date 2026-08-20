#ifndef THERMO_COMMS_H
#define THERMO_COMMS_H

#include <stdbool.h>
#include <stdint.h>
#include "hvac_states.h"
#include "driver/gpio.h"
#include "driver/uart.h"



/* ============================================================
 * UART SETTINGS
 * ============================================================ */

#define UART_TX GPIO_NUM_1
#define UART_RX GPIO_NUM_3

#define THERMO_UART UART_NUM_2

#define UART_BAUD_RATE 115200

#define UART_BUFFER_SIZE 256


/* ============================================================
 * HVAC DATA RECEIVED FROM UART
 * ============================================================ */

extern uint8_t current_hvac_state;

extern uint8_t current_hvac_fault;

extern uint8_t current_hvac_fan;

extern uint8_t current_hvac_heater;


/* ============================================================
 * COMMAND SENT TO HVAC
 * ============================================================ */

extern hvac_cmd_t thermostat_command;


/* ============================================================
 * FUNCTIONS
 * ============================================================ */

extern void thermo_comms_init(void);

void uart_transmit_task(void *pvParameters);

void uart_receive_task(void *pvParameters);

#endif