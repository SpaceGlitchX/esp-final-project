#include "hvac_comms.h"

static const char *TAG = "HVAC_COMMS";


static uint8 calculate_checksum(const uint_8 *buffer, size_t length) {

    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^=buffer[i];
    }
    return checksum;
}

// Function to set up UART for receiving data
void uart_setup()
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS, // 8 data bits
        .parity = UART_PARITY_DISABLE, // No parity
        .stop_bits = UART_STOP_BITS_1, // 1 stop bit
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(
        uart_driuver_install(
            UART_PORT,
            UART_RX_BUFFER_SIZE,
            UART_TX_BUFFER_SIZE,
            0,
            NULL,
            0));

    ESP_ERROR_CHECK(
        uart_param_config(
            UART_PORT,
            &uart_config));
        
    ESP_ERROR_CHECK(
        uart_set_pin(
            UART_PORT,
            TX_PIN,
            RX_PIN, 
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE));

    // Configure UART parameters and install driver

    // Log UART setup completion
    ESP_LOGI("UART", "UART setup successful: RX pin %d, TX pin %d", RX_PIN, TX_PIN);
}

bool unpack_thermostat_packet(thermostat_packet_t *data, uint8_t *buffer) {

    uint8_t calculate_checksum;

    // BYTE 0: HVAC CMD
    data->thermostat_cmd = (hvac_cmd_t)buffer[0];

    // BYTE 1-4: TIMESTAMP
    memcpy(&data->timestamp_us, &buffer[1], sizeof(data->timestamp_us));

    // BYTE 5: RECEIVED CHECKSUM
    data->checksum = buffer[5];

    calculate_checksum = calculate_checksum(buffer, 5);

    if (calculate_checksum == data->checksum) {
        return true;
    } else {
        ESP_LOGE(TAG, "Thermostat packet checksum error");
        return false;
    }
}

void pack_hvac_status_packet(hvac_status_packet_t *data, uint8_t *buffer) {

    // BYTE 0: CURRENT HVAC STATE
    buffer[0] = (uint8_t)data->current_state;

    // BYTE 1: CURRENT FAULT
    buffer[1] = (uint8_t)data->current_fault;

    // BYTE 2: FAN STATE
    buffer[2] = (uint8_t)data->fan_state;

    // BYTE 3-6: TIMESTAMP
    memcpy(
        &buffer[3],
        &data->timestamp_us,
        sizeof(data->timestamp_us)
    );

    // BYTE 7:
    buffer[7] = calculate_checksum(buffer, 7);
    data->checksum = buffer[7];
}

void uart_send(uint8_t *buffer, size_t length) {

    int bytes_written = uart_write_bytes(
        UART_PORT,
        buffer,
        length
    );

    if (bytes_written < 0) {
        ESP_LOGE(TAG, "UART transmission failed");
    }
}

void uart_receive(uint8_t *buffer, size_t length, uint32_t timeout_ms) {

    return uart_read_bytes(UART_PORT, buffer, length, pdMS_TO_TICKS(timeout_ms));
}
