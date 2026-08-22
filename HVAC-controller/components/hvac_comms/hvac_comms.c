#include "hvac_comms.h"
#include "hardware_manager.h"
#include "hvac_state_machine.h"
#include "sensor_manager.h"

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
 * Communication Handles
 * ============================================================ */

QueueHandle_t transmit_queue = NULL;
TimerHandle_t transmit_timer = NULL;


/* ============================================================
 * Checksum
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
 * Transmit Timer Callback
 * ============================================================ */

void transmit_timer_callback(TimerHandle_t timer)
{
    (void)timer;

    uint8_t event = 1;

    if (transmit_queue != NULL)
    {
        BaseType_t result =
            xQueueSend(transmit_queue, &event, 0);

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
 * UART Initialization
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

    /* Install UART driver */

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

    /* Configure UART */

    ESP_ERROR_CHECK(
        uart_param_config(
            UART_PORT,
            &uart_config
        )
    );

    /* Configure UART pins */

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


    /* ========================================================
     * Create Transmit Queue
     * ======================================================== */

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


    /* ========================================================
     * Create UART Tasks
     * ======================================================== */

    BaseType_t result;

    result = xTaskCreate(
        uart_receive_task,
        "uart_rx",
        3072,
        NULL,
        5,
        NULL
    );

    if (result != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to create UART RX task"
        );

        return;
    }


    result = xTaskCreate(
        uart_transmit_task,
        "uart_tx",
        3072,
        NULL,
        4,
        NULL
    );

    if (result != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to create UART TX task"
        );

        return;
    }


    /* ========================================================
     * Create Periodic Transmit Timer
     *
     * 500 ms = 2 status packets per second
     * ======================================================== */

    transmit_timer =
        xTimerCreate(
            "tx_timer",
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


    if (xTimerStart(transmit_timer, 0) != pdPASS)
    {
        ESP_LOGE(
            TAG,
            "Failed to start transmit timer"
        );

        return;
    }

    ESP_LOGI(
        TAG,
        "UART communication initialized successfully"
    );
}


/* ============================================================
 * Thermostat Packet RX
 *
 * Packet:
 *
 * Byte 0     Command
 * Bytes 1-4  Timestamp
 * Byte 5     Checksum
 *
 * Total = 6 bytes
 * ============================================================ */

bool unpack_thermostat_packet(
    thermostat_packet_t *data,
    const uint8_t *buffer)
{
    if (data == NULL || buffer == NULL)
    {
        ESP_LOGE(
            TAG,
            "NULL pointer in unpack_thermostat_packet"
        );

        return false;
    }


    /* Check checksum */

    uint8_t calculated_checksum =
        calculate_checksum(
            buffer,
            5
        );

    if (calculated_checksum != buffer[5])
    {
        ESP_LOGW(
            TAG,
            "Invalid thermostat packet checksum"
        );

        return false;
    }


    /* Command */

    data->thermostat_cmd =
        (hvac_cmd_t)buffer[0];


    /* Timestamp - Big Endian */

    data->timestamp_us =
        ((uint32_t)buffer[1] << 24) |
        ((uint32_t)buffer[2] << 16) |
        ((uint32_t)buffer[3] << 8)  |
        ((uint32_t)buffer[4]);


    data->checksum =
        buffer[5];


    return true;
}


/* ============================================================
 * HVAC Status Packet TX
 *
 * Packet:
 *
 * Byte 0      State
 * Byte 1      Fault
 * Byte 2      Fan state
 * Byte 3      Heater state
 * Bytes 4-7   Timestamp
 * Bytes 8-9   Flame ADC
 * Bytes 10-11 RPM
 * Byte 12     Reserved
 * Byte 13     Checksum
 *
 * Total = 14 bytes
 * ============================================================ */

