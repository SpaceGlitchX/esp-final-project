#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#pragma once

/*  USER-INTERFACE HEADER
Covers buttons and the LCD screen. Also covers user-related data.
*/

//  Pushbutton inputs
#define SET_UP GPIO_NUM_4
#define SET_DWN GPIO_NUM_0
#define SEL_UP GPIO_NUM_2
#define SEL_DWN GPIO_NUM_15
//  LCD OUTPUTS
#define CONT GPIO_NUM_25    // Output toward LCD contast pin, needs DAC
#define LCD_RS GPIO_NUM_19  // Register Select pin: 0 means means incoming data is a command, 1 means it's character data
#define LCD_E GPIO_NUM_21   // Enable pin: when pulsed LOW from default HIGH, LCD will record incoming data to memory
#define LCD_D7 GPIO_NUM_18  // LCD Data pin 7
#define LCD_D6 GPIO_NUM_5   // LCD Data pin 6
#define LCD_D5 GPIO_NUM_17  // LCD Data pin 5
#define LCD_D4 GPIO_NUM_16  // LCD Data pin 4