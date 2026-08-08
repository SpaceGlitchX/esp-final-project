#ifndef HVAC_HARDWARE_H
#define HVAC_HARDWARE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "hvac_states.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Hardware pins
#define HEATER_PIN 21
#define FAN_PIN 22

// Queue and timer handles
extern QueueHandle_t hvac_queue;
extern TimerHandle_t flame_proving_timer;
extern TimerHandle_t fan_warmup_timer;
extern TimerHandle_t tach_window_timer;

// function handles
void init_hvac_hardware(void);
void set_heater_state(int level);
void set_fan_state(int level);

#endif