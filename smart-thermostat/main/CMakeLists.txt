idf_component_register(
                        SRCS 
                            "main.c"
                            "thermo_ui.h"
                            "thermo_ui.c"
                            "lcd_driver.h"
                            "lcd_driver.c"
                            "thermo_sensors.h"
                            "thermo_sensors.c"
                            "thermo_comms.h"
                            "thermo_comms.c"
                        PRIV_REQUIRES 
                            spi_flash
                            esp_driver_dac
                            esp_timer
                            esp_driver_gpio
                            esp_driver_i2c
                            esp_hal_i2c
                            esp_lcd
                            
                        INCLUDE_DIRS "")
