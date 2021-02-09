// model_init_oled_tempsens.c




void ICACHE_FLASH_ATTR model_init()
{
        int gpio;
	int i=0;

        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);

        // Setup buttons as inputs
	//gpio_relay[0]  = RELAY_1;
	//gpio_button[0] = BUTTON_1;

        // Remove any alternate function from these pins so they are gpio outputs
	//PIN_FUNC_SELECT(pin_mux[RELAY_1], pin_func[RELAY_1]);
        //PIN_FUNC_SELECT(pin_mux[wifi_led], pin_func[wifi_led]);

	// Setup button as input
	//gpio=BUTTON_1;
        //PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
        //GPIO_DIS_OUTPUT(gpio);
        //PIN_PULLUP_EN(pin_mux[gpio]);

	//button_state[0]=0;
	//push_duration[0]=0;									// button is not being pushed
	//relay_state[0]=0;

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
        sample_temps();
        for (i=0;i<num_ts;i++)                                                                  // for every temp sensor found
        {
		if (i==0)
			jcp_register_dev(i, DEVN_TEMPSENS, 0, "1000000000000001");						// register the device with the default UID
		else	jcp_register_dev(i, DEVN_TEMPSENS, 0, "0000000000000000");		// register the device with the default UID
		jcp_set_dev_uid(i, (unsigned char*)&ts[i].rom[1]);				// override the UID with the last 7 bytes of DS18B20 UID
        }

	//void ICACHE_FLASH_ATTR yafdp_server_init(char *manufacturer, char *modelname, char *device_description, char *location)
	yafdp_server_init("jonshouse", "oled-tempsens", "Temperature Sensor with O-LED display", (char*)&cfg.location);
	yafdp_start_udp_server();

	i2c_init();
    	OLED_Init();
	OLED_Print(0,0,"O-LED TEMPSENS",1);

	os_timer_setfn(&fast_timer,(os_timer_t *)fast_timer_cb,NULL);
	os_timer_arm(&fast_timer,100,1);							// 10 Hz

}

