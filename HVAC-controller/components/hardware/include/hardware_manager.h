#ifndef HVAC_HARDWARE_H
#define HVAC_HARDWARE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "hvac_states.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Hardware pins
#define HEATER_PIN 18
#define FAN_PIN 25

// Queue and timer handles
extern TimerHandle_t flame_proving_timer;
extern TimerHandle_t fan_warmup_timer;
extern TimerHandle_t tach_window_timer;
extern QueueHandle_t hvac_queue;

// function handles
void init_hvac_hardware(void);
void set_heater_state(int level);
void set_fan_state(int level);
int get_fan_state(void);
int get_heater_state(void);

#endif