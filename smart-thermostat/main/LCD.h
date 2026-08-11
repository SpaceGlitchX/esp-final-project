#ifndef LCD_H
#define LCD_H
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdint.h>

#define I2C_MASTER_NUM             I2C_NUM_0    // Built-in I2C peripheral port
#define I2C_MASTER_SDA_IO          21           // Default SDA Pin
#define I2C_MASTER_SCL_IO          22           // Default SCL Pin
#define I2C_MASTER_FREQ_HZ         100000       // 100kHz I2C clock speed
#define I2C_MASTER_TX_BUF_DISABLE  0            // Buffer allocation not needed for master
#define I2C_MASTER_RX_BUF_DISABLE  0            // Buffer allocation not needed for master

// Common native I2C address for AIP31068 / direct I2C LCD modules (Adjust if necessary)
#define LCD_I2C_ADDRESS            0x3E         

// Control byte signaling formats
#define LCD_CONTROL_COMMAND        0x00
#define LCD_CONTROL_DATA           0x40

// LCD Core Instructions
#define LCD_CMD_CLEAR_DISPLAY      0x01
#define LCD_CMD_RETURN_HOME        0x02
#define LCD_CMD_ENTRY_MODE_SET     0x06 // Increment cursor, no display shift
#define LCD_CMD_DISPLAY_CONTROL    0x0C // Display ON, Cursor OFF, Blink OFF
#define LCD_CMD_FUNCTION_SET       0x38 // 8-bit mode, 2-line display, 5x8 font

char buffer[16];
esp_err_t i2c_master_init(void);
void lcd_send_command(uint8_t cmd);
void lcd_send_data(uint8_t data);
void lcd_init(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_send_string(const char *str);

#endif