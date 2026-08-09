#include "thermo_comms.h"

#include <stdio.h>
#include "esp_err.h"

//Sends a command every 10 seconds
#define UART_TRANSMIT_PERIOD_MS 10000 

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

    BaseType_t transmit_task_result;
    BaseType_t receive_task_result;

    //Task that sends commands to HVAC
    transmit_task_result = xTaskCreate(uart_transmit_task, "UART transmit task", 3072, NULL, 2, NULL);

    if(transmit_task_result != pdPASS)
    {
        printf("Failed to create UART transmit task \n");
        return;
    }

    //Task that receives HVAC state and fault values
    receive_task_result = xTaskCreate(uart_receive_task, "UART receive task", 3072, NULL, 2, NULL);

    if (receive_task_result !=pdPASS)
    {
        printf("Failed to create UART receive task \n");
        return;
    }

    printf("UART communication tasks were successfully created \n");
}

void uart_transmit_task(void *pvParameters)
{
    int bytes_sent;

    while (1)
    {
        //Sends the current command in one byte
        bytes_sent = uart_write_bytes(THERMO_UART, &thermostat_command,sizeof(thermostat_command));
        if(bytes_sent == 1)
        {
            printf("%u command sent successfully\n", thermostat_command);
        }
        else
        {
            printf("Failed to send a command to HVAC\n");
        }
        vTaskDelay(pdMS_TO_TICKS(UART_TRANSMIT_PERIOD_MS));
    }
}

void uart_receive_task(void *pvParameters)
{
    uint8_t received_buffer[2];
    int bytes_received;

    while (1)
    {
        bytes_received = uart_read_bytes(THERMO_UART, received_buffer, sizeof(received_buffer), pdMS_TO_TICKS(1000));
        
        //Checks if both bytes were received
        if (bytes_received == 2)
        {
            current_hvac_state = received_buffer[0];
            current_hvac_fault = received_buffer[1];

            printf("Current HVAC state is: %u, HVAC fault: %u \n", current_hvac_state, current_hvac_fault);
        }
    }
}
