#ifndef HVAC_COMMS_H
#define HVAC_COMMS_H

#include <stdint.h>
#include "hvac_states.h"
#include "hvac_fsm.h"

#define UART_PORT UART_NUM_1
#define UART_BAUD_RATE 115200

#define RX_PIN GPIO_NUM_16 // Define the GPIO pin for UART RX
#define TX_PIN GPIO_NUM_17 // Define the GPIO pin for UART TX

#define UART_RX_BUFFER_SIZE 256
#define UART_TX_BUFFER_SIZE 256

typedef struct {
    hvac_cmd_t thermostat_cmd;
    uint32_t timestamp_us;
    uint8_t checksum;
} thermostat_packet_t;

typedef struct {
    hvac_state_t current_state;
    hvac_flt_t current_fault;
    fan_state_t fan_state;
    uint32_t timestamp_us;
    uint8_t checksum;
} hvac_status_packet_t;

void gpio_setup(void);
void uart_setup(void);

void unpack_buffer_to_data(
    thermostat_packet_t *data, 
    uint8_t *buffer
);

void pack_data_to_buffer(
    hvac_status_packet_t *data, 
    uint8_t *buffer
);

void send_sensor_data(
    uint8_t *buffer, 
    size_t length
);

#endif // RECEIVER_ESP32_H