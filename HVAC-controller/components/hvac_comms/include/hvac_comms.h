#ifndef HVAC_COMMS_H
#define HVAC_COMMS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "hvac_states.h"


#define UART_PORT UART_NUM_0

#define UART_BAUD_RATE 115200

#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256

#define TX_PIN GPIO_NUM_1
#define RX_PIN GPIO_NUM_3


typedef struct
{
    hvac_cmd_t thermostat_cmd;
    uint32_t timestamp_us;
    uint8_t checksum;

} thermostat_packet_t;


typedef struct
{
    hvac_state_t current_state;
    hvac_flt_t current_fault;

    uint8_t fan_state;
    uint8_t heater_state;

    uint32_t timestamp_us;

    uint16_t flame_value;
    uint32_t fan_rpm;

    uint8_t checksum;

} hvac_status_packet_t;


/* Handles are defined in hvac_comms.c */

extern QueueHandle_t transmit_queue;
extern TimerHandle_t transmit_timer;


/* Initialization */

void uart_init(void);


/* Packet functions */

bool unpack_thermostat_packet(
    thermostat_packet_t *data,
    const uint8_t *buffer
);

void pack_hvac_status_packet(
    hvac_status_packet_t *data,
    uint8_t *buffer
);


/* Tasks */

void uart_receive_task(void *pvParameters);

void uart_transmit_task(void *pvParameters);


/* Timer */

void transmit_timer_callback(
    TimerHandle_t timer
);

#endif