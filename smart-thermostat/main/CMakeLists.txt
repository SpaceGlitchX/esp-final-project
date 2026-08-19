idf_component_register(
	SRCS
		"thermo_sensors.c"
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