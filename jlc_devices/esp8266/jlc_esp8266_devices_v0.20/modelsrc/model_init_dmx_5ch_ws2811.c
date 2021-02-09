// model_init_dmx_5ch_ws2811.c


void ICACHE_FLASH_ATTR model_init()
{
        int c;
        int gpio;

        uint32 io_info[PWM_CHANNELS][3] =       {{PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM},
                                                {PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},
                                                {PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM},
                                                {PWM_3_OUT_IO_MUX,PWM_3_OUT_IO_FUNC,PWM_3_OUT_IO_NUM},
                                                {PWM_4_OUT_IO_MUX,PWM_4_OUT_IO_FUNC,PWM_4_OUT_IO_NUM}};
        uint32_t duty[1] = {0};
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);                                    // set as GPIO output   
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
        pwm_init(3000, duty, 5, io_info);
        pwm_start();

// Todo register DMX bridge, register neopixels 
	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ);
	jcp_register_dev(0, DEVF_LIGHT, 5, "0000000000000000");                                 // Register one "light" with 5 channels

        yafdp_server_init("JONSHOUSE", "DMX5CHWS", "DMX Bridge, 5 channel PWM, WS2811/2", (char*)&cfg.location);
        yafdp_start_udp_server();
}


