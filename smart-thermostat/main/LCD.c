#include "LCD.h"

static const char *TAG = "LCD";
static char buffer[17] = {0};
void i2c_lcd_init(void) {
static i2c_lcd1602_info_t lcd_handle;


    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    
    
    ESP_LOGI(TAG, "I2C master bus initialized successfully. \n");

    i2c_lcd1602_info_t lcd_handle = {
        .i2c_port = I2C_MASTER_NUM,
        .address = LCD_I2C_ADDRESS,
        .num_rows = 2,
        .num_columns = 16,
        .backlight = true
    };

    ESP_ERROR_CHECK(i2c_lcd1602_init(&lcd_handle));
    ESP_LOGI(TAG, "LCD1602 initialized successfully. ");

    }

void set_backlight(int level) {
    
    if (level == 1) {
        i2c_lcd1602_set_backlight(&lcd_handle, true);
    } else {
        i2c_lcd1602_set_backlight(&lcd_handle, false);
    }
}

void lcd_write(const char* text, int row, int col) {

    snprint(buffer, sizeof(buffer), "%s", text);

    i2c_lcd1602_move_cursor(&lcd_handle, row, col);
    i2c_lcd1602_write_string(&lcd_handle, buffer);

}

void lcd_clear(void) {
    i2c_lcd_clear(&lcd_handle);
}
