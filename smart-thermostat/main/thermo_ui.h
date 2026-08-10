#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "driver/dac_oneshot.h"
//#include "hal/lcd_types.h"
//#include "esp_lcd_panel_io.h"
//#include "esp_lcd_panel_ops.h"
//#include "driver/i2c_master.h"
//#include "driver/ledc.h"
#include "freertos/queue.h"

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
#define CONT GPIO_NUM_25    // Output toward LCD contrast pin, needs DAC
#define LCD_RS GPIO_NUM_19  // Register Select pin: 0 means means incoming data is a command, 1 means it's character data
#define LCD_E GPIO_NUM_21   // Enable pin: when pulsed LOW from default HIGH, LCD will record incoming data to memory
#define LCD_D7 GPIO_NUM_18  // LCD Data pin 7
#define LCD_D6 GPIO_NUM_5   // LCD Data pin 6
#define LCD_D5 GPIO_NUM_17  // LCD Data pin 5
#define LCD_D4 GPIO_NUM_16  // LCD Data pin 4


//  STRUCTS
/*  LCD VIEW
Pushing the SEL buttons circulates the LCD between different screens - called "views"
Each view struct holds pointers to the relevant data and strings to display.
Keep in mind that the LCD character grid is 16x2
*/
typedef struct {
    void* setting;      // Data that should be manipulated when SET buttons are pressed
    void* data;         // Data that should be displayed (and not edited)
    char* top_text;     // Text for the top row
    char* bottom_text   // Text for the bottom row
} view;

/*  PLANNED VIEWS ░▒▓

Empty template:
░░░░░░▒░░▒▒▒▒▒▒▒▒▒▓▓
░ ________________ ▒
░ ________________ ░
░░░░░░░░░░░░░░░░▒▒░▒

View 0: Display
░░░░░░▒░░▒▒▒▒▒▒▒▒▒▓▓
▒ 00.0__00.0__00.0 ▒
▒ OUT___IN___TARG. ▒
░░░░░░░░░░░░░░░░▒▒░▒

View 1: Set Target
░░░░░░▒░░▒▒▒▒▒▒▒▒▒▓▓
░ Target Temp:____ ▒
░ ____±00.0_°C____ ░    The sign should change, don't actually use '±'
░░░░░░░░░░░░░░░░▒▒░▒

View 2: Set Contrast
░░░░░░▒░░▒▒▒▒▒▒▒▒▒▓▓
░ LCD Contrast:___ ▒
░ ______100%______ ░    Increment contrast on a % scale
░░░░░░░░░░░░░░░░▒▒░▒

View 3: Set Units
░░░░░░▒░░▒▒▒▒▒▒▒▒▒▓▓
░ Display Units:__ ▒
░ _______°C_______ ░    Units may be: °C, K (no °), °F, °R (or °Ra)
░░░░░░░░░░░░░░░░▒▒░▒
*/

//  GLOBALS

const dac_oneshot_config_t dac_config;
static dac_oneshot_handle_t dac_handle = DAC_CHAN_0;
const uint8_t dac_high = 255;           // maximum value to be written to the DAC (the LCD contrast pin, pin 25)

TimerHandle_t timer_handle;
QueueHandle_t isr_handle;

const gpio_num_t lcd_data_pins[4] = {LCD_D4, LCD_D5, LCD_D6, LCD_D7};
static uint8_t bmap = 0000 & 0xFF; // each button state is recorded in a bit; &0xFF simply maps the 4-bit (nibble) variable to a full byte


//  FUNCTIONS

void lcd_setup(void);
void button_setup(void);

void timerCallback(TimerHandle_t timer);

static void IRAM_ATTR isr_trig(void *param);
static void IRAM_ATTR set_dwn_isr(void *param);
static void IRAM_ATTR sel_up_isr(void *param);
static void IRAM_ATTR sel_dwn_isr(void *param);