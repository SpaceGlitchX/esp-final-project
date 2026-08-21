idf_component_register(
	SRCS
		"temperature_sensor.c"
		"sensor_manager.c"
		"thermo_logic.c"
		"thermo_comms.c"
		"user_input.c"
		"main.c"

	INCLUDE_DIRS
		"include"

	REQUIRES
		esp_adc
		esp_driver_gpio
		esp_timer
		freertos
		console
		driver
		esp_driver_uart
)