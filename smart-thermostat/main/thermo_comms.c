#include "thermo_comms.h"

#include <stdio.h>
#include "esp_err.h"

//Command sent to the HVAC
uint8_t thermostat_command = HVAC_CMD_OFF;

//Most recent operating state
uint8_t current_hvac_state = HVAC_STATE_IDLE;

//Most recent fault value
uint8_t current_hvac_fault = HVAC_FAULT_NONE;

void thermo_comms_init(void)
{
    //UART communication settings
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    //Apply the same settings from UART to UART2
    ESP_ERROR_CHECK(uart_param_config(THERMO_UART, &uart_config));

    //Connect UART2 to the TX and RX pins
    ESP_ERROR_CHECK(uart_set_pin(THERMO_UART, UART_TX, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    //Instal the UART driver and create the receive buffer
    ESP_ERROR_CHECK(uart_driver_install(THERMO_UART, UART_BUFFER_SIZE, 0, 0, NULL, 0));

    printf("Thermostat UART was initialized\n");
}
