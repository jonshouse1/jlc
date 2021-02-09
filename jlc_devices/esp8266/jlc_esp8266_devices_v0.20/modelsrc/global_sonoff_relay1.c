// global_sonoff_relay1.c

#define PUSH_MIN	3							// length of time before a button push changes button state
#define BEEP_TIME	170							// button held down this long

#define BUTTON_1	0							// Active low
#define RELAY_1		12							// high=on
// LEDs
int wifi_led = 13;


#pragma message ( "Building for SONOFF RELAY1" )

int gpio_relay[5];
int gpio_button[5];

int push_duration[4];								// How long a button is held
int button_state[4];								// [0]=button1 etc
int relay_state[4];
int prelay_state[4];
uint32_t forceoffseconds[4];

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
        if (idx==-100)									// devices are registered now
		return;
	os_printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);
	if (idx==0)									// index 0 is button
		button_state[0]=ds->value1;

	if (idx==1)									// index 1 is relay
	{
		if (ds->value1>0)
        		relay_state[0]=1; 
		else	relay_state[0]=0;

		// Optional timer, if value2 is greater than zero then wait this many seconds and turn relay off
		if (ds->value2>0)
		{
			forceoffseconds[0]=ds->value2*50;
			os_printf("Relay will toggle back to off in %d seconds\n",ds->value2);
		}
	}
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}




// 50 Hz
// GPIO goes low only for as long as finger is held on the button
void fast_timer_cb(void *arg)
{
	//int i;
	int b=-1;
	char st[16];

	if (forceoffseconds[0]>0)						// counter active ?
	{
		forceoffseconds[0]--;
		if (forceoffseconds[0]==0)					// counter just reached 0 ?
		{
			relay_state[0]=0;					// relay is now off
			os_printf("relay forced to off by timer\n");
			jcp_send_device_state(1, relay_state[0], 0, 0, "", 0);	// update server
		}
	}

	b=GPIO_INPUT_GET(BUTTON_1);
	if (b==0)								// finger is held on button ? 
	{
		push_duration[0]++;						// counting up at 100Hz as button is held
		if (push_duration[0]==PUSH_MIN)					// change state once only when held for a few 10 of ms
		{
			button_state[0]=!button_state[0];			// toggle
			jcp_send_device_state(0, button_state[0], 0, 0, "", 0);
		}
	}
	else push_duration[0]=0;						// finger not on button, clear counter

	// Holding any button down for or 4 seconds gives a beep, lets use that to cancel PIRs
	if (push_duration[0]==BEEP_TIME)					// Unit went "beep"
	{
		//jcp_send_device_state(0, button_state[0], 1, "", 0);		// long push
		jcp_send_device_state(0, button_state[0], 0, 0, "", 0);
	}


 
	if (relay_state[0] != prelay_state[0])					// relay state changed ?
	{
		if (relay_state[0]==1)
		{
			output_high(gpio_relay[0]);
			os_printf("RELAY state %d (ON)\n",relay_state[0]);
		}
		else
		{
			output_low(gpio_relay[0]);
			os_printf("RELAY state %d (OFF)\n",relay_state[0]);
		}
		prelay_state[0] = relay_state[0];
	}
}




void ICACHE_FLASH_ATTR connected()
{
	build_send_regiserdevs();
}


