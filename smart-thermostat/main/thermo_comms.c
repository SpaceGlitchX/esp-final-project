#include "thermo_comms.h"

#include <stdio.h>
#include <string.h>

#include "driver/uart.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#define UART_TRANSMIT_PERIOD_MS 10000

#define THERMOSTAT_PACKET_SIZE 6

#define HVAC_STATUS_PACKET_SIZE 9


/* ============================================================
 * GLOBAL DATA
 * ============================================================ */

hvac_cmd_t thermostat_command =
    CMD_OFF;


uint8_t current_hvac_state =
    STATE_IDLE;


uint8_t current_hvac_fault =
    FLT_NONE;


uint8_t current_hvac_fan =
    0;


uint8_t current_hvac_heater =
    0;


/* ============================================================
 * CHECKSUM
 * ============================================================ */

static uint8_t calculate_checksum(
    const uint8_t *buffer,
    size_t length)
{
    uint8_t checksum = 0;


    for (size_t i = 0; i < length; i++)
    {
        checksum ^=
            buffer[i];
    }


    return checksum;
}


/* ============================================================
 * UART INITIALIZATION
 * ============================================================ */

void thermo_comms_init(void)
{
    uart_config_t uart_config = {
        .baud_rate =
            UART_BAUD_RATE,

        .data_bits =
            UART_DATA_8_BITS,

        .parity =
            UART_PARITY_DISABLE,

        .stop_bits =
            UART_STOP_BITS_1,

        .flow_ctrl =
            UART_HW_FLOWCTRL_DISABLE,

        .source_clk =
            UART_SCLK_DEFAULT
    };


    ESP_ERROR_CHECK(
        uart_param_config(
            THERMO_UART,
            &uart_config
        )
    );


    ESP_ERROR_CHECK(
        uart_set_pin(
            THERMO_UART,
            UART_TX,
            UART_RX,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );


    ESP_ERROR_CHECK(
        uart_driver_install(
            THERMO_UART,
            UART_BUFFER_SIZE,
            UART_BUFFER_SIZE,
            0,
            NULL,
            0
        )
    );


    printf(
        "Thermostat UART initialized\n"
    );


    BaseType_t result =
        xTaskCreate(
            uart_transmit_task,
            "uart_transmit",
            3072,
            NULL,
            2,
            NULL
        );


    if (result != pdPASS)
    {
        printf(
            "Failed to create UART transmit task\n"
        );

        return;
    }


    result =
        xTaskCreate(
            uart_receive_task,
            "uart_receive",
            3072,
            NULL,
            2,
            NULL
        );


    if (result != pdPASS)
    {
        printf(
            "Failed to create UART receive task\n"
        );

        return;
    }


    printf(
        "UART communication tasks started\n"
    );
}


/* ============================================================
 * UART TRANSMIT TASK
 * ============================================================ */

void uart_transmit_task(void *pvParameters)
{
    (void)pvParameters;


    uint8_t buffer[
        THERMOSTAT_PACKET_SIZE
    ];


    while (1)
    {
        /* ----------------------------------------------------
         * BYTE 0
         * HVAC COMMAND
         * ---------------------------------------------------- */

        buffer[0] =
            (uint8_t)thermostat_command;


        /* ----------------------------------------------------
         * BYTES 1-4
         * TIMESTAMP
         * ---------------------------------------------------- */

        uint32_t timestamp_us =
            (uint32_t)esp_timer_get_time();


        memcpy(
            &buffer[1],
            &timestamp_us,
            sizeof(timestamp_us)
        );


        /* ----------------------------------------------------
         * BYTE 5
         * CHECKSUM
         * ---------------------------------------------------- */

        buffer[5] =
            calculate_checksum(
                buffer,
                5
            );


        /* ----------------------------------------------------
         * TRANSMIT
         * ---------------------------------------------------- */

        int bytes_sent =
            uart_write_bytes(
                THERMO_UART,
                buffer,
                sizeof(buffer)
            );


        if (bytes_sent ==
            sizeof(buffer))
        {
            printf(
                "UART TX: Command = %d\n",
                thermostat_command
            );
        }
        else
        {
            printf(
                "UART TX failed\n"
            );
        }


        vTaskDelay(
            pdMS_TO_TICKS(
                UART_TRANSMIT_PERIOD_MS
            )
        );
    }
}


/* ============================================================
 * UART RECEIVE TASK
 * ============================================================ */

void uart_receive_task(void *pvParameters)
{
    (void)pvParameters;


    uint8_t buffer[
        HVAC_STATUS_PACKET_SIZE
    ];


    while (1)
    {
        int bytes_received =
            uart_read_bytes(
                THERMO_UART,
                buffer,
                sizeof(buffer),
                pdMS_TO_TICKS(1000)
            );


        if (bytes_received !=
            HVAC_STATUS_PACKET_SIZE)
        {
            continue;
        }


        /* ----------------------------------------------------
         * VERIFY CHECKSUM
         * ---------------------------------------------------- */

        uint8_t calculated_checksum =
            calculate_checksum(
                buffer,
                8
            );


        if (calculated_checksum !=
            buffer[8])
        {
            printf(
                "UART RX: Invalid checksum\n"
            );

            continue;
        }


        /* ----------------------------------------------------
         * STORE HVAC STATUS
         * ---------------------------------------------------- */

        current_hvac_state =
            buffer[0];


        current_hvac_fault =
            buffer[1];


        current_hvac_fan =
            buffer[2];


        current_hvac_heater =
            buffer[3];


        /* ----------------------------------------------------
         * DISPLAY
         * ---------------------------------------------------- */

        printf(
            "\n"
            "=============== UART RX ===============\n"
            "HVAC State:   %u\n"
            "HVAC Fault:   %u\n"
            "Fan State:    %u\n"
            "Heater State: %u\n"
            "=======================================\n",
            current_hvac_state,
            current_hvac_fault,
            current_hvac_fan,
            current_hvac_heater
        );
    }
}