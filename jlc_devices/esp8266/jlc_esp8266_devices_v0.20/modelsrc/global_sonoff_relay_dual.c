// global_sonoff_relay_dual.c
/*
	For sonoff dual R2 PCB

	https://github.com/arendst/Sonoff-Tasmota/issues/1249

	GPIO0 markred "button0" on J1 header
	GPIO 9 marked "button1" on J1 header
	GPIO12 first relay,   GPIO5 second relay,  GPIO13 LED
	GPIO10 button on PCB
*/

#define PUSH_MIN        4				// length of time before a button push changes button state
#define BEEP_TIME       200				// button held down this long, counting up at 100Hz

#define BUTTON_1	10
#define RELAY_1		12
#define RELAY_2		5

// LEDs
int wifi_led = 13;


#pragma message ( "Building for SONOFF RELAY DUAL" )
int push_duration[5];								// How long a button is held
int button_state[5];								// [0]=button1 etc

int gpio_relay[5];
int gpio_button[5];
int relay_state[5];
int prelay_state[5];
uint32_t forceoffseconds[5];

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
        if (idx==-100)										// devices are registered now
		return;
	os_printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);

        if (idx>=0 && idx<=3)									// index 0 to 3 are buttons ?
		button_state[idx]=ds->value1;							// invisible to user, but still important

	if (idx>=1 && idx<=2)									// index is our relay devices ?
	{
                if (ds->value1>0)
                        relay_state[idx-1]=1; 
                else	relay_state[idx-1]=0;
        }
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}



// Poll button, values start at 0 and must matched index number used by jcp_register_dev
void poll_button(int button)
{
	int b=-1;
	char st[16];

	b=GPIO_INPUT_GET(gpio_button[button]);
	if (b==0)								// finger is held on button ? 
	{
		push_duration[button]++;					// counting up at 100Hz as button is held
		if (push_duration[button]==PUSH_MIN)				// change state once only when held for a few 10 of ms
		{
			button_state[button]=!button_state[button];		// toggle
			jcp_send_device_state(button, button_state[button], 0, 0, "", 0);
		}
	}
	else push_duration[button]=0;						// finger not on button, clear counter

	// Holding any button down for or 4 seconds gives a beep, lets use that to cancel PIRs
	if (push_duration[button]==BEEP_TIME)					// Unit went "beep"
	{
		jcp_send_device_state(button, button_state[button], 1, 0, "", 0);	// long push
	}
}



// 50 Hz
// GPIO goes low only for as long as finger is held on the button
void fast_timer_cb(void *arg)
{
	int i=0;

	for (i=0;i<2;i++)
	{
		poll_button(0);
		if (relay_state[i] != prelay_state[i])				// relay state changed ?
		{
			if (relay_state[i]==1)
			{
				output_high(gpio_relay[i]);
				os_printf("RELAY %d state %d (ON)\n",i,relay_state[i]);
			}
			else
			{
				output_low(gpio_relay[i]);
				os_printf("RELAY %d state %d (OFF)\n",i,relay_state[i]);
			}
			prelay_state[i] = relay_state[i];
		}
	}
}




void ICACHE_FLASH_ATTR connected()
{
	build_send_regiserdevs();
}


