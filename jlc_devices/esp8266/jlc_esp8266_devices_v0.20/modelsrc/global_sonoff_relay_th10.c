/*	global_sonoff_relay_th10.c

	Note:
	All DS18B20 ROM id's start 0x28, so we can shorten 8 bytes to 7
	and register as a UID for the system.
*/


#define PUSH_MIN	3							// length of time before a button push changes button state
#define BEEP_TIME	170							// button held down this long

#define BUTTON_1	0							// Active low
#define RELAY_1		12							// high=on
#define ONEWIREGPIO	14							// GPIO for DS18B20 temp sensor
// LEDs
int wifi_led = 13;


#pragma message ( "Building for SONOFF RELAY TH10" )

int gpio_relay[5];
int gpio_button[5];

int push_duration[4];								// How long a button is held
int button_state[4];								// [0]=button1 etc
int relay_state[4];
int prelay_state[4];
uint32_t forceoffseconds[4];

struct tsensors         ts[MAX_TS];
int num_ts=0;									// number of temperature sensors attached
int report_interval_seconds=30;

struct espconn conn1;
static esp_udp udpsd;

static volatile os_timer_t fast_timer;
static volatile os_timer_t app_timer;


// Prepare to flash device by stopping all timers
void ICACHE_FLASH_ATTR app_suspend_timers()
{
}

void ICACHE_FLASH_ATTR sample_temps()
{
	int i=0;

	num_ts=do_ds18b20(ONEWIREGPIO);
	wdt_feed();
	os_printf("Found %d temp sensors\n",num_ts);
	for (i=0;i<num_ts;i++)
	{
		os_printf("\tDS18B20  ROM:%02X%02X%02X%02X%02X%02X%02X%02X  = %s\n",
		ts[i].rom[0], ts[i].rom[2], ts[i].rom[2], ts[i].rom[3],
		ts[i].rom[4], ts[i].rom[5], ts[i].rom[6], ts[i].rom[7], ts[i].atemp);
	}
}



void ICACHE_FLASH_ATTR sample_and_send()
{
	int i=0;
	sample_temps();
	for (i=0;i<num_ts;i++)
	{
		wdt_feed();
		jcp_send_device_state(2+i, 101, 0, 1, (char*)&ts[i].atemp, strlen(ts[i].atemp));
	}
}

// Server is sending us new state.  idx is the index used when regisering the device. 
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
        if (idx==-100)										// devices are registered now
	{
		sample_and_send();								// send first reading now
		return;
	}
	os_printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);

	if (idx==0)										// index 0 is button
		button_state[0]=ds->value1;

	if (idx==1)										// index 1 is relay
	{
		if (ds->value1>0)
        		relay_state[0]=1; 
		else	relay_state[0]=0;
		forceoffseconds[0]=ds->value2;
	}

	if (idx>=2)										// index 2+ temp sensors
	{
		report_interval_seconds=ds->value1;
		if (report_interval_seconds<=0)
			report_interval_seconds=15;						// use default if daft
		os_printf("report_interval_seconds changed to %d\n",report_interval_seconds);
	}
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}



// This is used to turn a float into text.  The os_sprintf() function does not currently work with floats
// This is typically used for temperature, the output is fixed position.
// Examples:
//      -1.1    Gives   -01.1
//      -11.1           -11.1
//      1.1             +01.1
//      11.1            +11.1
void ICACHE_FLASH_ATTR printFloat(float val, char *buff)
{
	char smallBuff[16];
	int val1 = (int) val;
	unsigned int val2;
	if (val < 0)
	{
		val2 = (int) (-100.0 * val) % 100;
		//os_sprintf(smallBuff, "%03d.%u", val1, val2);
		os_sprintf(smallBuff, "%03d.%01u", val1, val2/10);
		smallBuff[0]='-';
	}
	else
	{
		val2 = (int) (100.0 * val) % 100;
		//os_sprintf(smallBuff, "+%02d.%u", val1, val2);
		os_sprintf(smallBuff, "+%02d.%01u", val1, val2/10);
	}
	//strcat(buff, smallBuff);                   // Append
	strcpy(buff, smallBuff);                     // replace
}




// 50 Hz
// GPIO goes low only for as long as finger is held on the button
void fast_timer_cb(void *arg)
{
	//int i;
	int b=-1;
	char st[16];
	if (forceoffseconds[0]>0)                                               // counter active ?
	{
		forceoffseconds[0]--;
		if (forceoffseconds[0]==0)                                      // counter just reached 0 ?
		{
			relay_state[0]=0;                                       // relay is now off
			os_printf("relay forced to off by timer\n");
			jcp_send_device_state(1, relay_state[0], 0, 0, "", 0);  // update server
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
		jcp_send_device_state(0, button_state[0], 1, 0, "", 0);		// long push
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





// 10Hz
void ICACHE_FLASH_ATTR app_timer_cb(void *arg)
{
	static int fc=0;									// fast counter
	static int sc=0;									// seconds counter

	fc++;
	if (fc>10)
	{
		fc=0;
		sc++;										// count in seconds
		os_printf("report_interval_seconds = %d\tsc=%d\n",report_interval_seconds, sc);
	}

	if (sc>=report_interval_seconds)
	{
		sample_and_send();
		sc=0;
	}
}



