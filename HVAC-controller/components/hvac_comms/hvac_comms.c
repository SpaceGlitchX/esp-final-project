#include "hvac_comms.h"
#include "hvac_state_machine.h"

#include <string.h>

#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


static const char *TAG = "HVAC_COMMS";


/*
 * Calculate an XOR checksum over a buffer.
 *
 * The checksum is calculated over all packet bytes
 * except the final checksum byte.
 */
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


/*
 * Initialize UART communication.
 */
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

    /*
     * Install UART driver.
     */
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

    /*
     * Configure UART parameters.
     */
    ESP_ERROR_CHECK(
        uart_param_config(
            UART_PORT,
            &uart_config
        )
    );

    /*
     * Configure UART pins.
     */
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
        "UART setup successful: RX pin %d, TX pin %d, baud rate %d",
        RX_PIN,
        TX_PIN,
        UART_BAUD_RATE
    );
}


/*
 * Unpack and validate a thermostat packet.
 *
 * Thermostat packet format:
 *
 * BYTE 0     HVAC command
 * BYTES 1-4  Timestamp
 * BYTE 5     Checksum
 *
 * Total packet size = 6 bytes
 */
bool unpack_thermostat_packet(
    thermostat_packet_t *data,
    const uint8_t *buffer)
{
    uint8_t calculated_checksum;

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

    /*
     * Calculate checksum using bytes 0-4.
     */
    calculated_checksum =
        calculate_checksum(
            buffer,
            5
        );

    /*
     * Validate checksum.
     */
    if (calculated_checksum == data->checksum)
    {
        return true;
    }

    ESP_LOGE(
        TAG,
        "Thermostat packet checksum error"
    );

    return false;
}


/*
 * Pack the current HVAC status into a UART packet.
 *
 * HVAC status packet format:
 *
 * BYTE 0     HVAC state
 * BYTE 1     Fault
 * BYTE 2     Fan state
 * BYTE 3     Heater state
 * BYTES 4-7 Timestamp
 * BYTE 8     Checksum
 *
 * Total packet size = 9 bytes.
 */
void pack_hvac_status_packet(
    hvac_status_packet_t *data,
    uint8_t *buffer)
{
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
     */
    memcpy(
        &buffer[4],
        &data->(uint32_t)(esp_timer_get_time()/1000);
        sizeof(data->timestamp_us)
    );

    /*
     * BYTE 8:
     * Checksum.
     */
    buffer[8] =
        calculate_checksum(
            buffer,
            8
        );

    data->checksum = buffer[8];
}


/*
 * Send a packet over UART.
 */
int uart_send(
    uint8_t *buffer,
    size_t length)
{
    int bytes_written;

    bytes_written =
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
    }

    return bytes_written;
}


/*
 * Receive data from UART.
 *
 * timeout_ms specifies how long to wait for data.
 */
int uart_receive(
    uint8_t *buffer,
    size_t length,
    uint32_t timeout_ms)
{
    return uart_read_bytes(
        UART_PORT,
        buffer,
        length,
        pdMS_TO_TICKS(timeout_ms)
    );
}


/*
 * UART receive task.
 *
 * This task waits for thermostat packets,
 * validates them, and sends valid commands
 * to the HVAC state machine queue.
 */
void hvac_uart_receive_task(
    void *pvParameters)
{
    (void)pvParameters;

    /*
     * Thermostat packet is 6 bytes:
     *
     * Command     = 1 byte
     * Timestamp   = 4 bytes
     * Checksum    = 1 byte
     */
    uint8_t buffer[6];

    thermostat_packet_t packet;

    while (1)
    {
        /*
         * Wait for a complete thermostat packet.
         */
        int bytes_received =
            uart_receive(
                buffer,
                sizeof(buffer),
                100
            );

        /*
         * Complete packet received.
         */
        if (bytes_received == sizeof(buffer))
        {
            /*
             * Validate and unpack packet.
             */
            if (unpack_thermostat_packet(
                    &packet,
                    buffer))
            {
                ESP_LOGI(
                    TAG,
                    "Received thermostat command: %d",
                    packet.thermostat_cmd
                );

                /*
                 * Send the command to the
                 * HVAC state machine queue.
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
                            "HVAC command queue full"
                        );
                    }
                }
                else
                {
                    ESP_LOGE(
                        TAG,
                        "HVAC command queue is NULL"
                    );
                }
            }
        }

        /*
         * Some data was received, but it was
         * not a complete packet.
         */
        else if (bytes_received > 0)
        {
            ESP_LOGW(
                TAG,
                "Incomplete packet: %d/%d bytes",
                bytes_received,
                sizeof(buffer)
            );
        }

        /*
         * Allow other FreeRTOS tasks to run.
         */
        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
    }
}