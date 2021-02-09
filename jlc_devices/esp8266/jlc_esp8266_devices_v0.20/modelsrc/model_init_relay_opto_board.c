// model_init_relay_opto_board


void ICACHE_FLASH_ATTR model_init()
{
        int gpio;
	int i=0;

        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);

        // Setup buttons as inputs
	gpio=gpio_pir;
	PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
	GPIO_DIS_OUTPUT(gpio);
	PIN_PULLUP_EN(pin_mux[gpio]);

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
	jcp_register_dev(0, DEVN_MOVEMENTSENSOR, 0, "0000000000000000");			// Register one PIR
	jcp_register_dev(1, DEVN_RELAY, 0, "0000000000000000");	

	yafdp_server_init("JONSHOUSE", "RELAYOPTO", "Relay/Movement Sensor", (char*)&cfg.location);
	yafdp_start_udp_server();

relay_state[0]=0;

	os_timer_setfn(&fast_timer,(os_timer_t *)fast_timer_cb,NULL);
	os_timer_arm(&fast_timer,20,1);								// 50 Hz
}

