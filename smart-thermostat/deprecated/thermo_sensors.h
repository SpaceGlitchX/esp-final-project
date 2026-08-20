#ifndef THERMO_SENSORS_H
#define THERMO_SENSORS_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define SENSOR_PERIOD_MS 200

/*
 * Temperature data passed through the queue.
 */
typedef struct
{
    float temperature_c;
    int raw_adc;
    int voltage_mv;
    float resistance_ohm;
} TemperatureData;


/*
 * Initialize temperature sensor.
 */
void thermo_sensors_init(void);


/*
 * Get the temperature queue.
 *
 * Other modules can use this queue to receive
 * temperature measurements.
 */
QueueHandle_t thermo_temperature_queue(void);


/*
 * Get the most recent temperature without
 * modifying the queue.
 */
float get_indoor_temperature(void);

#endif