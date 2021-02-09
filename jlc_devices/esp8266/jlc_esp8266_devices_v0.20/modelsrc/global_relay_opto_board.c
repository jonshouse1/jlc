// global_relay_opto_board.c

// LEDs
int wifi_led = 13;

#pragma message ( "Building for OPTO_RELAY_BAORD" )



int gpio_pir=5;
int gpio_relay=4;
int gpio_pirled=-1;	 								// set negative to disable
int pir_state=0;
int ppir_state=-1;

int relay_state[4];
int prelay_state[4];


struct espconn conn1;
static esp_udp udpsd;
static volatile os_timer_t fast_timer;


// Prepare to flash device by stopping all timers
void ICACHE_FLASH_ATTR app_suspend_timers()
{
}


// Server is sending us new state.  Idx is the index used when regisering the device. 
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
	if (idx==-100)								// devices are registered now
		return;
	os_printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);

        if (idx==1)                                                                     // index 1 is relay
        {
                if (ds->value1>0)
                        relay_state[0]=1;
                else    relay_state[0]=0;
	}
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}


void ICACHE_FLASH_ATTR connected()
{
	build_send_regiserdevs();
}



// 50 Hz
// This device has an opto iscolated input, the input is low when the PIR is triggered (and the LED on the board is on)
void fast_timer_cb(void *arg)
{
	int b=-1;
	static int oncount=0;

	pir_state=GPIO_INPUT_GET(gpio_pir);
	if (pir_state != ppir_state)						// Evrery time PIR changes state
	{
		if (pir_state == 0)						// PIR is triggered
		{
			if (gpio_pirled>=0)					// show PIR state with an LED ?
				output_low(gpio_pirled);			// low=on
			os_printf("PIR is ON\n");
		}
		else
		{
			if (gpio_pirled>=0)
				output_high(gpio_pirled);
			os_printf("PIR is OFF\n");
		}
		jcp_send_device_state(0, !pir_state, 0, 1, "", 0);		// Update the server 
		ppir_state = pir_state;
	}


	if (pir_state==0)							// GPIO is high, pir is triggered ?
	{
		oncount++;							// count up
	}
	else	oncount=0;


	// When walking around in front of a sensor it will come on, and then stay on as it is re-triggered
	// Keep re-running the script on the server every few seconds for as long as the PIR is on, this keeps any
	// countdown timers on the server refreshed.
	// server logic is "keep light on while PIR is triggered, then for a time after"
	if (oncount> 50 * 2)							// been on for two seconds?
	{
		jcp_send_device_state(0, !pir_state, 0, 0, "", 0);		// Update the server 
		oncount=0;
	}


	if (relay_state[0]!=prelay_state[0])
	{
		os_printf("Relay changed state to %d\n",relay_state[0]);
		prelay_state[0]=relay_state[0];
		if (relay_state[0]==0)
		{
			output_low(gpio_relay);
		}
		else	
		{
			output_high(gpio_relay);
		}
	}
}



