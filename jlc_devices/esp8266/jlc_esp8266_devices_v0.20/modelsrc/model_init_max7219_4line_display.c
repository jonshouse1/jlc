// model_init_max7219_4line_display.c


void ICACHE_FLASH_ATTR model_init()
{
        int c;
        int gpio;
	int i;

PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12); // Set GPIO12 function
PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); // Set GPIO13 function
PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14); // set GPIO14 function
PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); // set GPIO15 function



	bzero(&popupmsg_string,sizeof(popupmsg_string));
	bzero(&disp,sizeof(disp));
	update_display(0);

	jcp_init(JCP_SERVER_PORT+1, JLP_SERVER_PORT+1, 1000/JCP_TIMER_FREQ_HZ); 
	//jcp_register_dev(0, DEVN_CLOCKCAL, 0, "0000000000000001");				// JCP_TOPIC_DATETIME_S_M
	jcp_register_dev(0, DEVN_LED_SIGN, 0, "0000000000000000");

        //void ICACHE_FLASH_ATTR yafdp_server_init(char *manufacturer, char *modelname, char *device_description, char *location)
        //yafdp_server_init("JONSHOUSE", "MAX7219CLOCK", "MAX7219 Clock", (char*)&cfg.location);
        yafdp_server_init("JONSHOUSE", "MAX72194LDISPLAY", "MAX7219 4ln display", (char*)&cfg.location);
        yafdp_start_udp_server();

	init_displays();
	//for (i=0;i<=3;i++)
	//{
		//select_module(i);
        	//max_init(4);									// Initialse 4 controllers
	//}

	//void display_text(int l,char *st, int moveright)
	display_text(3,"Four",0);
	display_text(2,"line",0);
	display_text(1,"LED",0);
	display_text(0,"sign",0);


	os_timer_setfn(&slow_timer,(os_timer_t *)slow_timer_cb,NULL);
        os_timer_arm(&slow_timer,500,1);
}


