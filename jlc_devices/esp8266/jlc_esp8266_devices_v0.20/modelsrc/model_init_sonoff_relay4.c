// model_init_sonoff_relay4.c

void ICACHE_FLASH_ATTR model_init()
{
	int idx;
	int gpio;

	gpio_relay[0]	= RELAY_1;
	gpio_relay[1]	= RELAY_2;
	gpio_relay[2]	= RELAY_3;
	gpio_relay[3]	= RELAY_4;
	gpio_button[0]	= BUTTON_1;
	gpio_button[1]	= BUTTON_2;
	gpio_button[2]	= BUTTON_3;
	gpio_button[3]	= BUTTON_4;

	// Remove any alternate function from these pins so they are gpio outputs
	PIN_FUNC_SELECT(pin_mux[RELAY_1], pin_func[RELAY_1]);
	PIN_FUNC_SELECT(pin_mux[RELAY_2], pin_func[RELAY_2]);
	PIN_FUNC_SELECT(pin_mux[RELAY_3], pin_func[RELAY_3]);
	PIN_FUNC_SELECT(pin_mux[RELAY_4], pin_func[RELAY_4]);
	PIN_FUNC_SELECT(pin_mux[wifi_led], pin_func[wifi_led]);

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
	for (idx=0;idx<4;idx++)									// 0,1,2,3
	{
		// Setup buttons as inputs
		gpio=gpio_button[idx];
		PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
		GPIO_DIS_OUTPUT(gpio);
		PIN_PULLUP_EN(pin_mux[gpio]);

		// init button and relay initial state
		button_state[idx]  = 0;
		relay_state[idx]   = 0;
		prelay_state[idx]  =-1;
		push_duration[idx] = 0;
		forceoffseconds[idx]=0;

		// Register device with server, buttons are idx 0,1,2 and 3
		jcp_register_dev(idx, DEVN_SWITCHPBT, 0, "0000000000000000");
	}

	//void ICACHE_FLASH_ATTR yafdp_server_init(char *manufacturer, char *modelname, char *device_description, char *location)
	yafdp_server_init("Sonoff", "Relay 4CH", "4xMains relay, 4xPush button", (char*)&cfg.location);
	yafdp_start_udp_server();

	jcp_register_dev(4, DEVN_RELAY, 0, "0000000000000000");					// Register relays as idx 4,5,6,7
	jcp_register_dev(5, DEVN_RELAY, 0, "0000000000000000");
	jcp_register_dev(6, DEVN_RELAY, 0, "0000000000000000");
	jcp_register_dev(7, DEVN_RELAY, 0, "0000000000000000");

	os_timer_setfn(&fast_timer,(os_timer_t *)fast_timer_cb,NULL);
	os_timer_arm(&fast_timer,20,1);                                                         // 50 Hz
}

