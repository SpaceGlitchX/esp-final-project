#ifndef HVAC_COMMS_H
#define HVAC_COMMS_H
#include <stdint.h>
#include "hvac_states.h"
#include "hvac_fsm.h"

#define RX_PIN GPIO_NUM_16 // Define the GPIO pin for UART RX
#define TX_PIN GPIO_NUM_17 // Define the GPIO pin for UART TX

extern typedef struct {
    extern hvac_cmd_t thermostat_command;
    uint32_t timestamp_us;
    uint8_t checksum;
} RX_data;

extern typedef struct {
    extern hvac_state_t current_state;
    extern hvac_flt_t current_fault;
    extern fan_state;
    uint32_t timestamp_us;
    uint8_t checksum;
} TX_data;

void gpio_setup(void);
void unpack_buffer_to_data(RX_data *data, uint8_t *buffer);
void pack_data_to_buffer(TX_data *data, uint8_t *buffer);
void uart_setup(void);
void send_sensor_data(uint8_t *buffer, size_t length);

#endif // RECEIVER_ESP32_H