void pack_hvac_status_packet(
    hvac_status_packet_t *data,
    uint8_t *buffer)
{
    if (data == NULL || buffer == NULL)
    {
        ESP_LOGE(
            TAG,
            "NULL pointer in pack_hvac_status_packet"
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
        data->fan_state;


    /* Heater */

    buffer[3] =
        data->heater_state;


    /* Timestamp */

    uint32_t timestamp_us =
        (uint32_t)esp_timer_get_time();

    buffer[4] =
        (timestamp_us >> 24) & 0xFF;

    buffer[5] =
        (timestamp_us >> 16) & 0xFF;

    buffer[6] =
        (timestamp_us >> 8) & 0xFF;

    buffer[7] =
        timestamp_us & 0xFF;


    /* Flame ADC */

    buffer[8] =
        (data->flame_value >> 8) & 0xFF;

    buffer[9] =
        data->flame_value & 0xFF;


    /* Fan RPM */

    uint16_t rpm =
        (uint16_t)data->fan_rpm;

    buffer[10] =
        (rpm >> 8) & 0xFF;

    buffer[11] =
        rpm & 0xFF;


    /* Reserved */

    buffer[12] = 0;


    /* Checksum */

    buffer[13] =
        calculate_checksum(
            buffer,
            13
        );
}


/* ============================================================
 * UART Receive Task
 * ============================================================ */

void uart_receive_task(void *pvParameters)
{
    (void)pvParameters;

    uint8_t sliding_buf[6] = {0};

    uint8_t rx_byte = 0;

    thermostat_packet_t packet;


    ESP_LOGI(
        TAG,
        "UART RX task started"
    );


    while (1)
    {
        /*
         * Read one byte at a time.
         *
         * This allows the receiver to find packet
         * boundaries even if UART data becomes misaligned.
         */

        int read_len =
            uart_read_bytes(
                UART_PORT,
                &rx_byte,
                1,
                portMAX_DELAY
            );


        if (read_len <= 0)
        {
            continue;
        }


        /*
         * Shift previous bytes left.
         */

        memmove(
            &sliding_buf[0],
            &sliding_buf[1],
            5
        );


        /*
         * Add newest byte.
         */

        sliding_buf[5] =
            rx_byte;


        /*
         * Check whether the six-byte window
         * contains a valid thermostat packet.
         */

        if (unpack_thermostat_packet(
                &packet,
                sliding_buf))
        {
            ESP_LOGI(
                TAG,
                "UART command received: %d",
                packet.thermostat_cmd
            );


            /*
             * Send command to FSM queue.
             */

            if (hvac_queue != NULL)
            {
                BaseType_t result =
                    xQueueSend(
                        hvac_queue,
                        &packet.thermostat_cmd,
                        0
                    );


                if (result != pdTRUE)
                {
                    ESP_LOGW(
                        TAG,
                        "HVAC queue full"
                    );
                }
            }


            /*
             * Clear sliding buffer after
             * successful packet reception.
             */

            memset(
                sliding_buf,
                0,
                sizeof(sliding_buf)
            );
        }
    }
}


/* ============================================================
 * UART Transmit Task
 * ============================================================ */

void uart_transmit_task(void *pvParameters)
{
    (void)pvParameters;

    uint8_t event;

    uint8_t buffer[14];

    hvac_status_packet_t packet;


    ESP_LOGI(
        TAG,
        "UART TX task started"
    );


    while (1)
    {
        /*
         * Wait for timer event.
         *
         * The task does NOT periodically delay itself.
         * The FreeRTOS timer controls when transmission occurs.
         */

        if (xQueueReceive(
                transmit_queue,
                &event,
                portMAX_DELAY) != pdTRUE)
        {
            continue;
        }


        /*
         * Read current HVAC state.
         */

        packet.current_state =
            hvac_get_state();

        packet.current_fault =
            hvac_get_fault();


        /*
         * Read actuator states.
         */

        packet.fan_state =
            (uint8_t)get_fan_state();

        packet.heater_state =
            (uint8_t)get_heater_state();


        /*
         * Read sensor values.
         */

        packet.flame_value =
            current_adc;

        packet.fan_rpm =
            current_rpm;


        /*
         * Build packet.
         */

        pack_hvac_status_packet(
            &packet,
            buffer
        );


        /*
         * Transmit packet.
         */

        int result =
            uart_write_bytes(
                UART_PORT,
                buffer,
                sizeof(buffer)
            );


        if (result < 0)
        {
            ESP_LOGE(
                TAG,
                "UART transmission failed"
            );
        }
        else
        {
            ESP_LOGD(
                TAG,
                "HVAC status packet transmitted"
            );
        }
    }
}