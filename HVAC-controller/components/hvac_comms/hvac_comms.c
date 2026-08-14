#include "hvac_comms.h"
#include "hvac_state_machine.h"
#include "hardware_manager.h"

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"


static const char *TAG = "HVAC_COMMS";


/* ============================================================
 * QUEUES AND TIMER
 * ============================================================ */

/*
 * RX:
 * UART receive task -> HVAC FSM queue
 *
 * TX:
 * Timer -> transmit queue -> UART transmit task
 */

QueueHandle_t transmit_queue = NULL;

TimerHandle_t transmit_timer = NULL;


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
        checksum ^= buffer[i];
    }

    return checksum;
}


/* ============================================================
 * TRANSMIT TIMER CALLBACK
 * ============================================================ */

void transmit_timer_callback(TimerHandle_t timer)
{
    (void)timer;

    uint8_t event = 1;

    if (transmit_queue != NULL)
    {
        BaseType_t result =
            xQueueSend(
                transmit_queue,
                &event,
                0
            );

        if (result != pdTRUE)
        {
            ESP_LOGW(
                TAG,
                "Transmit queue full"
            );
        }
    }
}


/* ============================================================
 * UART INITIALIZATION
 * ============================================================ */

void uart_init(void)
{
    uart_config_t uart_config =
    {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };


    /* --------------------------------------------------------
     * Install UART driver
     * -------------------------------------------------------- */

    ESP_ERROR_CHECK(
        uart_driver_install(
            UART_PORT,
            UART_RX_BUFFER_SIZE,
            UART_TX_BUFFER_SIZE,
            0,
            NULL,
            0
        )
    );


    /* --------------------------------------------------------
     * Configure UART
     * -------------------------------------------------------- */

    ESP_ERROR_CHECK(
        uart_param_config(
            UART_PORT,
            &uart_config
        )
    );


    /* --------------------------------------------------------
     * Configure UART pins
     * -------------------------------------------------------- */

    ESP_ERROR_CHECK(
        uart_set_pin(
            UART_PORT,
            TX_PIN,
            RX_PIN,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE
        )
    );


    ESP_LOGI(
        TAG,
        "UART initialized: port=%d RX=%d TX=%d baud=%d",
        UART_PORT,
        RX_PIN,
        TX_PIN,
        UART_BAUD_RATE
    );


    /* --------------------------------------------------------
     * Create transmit queue
     * -------------------------------------------------------- */

    transmit_queue =
        xQueueCreate(
            5,
            sizeof(uint8_t)
        );

    if (transmit_queue == NULL)
    {
        ESP_LOGE(
            TAG,
            "Failed to create transmit queue"
        );

        return;
    }


    /* --------------------------------------------------------
     * Start RX task
     * -------------------------------------------------------- */

    BaseType_t result;

    result =
        xTaskCreate(
            uart_receive_task,
            "uart_receive",
            3072,
            NULL,
            5,
            NULL
        );

    if (result != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to create UART receive task"
        );
    }


    /* --------------------------------------------------------
     * Start TX task
     * -------------------------------------------------------- */

    result =
        xTaskCreate(
            uart_transmit_task,
            "uart_transmit",
            3072,
            NULL,
            4,
            NULL
        );

    if (result != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to create UART transmit task"
        );
    }

    transmit_timer =
        xTimerCreate(
            "transmit_timer",
            pdMS_TO_TICKS(500),
            pdTRUE,
            NULL,
            transmit_timer_callback
        );

    if (transmit_timer == NULL)
    {
        ESP_LOGE(
            TAG,
            "Failed to create transmit timer"
        );

        return;
    }


    /* --------------------------------------------------------
     * Start transmit timer
     * -------------------------------------------------------- */

    if (
        xTimerStart(
            transmit_timer,
            pdMS_TO_TICKS(1000)
        ) != pdPASS
    )
    {
        ESP_LOGE(
            TAG,
            "Failed to start transmit timer"
        );
    }


    ESP_LOGI(
        TAG,
        "UART communication system started"
    );
}


/* ============================================================
 * THERMOSTAT PACKET RX
 * ============================================================ */

/*
 * Thermostat packet:
 *
 * BYTE 0       HVAC command
 * BYTES 1-4    Timestamp
 * BYTE 5       Checksum
 *
 * TOTAL = 6 bytes
 */

bool unpack_thermostat_packet(
    thermostat_packet_t *data,
    const uint8_t *buffer)
{
    if (
        data == NULL ||
        buffer == NULL
    )
    {
        ESP_LOGE(
            TAG,
            "NULL pointer passed to unpack_thermostat_packet"
        );

        return false;
    }


    /* HVAC command */

    data->thermostat_cmd =
        (hvac_cmd_t)buffer[0];


    /* Timestamp */

    memcpy(
        &data->timestamp_us,
        &buffer[1],
        sizeof(data->timestamp_us)
    );


    /* Received checksum */

    data->checksum =
        buffer[5];


    /* Calculate checksum */

    uint8_t calculated_checksum =
        calculate_checksum(
            buffer,
            5
        );


    /* Validate checksum */

    if (
        calculated_checksum !=
        data->checksum
    )
    {
        ESP_LOGE(
            TAG,
            "Checksum error: received=0x%02X calculated=0x%02X",
            data->checksum,
            calculated_checksum
        );

        return false;
    }


    ESP_LOGD(
        TAG,
        "Valid thermostat packet: command=%d timestamp=%lu",
        data->thermostat_cmd,
        (unsigned long)data->timestamp_us
    );


    return true;
}


