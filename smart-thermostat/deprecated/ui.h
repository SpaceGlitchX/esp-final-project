#ifndef UI_H
#define UI_H

#include "driver/gpio.h"


/* Button pins */

#define TEMP_UP		GPIO_NUM_4
#define TEMP_DOWN	GPIO_NUM_0
#define POWER		GPIO_NUM_2
#define FAN_MODE	GPIO_NUM_15


void ui_init(void);


#endif