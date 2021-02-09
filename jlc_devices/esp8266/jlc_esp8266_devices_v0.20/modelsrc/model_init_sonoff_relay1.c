// model_init_sonoff_relay1.c


void ICACHE_FLASH_ATTR model_init()
{
        int gpio;
	int i=0;

        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);

        // Setup buttons as inputs
	gpio_relay[0]  = RELAY_1;
	gpio_button[0] = BUTTON_1;

        // Remove any alternate function from these pins so they are gpio outputs
	PIN_FUNC_SELECT(pin_mux[RELAY_1], pin_func[RELAY_1]);
        PIN_FUNC_SELECT(pin_mux[wifi_led], pin_func[wifi_led]);

	// Setup button as input
	gpio=BUTTON_1;
        PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
        GPIO_DIS_OUTPUT(gpio);
        PIN_PULLUP_EN(pin_mux[gpio]);

	button_state[0]=0;
	push_duration[0]=0;									// button is not being pushed
	relay_state[0]=0;
	forceoffseconds[0]=0;

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
	jcp_register_dev(0, DEVN_SWITCHPBT, 0, "0000000000000000");				// Register a switch with JCP protocol
	jcp_register_dev(1, DEVN_RELAY, 0, "0000000000000000");

	//void ICACHE_FLASH_ATTR yafdp_server_init(char *manufacturer, char *modelname, char *device_description, char *location)
	yafdp_server_init("Sonoff", "Sonoff Basic", "Mains relay and push button", (char*)&cfg.location);
	yafdp_start_udp_server();

	os_timer_setfn(&fast_timer,(os_timer_t *)fast_timer_cb,NULL);
	os_timer_arm(&fast_timer,20,1);								// 50 Hz
}

