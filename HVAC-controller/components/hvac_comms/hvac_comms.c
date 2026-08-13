#include "hvac_comms.h"
#include "hvac_state_machine.h"

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


static const char *TAG = "HVAC_COMMS";


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
 * UART Initialization
 * ============================================================ */

void uart_setup(void)
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

    // Install UART driver.
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

    // Configure UART parameters.
    ESP_ERROR_CHECK(
        uart_param_config(
            UART_PORT,
            &uart_config
        )
    );

    // Configure UART pins.
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
}


/* ============================================================
 * Thermostat Packet RX
 * ============================================================ */

/*
 * Thermostat packet format:
 *
 * BYTE 0       HVAC command
 * BYTES 1-4    Timestamp
 * BYTE 5       Checksum
 *
 * Total = 6 bytes
 */
bool unpack_thermostat_packet(
    thermostat_packet_t *data,
    const uint8_t *buffer)
{
    if (data == NULL || buffer == NULL)
    {
        ESP_LOGE(
            TAG,
            "NULL pointer passed to unpack_thermostat_packet"
        );

        return false;
    }

    /*
     * BYTE 0:
     * HVAC command.
     */
    data->thermostat_cmd =
        (hvac_cmd_t)buffer[0];

    /*
     * BYTES 1-4:
     * Timestamp.
     */
    memcpy(
        &data->timestamp_us,
        &buffer[1],
        sizeof(data->timestamp_us)
    );

    /*
     * BYTE 5:
     * Received checksum.
     */
    data->checksum = buffer[5];

    // Calculate checksum using bytes 0-4.
    uint8_t calculated_checksum =
        calculate_checksum(
            buffer,
            5
        );

    // Validate checksum.
    if (calculated_checksum != data->checksum)
    {
        ESP_LOGE(
            TAG,
            "Thermostat packet checksum error "
            "(received=0x%02X calculated=0x%02X)",
            data->checksum,
            calculated_checksum
        );

        return false;
    }

    ESP_LOGD(
        TAG,
        "Thermostat packet valid: command=%d timestamp=%lu",
        data->thermostat_cmd,
        (unsigned long)data->timestamp_us
    );

    return true;
}


/* ============================================================
 * HVAC Status Packet TX
 * ============================================================ */

/*
 * HVAC status packet format:
 *
 * BYTE 0       Current HVAC state
 * BYTE 1       Current fault
 * BYTE 2       Fan state
 * BYTE 3       Heater state
 * BYTES 4-7    Timestamp
 * BYTE 8       Checksum
 *
 * Total = 9 bytes
 */
void pack_hvac_status_packet(
    hvac_status_packet_t *data,
    uint8_t *buffer)
{
    if (data == NULL || buffer == NULL)
    {
        ESP_LOGE(
            TAG,
            "NULL pointer passed to pack_hvac_status_packet"
        );

        return;
    }

    /*
     * BYTE 0:
     * Current HVAC state.
     */
    buffer[0] =
        (uint8_t)data->current_state;

    /*
     * BYTE 1:
     * Current fault.
     */
    buffer[1] =
        (uint8_t)data->current_fault;

    /*
     * BYTE 2:
     * Fan state.
     */
    buffer[2] =
        (uint8_t)data->fan_state;

    /*
     * BYTE 3:
     * Heater state.
     */
    buffer[3] =
        (uint8_t)data->heater_state;

    /*
     * BYTES 4-7:
     * Timestamp.
     *
     * esp_timer_get_time() returns microseconds.
     */
    uint32_t timestamp_us =
        (uint32_t)esp_timer_get_time();

    memcpy(
        &buffer[4],
        &timestamp_us,
        sizeof(timestamp_us)
    );

    /*
     * Store timestamp in the packet structure.
     */
    data->timestamp_us =
        timestamp_us;

    /*
     * BYTE 8:
     * Checksum.
     */
    buffer[8] =
        calculate_checksum(
            buffer,
            8
        );

    data->checksum =
        buffer[8];
}


/* ============================================================
 * UART TX
 * ============================================================ */

int uart_send(
    uint8_t *buffer,
    size_t length)
{
    if (buffer == NULL || length == 0)
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
 * UART RX
 * ============================================================ */

int uart_receive(
    uint8_t *buffer,
    size_t length,
    uint32_t timeout_ms)
{
    if (buffer == NULL || length == 0)
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
 * UART Receive Task
 * ============================================================ */

void hvac_uart_receive_task(
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
        // Wait for a complete thermostat packet. Timeout = 100 ms.
        int bytes_received =
            uart_receive(
                buffer,
                sizeof(buffer),
                100
            );

        if (bytes_received == sizeof(buffer))
        {
            
            // Validate and unpack packet.
            if (unpack_thermostat_packet(
                    &packet,
                    buffer))
            {
                ESP_LOGI(
                    TAG,
                    "Thermostat command received: %d",
                    packet.thermostat_cmd
                );

                if (hvac_queue != NULL)
                {
                    
                    // Send the command to the FSM.
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
                            "HVAC command queue full"
                        );
                    }
                    else
                    {
                        ESP_LOGD(
                            TAG,
                            "Command sent to HVAC FSM"
                        );
                    }
                }
                else
                {
                    ESP_LOGE(
                        TAG,
                        "HVAC queue is NULL"
                    );
                }
            }
        }
        else if (bytes_received > 0)
        {
            ESP_LOGW(
                TAG,
                "Incomplete thermostat packet: "
                "%d/%d bytes",
                bytes_received,
                sizeof(buffer)
            );
        }
    }
}