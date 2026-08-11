#include "LCD.h"

static const char *TAG = "LCD1602";

esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE, // Enable internal pullup
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE, // Enable internal pullup
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;
    
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void lcd_send_command(uint8_t cmd) {
    uint8_t write_buf[2] = {LCD_CONTROL_COMMAND, cmd};
    
    // Direct link abstraction to transmit address and payload via driver/i2c.h
    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_I2C_ADDRESS, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Command transfer failed: %s", esp_err_to_name(err));
    }
}


void lcd_send_data(uint8_t data) {
    uint8_t write_buf[2] = {LCD_CONTROL_DATA, data};
    
    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_I2C_ADDRESS, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Data transfer failed: %s", esp_err_to_name(err));
    }
}


void lcd_init(void) {
    vTaskDelay(pdMS_TO_TICKS(50)); // Wait for internal controller to power up completely
    
    lcd_send_command(LCD_CMD_FUNCTION_SET);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    lcd_send_command(LCD_CMD_DISPLAY_CONTROL);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    lcd_send_command(LCD_CMD_ENTRY_MODE_SET);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    ESP_LOGI(TAG, "Native LCD Initialization Sequence Finished.");

    i2c_master_init();

    lcd_set_cursor(0, 0);
    lcd_send_string("Native I2C Mode");

    lcd_set_cursor(1, 0);
    lcd_send_string("No Expander Pack");
}


void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? (0x00 + col) : (0x40 + col);
    lcd_send_command(0x80 | address); // Set DDRAM Address instruction mask
}


void lcd_send_string(const char *str) {
    while (*str) {
        lcd_send_data((uint8_t)(*str));
        str++;
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C master setup initiated.");

    lcd_init();

    lcd_set_cursor(0, 0);
    lcd_send_string("Native I2C Mode");

    lcd_set_cursor(1, 0);
    lcd_send_string("No Expander Pack");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}