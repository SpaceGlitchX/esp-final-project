/*
Based on the 1602 driver written by Aad van Gerwen
https://components.espressif.com/components/vgerwen/lcd1602
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/dac_oneshot.h"

#pragma once

//  LCD OUTPUTS
#define CONT GPIO_NUM_25    // Output toward LCD contrast pin, needs DAC
#define LCD_RS GPIO_NUM_19  // Register Select pin: 0 means means incoming data is a command, 1 means it's character data
#define LCD_E GPIO_NUM_21   // Enable pin: when pulsed LOW from default HIGH, LCD will record incoming data to memory
#define LCD_D7 GPIO_NUM_18  // LCD Data pin 7
#define LCD_D6 GPIO_NUM_5   // LCD Data pin 6
#define LCD_D5 GPIO_NUM_17  // LCD Data pin 5
#define LCD_D4 GPIO_NUM_16  // LCD Data pin 4

//  IDENTITIES
#define COMMAND 1
#define DATA 0
//  COMMANDS
#define CLEAR 0x01
#define LINE_1_FOREWARD 0x80
#define LINE_1_BACKWARD 0x60
#define LINE_2_FOREWARD 0x40
#define LINE_2_BACKWARD 0x20

//  TYPES & STRUCTS

typedef int error_t;    // Holds error state for functions

//  GLOBALS

//  FUNCTIONS
/*  TRIGGER LCD ENABLE PIN
    forces the E pin low, high, and then low again
    the LCD latches data on the falling edge of the pulse
*/
static void lcd_enable(void);
/*  SEND 4-BIT CHARACTER
    takes an array of 4 pins which will transmit the data
    also takes an unsigned char (an 8-bit character) but sends only the lower 4 bits
*/
static void send_nibble(gpio_num_t data, unsigned char c);
/*  LCD COMMAND
    Write a command to the LCD, given a character to send
*/
static void lcd_command(unsigned char command);
/*  LCD WRITE
    Write data to the LCD, given a character to send
*/
static void lcd_write_byte(unsigned char data);
/*  LCD INITIALIZE
    Set up LCD
    this function is heavily dervived from Aad van Gerwen's driver
*/
void lcd_init(void);