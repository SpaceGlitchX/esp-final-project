#ifndef THERMO_LOGIC_H
#define THERMO_LOGIC_H

#include "thermo_sensors.h"
#include "user_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

double setpoint;
void set_setpoint(int level);
#endif