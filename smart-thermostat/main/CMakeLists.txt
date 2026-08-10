idf_component_register(
                        SRCS 
                            "thermo_ui.h"
                            "thermo_ui.c"
                        REQUIRES "driver" "esp_driver_gpio" "esp_adc" "esp_timer" "esp_driver_ledc"

                            
                        INCLUDE_DIRS ".")
