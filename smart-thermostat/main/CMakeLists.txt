idf_component_register(
                        SRCS 
                            "thermo_ui.h"
                            "LCD.h"
                            "thermo_sensors.h"
                            "LCD.c"
                            "thermo_sensors.c"
                            "ui.c"
                            "ui.h"
                        REQUIRES "driver" "esp_driver_gpio" "esp_adc" "esp_timer" "esp_driver_ledc"

                            
                        INCLUDE_DIRS ".")
