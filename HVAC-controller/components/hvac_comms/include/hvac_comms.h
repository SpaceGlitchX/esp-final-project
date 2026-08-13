#ifndef HVAC_COMMS_H
#define HVAC_COMMS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "driver/gpio.h"

#include "hvac_states.h"


#define UART_PORT UART_NUM_1
#define UART_BAUD_RATE 115200

#define RX_PIN GPIO_NUM_16
#define TX_PIN GPIO_NUM_17

#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256


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

    int fan_state;
    int heater_state;

    uint32_t timestamp_us;
    uint8_t checksum;

} hvac_status_packet_t;


extern void uart_setup(void);


bool unpack_thermostat_packet(
    thermostat_packet_t *data,
    const uint8_t *buffer
);


void pack_hvac_status_packet(
    hvac_status_packet_t *data,
    uint8_t *buffer
);


int uart_send(
    uint8_t *buffer,
    size_t length
);


int uart_receive(
    uint8_t *buffer,
    size_t length,
    uint32_t timeout_ms
);


void hvac_uart_receive_task(
    void *pvParameters
);


#endif // HVAC_COMMS_H