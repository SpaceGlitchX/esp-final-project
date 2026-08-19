#ifndef UI_H
#define UI_H

#include <stdint.h>

#include "driver/gpio.h"


/* ============================================================
 * BUTTON PINOUT
 * ============================================================ */

#define SET_UP GPIO_NUM_4
#define SET_DWN GPIO_NUM_0
#define SEL_UP GPIO_NUM_2
#define SEL_DWN GPIO_NUM_15


/* ============================================================
 * FUNCTIONS
 * ============================================================ */

void ui_init(void);

#endif