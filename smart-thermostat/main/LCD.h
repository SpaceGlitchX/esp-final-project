#ifndef LCD_H
#define LCD_H
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_SCL_IO           22                // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           21                // GPIO number for I2C master data
#define I2C_MASTER_NUM              I2C_NUM_0         // I2C port number
#define I2C_MASTER_FREQ_HZ          100000            // I2C master clock frequency (100kHz)
#define LCD_I2C_ADDRESS             0x27  

char buffer[16];
void i2c_lcd_init(void);
void set_backlight(int level);
void lcd_write(const char* text, int row, int col);
void lcd_clear(void);

#endif