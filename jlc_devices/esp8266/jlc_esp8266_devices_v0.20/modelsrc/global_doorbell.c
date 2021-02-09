// user_init_doorbell.c

#define PUSH_MIN        10                              // length of time before a button push changes button state
#define BEEP_TIME       340                             // button held down this long, counting up at 100Hz

#ifdef DOORBELL
	//#define WIFI_LED	13			// low = on
	#define UHF_RF		2			// 433 Mhz UHF receiver maybe ??, possibly link to another CPU, unclear
	#define BUTTON_1	0			// Active low
	#define RELAY_1		12			// high=on
	#pragma message ( "Building for DOORBELL" )
        int b1state=0;                                  // off
        int noc=1;
#endif


// LEDs
int wifi_led = 5;


int oled_saver_interval=0;
int push_duration[4];                                           // How long a button is held
int button_state[4];                                            // [0]=button1 etc
int pbutton_state[4];                                           // previous button state
//int online=FALSE;
struct espconn jpixel_conn;
static esp_udp jpixel_udp;
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
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}


void ICACHE_FLASH_ATTR connected()
{
        build_send_regiserdevs();
}



// 100 Hz
// GPIO goes low only for as long as finger is held on the button
void fast_timer_cb(void *arg)
{
	//int i;
	int b=-1;
	char st[16];

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

	if (button_state[0] != pbutton_state[0])				// button changed state ?
	{
		if (button_state[0]==1)
		{
			output_high(RELAY_1);					// Relay is on
		}
		else	
		{
			output_low(RELAY_1);					// Relay is off
		}
		pbutton_state[0]=button_state[0];
	}
	// Holding any button down for or 4 seconds gives a beep, lets use that to cancel PIRs
	if (push_duration[0]==BEEP_TIME)					// Unit went "beep"
	{
		jcp_send_device_state(0, button_state[0], 1, 0, "", 0);		// long push
	}
}



