idf_component_register(
    SRCS "components"
    "test_hvac_system.c"
    "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES spi_flash
    REQUIRES esp_driver_gpio esp_timer esp_driver_pcnt "components"
)
