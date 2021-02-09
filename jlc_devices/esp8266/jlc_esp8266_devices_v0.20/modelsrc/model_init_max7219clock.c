// model_init_max7219clock.c

void ICACHE_FLASH_ATTR model_init()
{
        int c;
        int gpio;

        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);                                    // set as GPIO output   
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);

	bzero(&popupmsg_string,sizeof(popupmsg_string));
	bzero(&disp,sizeof(disp));
	update_display();

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ); 
	jcp_register_dev(0, DEVN_CLOCKCAL, 0, "0000000000000001");				// JCP_TOPIC_DATETIME_S_M

        //void ICACHE_FLASH_ATTR yafdp_server_init(char *manufacturer, char *modelname, char *device_description, char *location)
        yafdp_server_init("JONSHOUSE", "MAX7219CLOCK", "MAX7219 Clock", (char*)&cfg.location);
        yafdp_start_udp_server();

        max_setgpio(4, 0, 5);                                                                   // Data, Clock, Chipsel
        max_setgpio(4, 0, 5);                                                                   // Try and make it more reliable
        max_init(5);                                                                            // Initialse 4 controllers
        os_delay_us(1000);
        max_init(5);                                                                            // Do it again, not always reliable

	os_timer_setfn(&slow_timer,(os_timer_t *)slow_timer_cb,NULL);
        os_timer_arm(&slow_timer,500,1);
}


