// model_init_sonoff_relay_dual.c

void ICACHE_FLASH_ATTR model_init()
{
	int idx;
	int gpio;

	gpio_relay[0]	= RELAY_1;
	gpio_relay[1]	= RELAY_2;
	gpio_button[0]	= BUTTON_1;
	forceoffseconds[0]=0;
	forceoffseconds[1]=0;

	// Remove any alternate function from these pins so they are gpio outputs
	PIN_FUNC_SELECT(pin_mux[RELAY_1], pin_func[RELAY_1]);
	PIN_FUNC_SELECT(pin_mux[RELAY_2], pin_func[RELAY_2]);
	PIN_FUNC_SELECT(pin_mux[wifi_led], pin_func[wifi_led]);

	// Setup button as input
	gpio=BUTTON_1;
	PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
	GPIO_DIS_OUTPUT(gpio);
	PIN_PULLUP_EN(pin_mux[gpio]);


	//void ICACHE_FLASH_ATTR yafdp_server_init(char *manufacturer, char *modelname, char *device_description, char *location)
	yafdp_server_init("Sonoff", "Relay dual", "2xMains relay, 1xPush button", (char*)&cfg.location);
	yafdp_start_udp_server();

	// One push button, 2 relays
	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
	jcp_register_dev(0, DEVN_SWITCHPBT, 0, "0000000000000000"); 
	jcp_register_dev(1, DEVN_RELAY, 0, "0000000000000000");	
	jcp_register_dev(2, DEVN_RELAY, 0, "0000000000000000");

	os_timer_setfn(&fast_timer,(os_timer_t *)fast_timer_cb,NULL);
	os_timer_arm(&fast_timer,(1000/50),1);							// 50 Hz
}