/* ============================================================
 * HVAC STATUS PACKET TX
 * ============================================================ */

/*
 * HVAC status packet:
 *
 * BYTE 0       Current HVAC state
 * BYTE 1       Current fault
 * BYTE 2       Fan state
 * BYTE 3       Heater state
 * BYTES 4-7   Timestamp
 * BYTE 8       Checksum
 *
 * TOTAL = 9 bytes
 */

void pack_hvac_status_packet(
    hvac_status_packet_t *data,
    uint8_t *buffer)
{
    if (
        data == NULL ||
        buffer == NULL
    )
    {
        ESP_LOGE(
            TAG,
            "NULL pointer passed to pack_hvac_status_packet"
        );

        return;
    }


    /* State */

    buffer[0] =
        (uint8_t)data->current_state;


    /* Fault */

    buffer[1] =
        (uint8_t)data->current_fault;


    /* Fan */

    buffer[2] =
        (uint8_t)data->fan_state;


    /* Heater */

    buffer[3] =
        (uint8_t)data->heater_state;


    /* Timestamp */

    uint32_t timestamp_us =
        (uint32_t)esp_timer_get_time();


    memcpy(
        &buffer[4],
        &timestamp_us,
        sizeof(timestamp_us)
    );


    data->timestamp_us =
        timestamp_us;


    /* Checksum */

    buffer[8] =
        calculate_checksum(
            buffer,
            8
        );


    data->checksum =
        buffer[8];
}


/* ============================================================
 * UART SEND
 * ============================================================ */

int uart_send(
    uint8_t *buffer,
    size_t length)
{
    if (
        buffer == NULL ||
        length == 0
    )
    {
        return -1;
    }


    int bytes_written =
        uart_write_bytes(
            UART_PORT,
            buffer,
            length
        );


    if (bytes_written < 0)
    {
        ESP_LOGE(
            TAG,
            "UART transmission failed"
        );

        return bytes_written;
    }


    ESP_LOGD(
        TAG,
        "UART transmitted %d bytes",
        bytes_written
    );


    return bytes_written;
}


/* ============================================================
 * UART RECEIVE
 * ============================================================ */

int uart_receive(
    uint8_t *buffer,
    size_t length,
    uint32_t timeout_ms)
{
    if (
        buffer == NULL ||
        length == 0
    )
    {
        return -1;
    }


    return uart_read_bytes(
        UART_PORT,
        buffer,
        length,
        pdMS_TO_TICKS(timeout_ms)
    );
}


/* ============================================================
 * UART RECEIVE TASK
 * ============================================================ */

void uart_receive_task(
    void *pvParameters)
{
    (void)pvParameters;


    uint8_t buffer[6];

    thermostat_packet_t packet;


    ESP_LOGI(
        TAG,
        "HVAC UART receive task started"
    );


    while (1)
    {

        int bytes_received =
            uart_receive(
                buffer,
                sizeof(buffer),
                100
            );


        if (
            bytes_received ==
            sizeof(buffer)
        )
        {
            /*
             * Validate packet.
             */

            if (
                unpack_thermostat_packet(
                    &packet,
                    buffer
                )
            )
            {
                ESP_LOGI(
                    TAG,
                    "Thermostat command received: %d",
                    packet.thermostat_cmd
                );


                /*
                 * Send command to FSM.
                 */

                if (
                    hvac_queue !=
                    NULL
                )
                {
                    BaseType_t result =
                        xQueueSend(
                            hvac_queue,
                            &packet.thermostat_cmd,
                            0
                        );


                    if (
                        result !=
                        pdTRUE
                    )
                    {
                        ESP_LOGW(
                            TAG,
                            "HVAC command queue full"
                        );
                    }
                }
            }
        }


        else if (
            bytes_received > 0
        )
        {
            ESP_LOGW(
                TAG,
                "Incomplete packet: %d/%d bytes",
                bytes_received,
                sizeof(buffer)
            );
        }
    }
}


/* ============================================================
 * UART TRANSMIT TASK
 * ============================================================ */

void uart_transmit_task(
    void *pvParameters)
{
    (void)pvParameters;


    uint8_t event;

    uint8_t buffer[9];

    hvac_status_packet_t packet;


    ESP_LOGI(
        TAG,
        "HVAC UART transmit task started"
    );


    while (1)
    {
        
        if (
            xQueueReceive(
                transmit_queue,
                &event,
                portMAX_DELAY
            ) == pdTRUE
        )
        {
            /*
             * Build current HVAC status.
             */

            packet.current_state =
                g_current_hvac_state;

            packet.current_fault =
                g_current_hvac_fault;


            /*
             * Get current actuator states.
             *
             * Replace these with your actual getter
             * functions if hardware_manager provides them.
             */

            packet.fan_state =
                get_fan_state();

            packet.heater_state =
                get_heater_state();


            /*
             * Pack the status packet.
             */

            pack_hvac_status_packet(
                &packet,
                buffer
            );


            /*
             * Transmit packet.
             */

            int result =
                uart_send(
                    buffer,
                    sizeof(buffer)
                );


            if (
                result ==
                sizeof(buffer)
            )
            {
                ESP_LOGD(
                    TAG,
                    "HVAC status packet transmitted"
                );
            }
            else
            {
                ESP_LOGW(
                    TAG,
                    "HVAC status packet transmission incomplete"
                );
            }
        }
    }
}