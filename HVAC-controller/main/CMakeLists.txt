idf_component_register(SRCS "tachometer.c" 
                           "components"
        

                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_gpio
                       esp_timer)
