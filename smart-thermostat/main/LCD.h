#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "lcd1602.h"

#define I2C_MASTER_SCL_IO           22                // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           21                // GPIO number for I2C master data
#define I2C_MASTER_NUM              I2C_NUM_0         // I2C port number
#define I2C_MASTER_FREQ_HZ          100000            // I2C master clock frequency (100kHz)
#define LCD_I2C_ADDRESS             0x27  

extern char buffer[16];
extern void i2c_lcd_init(void);
extern void set_backlight(int level);
extern void lcd_write(const char* text, char* buffer, int row, int col);
extern void lcd_clear(void);

static const char *TAG = "i2c_lcd";
static i2c_lcd1602_info_t lcd_handle;
