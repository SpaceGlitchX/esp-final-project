#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"

//~  COMMUNICATIONS HEADER
//~ Defines the UART settings, HVAC commands, states and faults
//~ Declares the shared comm data and UART task functions


//  UART pins and settings
#define UART_TX GPIO_NUM_1  // UART connection to HVAC, TX
#define UART_RX GPIO_NUM_3  // UART connection to HVAC, RX
#define THERMO_UART UART_NUM_2 //UART peripheral used for HVAC comm.
#define UART_BAUD_RATE 115200 // Number of UART bits transmitted per second
#define UART_BUFFER_SIZE 256 // Size of the UART received buffer in bytes

//Commands sent from the thermostat to the HVAC controller
#define HVAC_CMD_OFF 0
#define HVAC_CMD_FAN_ONLY 1
#define HVAC_CMD_HEAT 2

//Possible HVAC states 
#define HVAC_STATE_IDLE 0
#define HVAC_STATE_FAN_CIRCULATE 1
#define HVAC_STATE_IGNITION 2
#define HVAC_STATE_WARMUP 3
#define HVAC_STATE_VERIFY_RPM 4
#define HVAC_STATE_RUNNING 5
#define HVAC_STATE_FAULT 6

//Possible HVAC fault values
#define HVAC_FAULT_FLAME 0
#define HVAC_FAULT_FAN 1
#define HVAC_FAULT_NONE 2

//Command sent to the HVAC
extern uint8_t thermostat_command;

//Most recent HVAC state
extern uint8_t current_hvac_state;

//Most recent fault value from HVAC
extern uint8_t current_hvac_fault;

//COnfigures UART and creates transmission and reception tasks
void thermo_comms_init(void);

//Sends commands to the HVAC controller
void uart_transmit_task(void *pvParameters);

//Receives the HVAC operating state and fault value
void uart_receive_task(void *pvParameters);