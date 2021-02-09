// model_init_t1_uk_3c.c

#pragma message ( "Building for SONOFF T1_UK_3C  - model_init_t1_uk_3c" )

void ICACHE_FLASH_ATTR model_init()
{
        int c;
        int gpio;

        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);

        // Setup buttons as inputs
        gpio=BUTTON_1;
        PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
        GPIO_DIS_OUTPUT(gpio);
        PIN_PULLUP_EN(pin_mux[gpio]);

        gpio=BUTTON_2;
        PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
        GPIO_DIS_OUTPUT(gpio);
        PIN_PULLUP_EN(pin_mux[gpio]);

        gpio=BUTTON_3;
        PIN_FUNC_SELECT(pin_mux[gpio], pin_func[gpio]);
        GPIO_DIS_OUTPUT(gpio);
        PIN_PULLUP_EN(pin_mux[gpio]);

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
        for (c=0;c<3;c++)
        {
                button_state[c]=0;
                pbutton_state[c]=-1;
                push_duration[c]=0;                                                             // button is not being pushed
                jcp_register_dev(c, DEVN_SWITCHPBT, 0, "0000000000000000");			// Register a switch with JCP protocol
        }
        os_timer_setfn(&fast_timer,(os_timer_t *)fast_timer_cb,NULL);                           // 100Hz
        os_timer_arm(&fast_timer,10,1);

	yafdp_server_init("Sonoff", "T1_UK_3C", "Mains touch switch", (char*)&cfg.location);
	yafdp_start_udp_server();
}

