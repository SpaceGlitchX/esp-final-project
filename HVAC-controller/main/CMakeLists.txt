idf_component_register(
    SRCS "tachometer.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES spi_flash
    REQUIRES esp_driver_gpio esp_timer esp_driver_pcnt
)
