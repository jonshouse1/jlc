// model_init_splotlight_1ch.c

void ICACHE_FLASH_ATTR model_init()
{
	//PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U0TXD_BK);					// Put both uarts on the same pin

        uint32 io_info[PWM_CHANNELS][3] =       {{PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM}};
	uint32_t duty[1] = {0};
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);                                    // set as GPIO output   
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	pwm_init(3000, duty, 1, io_info);
	pwm_start();

	os_printf("%s\n",fwver);

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
	jcp_register_dev(0, DEVF_LIGHT, 1, "0000000000000000");					// Register one "light" with 1 channel

        yafdp_server_init("JONSHOUSE", "SPOT-PWM1", "Mains spotlight", (char*)&cfg.location);
        yafdp_start_udp_server();

	init_udprx();
}


