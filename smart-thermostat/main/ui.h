#ifndef UI_H
#define UI_H

#include "LCD.h"

#define SET_UP GPIO_NUM_4
#define SET_DWN GPIO_NUM_0
#define SEL_UP GPIO_NUM_2
#define SEL_DWN GPIO_NUM_15

typedef struct {
    void* setting;
    void* data;
    char* top_text;
    char* bottom_text;
} view;

typedef struct {
    
}
extern void ui_init(void);


#endif