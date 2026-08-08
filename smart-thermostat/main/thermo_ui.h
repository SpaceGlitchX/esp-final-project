#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#pragma once

/*  USER-INTERFACE HEADER
Covers buttons and the LCD screen. Also covers user-related data.
*/

/*  BUTTON INPUTS
Buttons will be arranged in 4 cardinal directions.
SEL buttons are right & left. They increment through different LCD "views"
SET buttons are up & down. They increment & decrement the selected setting of the current "view"
*/
#define SET_UP GPIO_NUM_4   // Increment selected setting
#define SET_DWN GPIO_NUM_0  // Decrement selected setting
#define SEL_UP GPIO_NUM_2   // Increment through LCD views
#define SEL_DWN GPIO_NUM_15 // Decrement through LCD views
//  LCD OUTPUTS
#define CONT GPIO_NUM_25    // Output toward LCD contast pin, needs DAC
#define LCD_RS GPIO_NUM_19  // Register Select pin: 0 means means incoming data is a command, 1 means it's character data
#define LCD_E GPIO_NUM_21   // Enable pin: when pulsed LOW from default HIGH, LCD will record incoming data to memory
#define LCD_D7 GPIO_NUM_18  // LCD Data pin 7
#define LCD_D6 GPIO_NUM_5   // LCD Data pin 6
#define LCD_D5 GPIO_NUM_17  // LCD Data pin 5
#define LCD_D4 GPIO_NUM_16  // LCD Data pin 4

//  STRUCTS
/*LCD VIEW
Pushing the SEL buttons circulates the LCD between different screens - called "views"
Each view struct holds pointers to the relevant data and strings to display.
*/
typedef struct {
    void* setting;      // Data that should be manipulated when SET buttons are pressed
    void* data;         // Data that should be displayed (and not edited)
    char* top_text;     // Text for the top row
    char* bottom_text   // Text for the bottom row
} view;

//  GLOBALS

