idf_component_register(
                        SRCS 
                            "main.c"
                            "thermo_sensors.h"
                            "thermo_ui.h"
                            "thermo_ui.c"
                            "thermo_comms.h"
                        PRIV_REQUIRES 
                            spi_flash
                            esp_driver_dac
                            esp_timer
                            esp_driver_gpio
                            
                        INCLUDE_DIRS "")
