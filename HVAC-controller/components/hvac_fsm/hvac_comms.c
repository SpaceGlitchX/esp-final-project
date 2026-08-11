#include "hvac_comms.h"

static RX_data rx_data = {0};
static TX_data_tx_data = {0};
extern hvac_cmd_t ;
extern hvac_flt_t current_fault
void unpack_buffer_to_data(uint8_t *buffer, RX_data *data)
{
    // For each pair of bytes, reconstruct the uint16_t
    data->thermostat_command = ((uint16_t)(buffer[0] << 8) | buffer[1]); // Combine high and low bytes
    data->speed_mms = ((uint16_t)(buffer[2] << 8) | buffer[3]);    // Combine high and low bytes

    // Extract the single bytes
    data->tooth_spacing = (uint8_t)buffer[4];
    data->timestamp_us = ((uint32_t)(buffer[5] << 8) | buffer[6]); // Combine high and low bytes
    data->checksum = buffer[7];

    // Validate checksum
    uint8_t calculated_checksum = 0;
    for (int i = 0; i < 7; i++)
    { // modulo 256 to fit in uint8_t
        calculated_checksum += buffer[i];
    }
    if (calculated_checksum != data->checksum)
    {
        ESP_LOGE("RX", "ERROR: Data corruption!");
    }
    else
    {
        ESP_LOGI("RX", "Speed: %d mm/s", data->speed_mms);
    }
}

// Function to set up UART for receiving data
void uart_setup()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS, // 8 data bits
        .parity = UART_PARITY_DISABLE, // No parity
        .stop_bits = UART_STOP_BITS_1, // 1 stop bit
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    // Configure UART parameters and install driver
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0));

    // Log UART setup completion
    ESP_LOGI("UART", "UART setup successful: RX pin %d, TX pin %d", RX_PIN, TX_PIN);
}
// Function to receive data over UART and unpack it
void uart_receive_task(void *pvParameters)
{
    uint8_t buffer[8];
    
    while (1)
    {
        // Read 8 bytes from UART with a timeout of 100ms
        int len = uart_read_bytes(UART_NUM_1,
                                  buffer,
                                  sizeof(buffer),
                                  pdMS_TO_TICKS(100));
        
        // If we received 8 bytes, unpack the data
        if (len == sizeof(buffer))
        {
            SensorData data;
            unpack_buffer_to_data(buffer, &data);

            ESP_LOGI("RX",
                     "Received Data: Frequency: %u Hz, Speed: %u mm/s, Tooth Spacing: %u mm, Timestamp: %u us, Checksum: %u",
                     data.frequency_hz,
                     data.speed_mms,
                     data.tooth_spacing,
                     data.timestamp_us,
                     data.checksum);
        }
        // If we received fewer than 8 bytes, log a warning
        else if (len > 0)
        {
            ESP_LOGW("RX", "Incomplete packet (%d/8 bytes)", len);
        }
    }
}

void app_main(void)
{
    uart_setup();
    xTaskCreate(uart_receive_task, "uart_receive_task", 4096, NULL, 10, NULL);
}