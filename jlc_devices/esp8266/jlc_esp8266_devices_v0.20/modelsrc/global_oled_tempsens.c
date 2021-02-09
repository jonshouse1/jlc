// global_oled_tempsens.c

//#include "ts.h"

#define PUSH_MIN	3							// length of time before a button push changes button state
#define BEEP_TIME	170							// button held down this long

#define BUTTON_1	0							// Active low
#define RELAY_1		12							// high=on
#define ONEWIREGPIO     14                                                      // GPIO for DS18B20 temp sensor

// LEDs
int wifi_led = 13;


#pragma message ( "Building for OLED_TEMPSENS" )

//int gpio_relay[5];
//int gpio_button[5];

int report_interval_seconds=30;
//int push_duration[4];								// How long a button is held
//int button_state[4];								// [0]=button1 etc
//int relay_state[4];
//int prelay_state[4];

struct espconn conn1;
static esp_udp udpsd;

static volatile os_timer_t fast_timer;

// DS18B20 ROM contents and temperature readings
struct tsensors         ts [MAX_TS];
int num_ts=0;									// number of temperature sensors detected



char dis_temp[6];                                                                                                               // Sign plus first two digits
//char pdis_temp[6];
int  dis_tsidx=0;
//int  dis_changed=FALSE;
char dis_botline[24];
int display_ip_details_count=0;                                                                                                 // Non zero value is the time to display IP details
int display_title_and_build_count=0;


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
                jcp_send_device_state(0+i, 101, 0, 1, (char*)&ts[i].atemp, strlen(ts[i].atemp));
        }
}


// Prepare to flash device by stopping all timers
void ICACHE_FLASH_ATTR app_suspend_timers()
{
}


// Server is sending us new state.  Idx is the index used when regisering the device. 
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
        if (idx==-100)
	{
		sample_and_send();
		return;
	}
	os_printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);
 
	report_interval_seconds=ds->value1;
	if (report_interval_seconds<=0)
		report_interval_seconds=15;                                             // use default if daft
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
        char st[64];
	int i=0;
	int c=0;

        if (topic == JCP_TOPIC_STATUSLINE)
        {
		memcpy(&dis_botline, tdata+(tlen-14), 14);
		dis_botline[16]=0;
		os_printf("jcp_topic_cb() got new statusline [%s]\n",tdata);
        }

        if (topic == JCP_TOPIC_DATETIME_S_M)
        {
		memcpy(&dis_botline, tdata+(tlen-19), 16);
		dis_botline[17]=0;
		os_printf("jcp_topic_cb() got new datetimem len=%d [%s] botline=[%s]\n",tlen, tdata, dis_botline);
        }
}

void ICACHE_FLASH_ATTR connected()
{
	build_send_regiserdevs();
}




void ICACHE_FLASH_ATTR clear_ts_table()
{
        int i;

        for (i=0;i<MAX_TS;i++)                                                                                                  // for every possible temp sensor
        {
                //bzero(&ts[i].rom[0],8);                                                                                               // blank the rom
                ts[i].active=FALSE;
                ts[i].rom[0]=0x00;
                ts[i].rom[1]=0x00;
                ts[i].rom[2]=0x00;
                ts[i].rom[3]=0x00;
                ts[i].ftemp=0;
                ts[i].pftemp=-999;
                ts[i].atemp[0]=0;
        }
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






//       A  
//     F   B
//       G
//     E   C
//       D
//
//   32  16  8   4   2   1
//   A   B   C   D   E   F
char segments[10][8]={	"abcdef",			// 0
                      	"bc",				// 1
                      	"abdeg",			// 2
                      	"abcdg",			// 3
                      	"bcfg",				// 4
                      	"acdfg",			// 5
			"acdefg",			// 6
			"abc",				// 7
			"abcdefg",			// 8
			"abcdfg" };			// 9


void ICACHE_FLASH_ATTR render_digit(int digit, int value, int setorblank, int x_offset)
{
	char segon[2];
	char segoff[2];
	char spaces[6];
	int i;
	int a;									// accross
	int d=0;								// down

	os_sprintf(spaces,"    ");
	a=x_offset+(digit*6)-1;						// move left to right

	if (value>9)
		return;
	if (setorblank==0)							// set or clear the digit
		os_sprintf(segon,"{");
	else	os_sprintf(segon," ");


	// Special case, plus and minus symbol
	// 45 ( ASCII - )  -  48 ( ASCII 0 ) = -3

	if (value == -3)							// normal minus sign
	{
		OLED_Print(0+a,2+d,segon,1);	OLED_Print(1+a,2+d,segon,1);	OLED_Print(2+a,2+d,segon,1);	OLED_Print(3+a,2+d,segon,1);
		return;
	}
	if (value == -4)							// special short minus for far left of display
	{
		OLED_Print(0,2+d,segon,1);	OLED_Print(1,2+d,segon,1);	OLED_Print(2,2+d,segon,1);
		return;
	}


	// Special case, value of 8 and an intention to blank, blank every pixel
	if ( ( value==8 ) && ( setorblank!=0) )
	{
		for (i=0;i<5;i++)
			OLED_Print(0+a,0+d+i,spaces,1);
		return;
	}
	if ( (value<0) | (value>9) )
		return;
		

	for (i=0;i<7;i++)
	{
		//system_soft_wdt_feed();
		switch (segments[value][i])
		{
			// partially overlapping segments
			case 'a' :  OLED_Print(0+a,0+d,segon,1);	OLED_Print(1+a,0+d,segon,1);	OLED_Print(2+a,0+d,segon,1);	OLED_Print(3+a,0+d,segon,1);	break;	
			case 'b' :  OLED_Print(3+a,0+d,segon,1);	OLED_Print(3+a,1+d,segon,1);	OLED_Print(3+a,2+d,segon,1);	break;	
			case 'c' :  OLED_Print(3+a,2+d,segon,1);	OLED_Print(3+a,3+d,segon,1);	OLED_Print(3+a,4+d,segon,1);	break;	
			case 'd' :  OLED_Print(0+a,4+d,segon,1);	OLED_Print(1+a,4+d,segon,1);	OLED_Print(2+a,4+d,segon,1);	OLED_Print(3+a,4+d,segon,1);	break;	
			case 'e' :  OLED_Print(0+a,2+d,segon,1);	OLED_Print(0+a,3+d,segon,1);	OLED_Print(0+a,4+d,segon,1);	break;	
			case 'f' :  OLED_Print(0+a,0+d,segon,1);	OLED_Print(0+a,1+d,segon,1);	OLED_Print(0+a,2+d,segon,1);	break;	
			case 'g' :  OLED_Print(0+a,2+d,segon,1);	OLED_Print(1+a,2+d,segon,1);	OLED_Print(2+a,2+d,segon,1);	OLED_Print(3+a,2+d,segon,1);	break;	
		}
	}
}



// ts_idx, table entry for the sensor want to display, default is 0 the first sensor detected
void ICACHE_FLASH_ATTR render_temp(int ts_idx)
{
	static int pbigtwo=0;
	char s[4];
	int i=0;
	int a=0;
	int leftofdot;


	// Render decimal point
	s[0]='{';
	s[1]=0;
  	OLED_Print(15,4,s,1);													// both digits, small text

	if (ts[dis_tsidx].active != TRUE)
		return;

	toggle_invert();
	strcpy(dis_temp,ts[dis_tsidx].atemp);
//os_printf("dis_temp=%s\n",dis_temp);
	if (ts[dis_tsidx].ftemp<-99)												// No sensor connected or error
	{
		dis_temp[0]=' '; 	dis_temp[1]=' ';	dis_temp[2]='.';	dis_temp[3]=' ';	dis_temp[4]=' ';	dis_temp[5]=0;
		pbigtwo++;													// ensure next block of code renders our -
	}

	if (dis_temp[1]=='0')													// remoe leading zero(s)
	{
		dis_temp[1]=dis_temp[0];											// move the + or - to the right
		dis_temp[0]=' ';
	}
	
	//os_printf("dis_temp=%s\n",dis_temp);

	// Render first 2 digits
	render_digit(1,8,1,-1);
	render_digit(2,8,1,-1);
	render_digit(1,dis_temp[1]-48,0,-1);
	render_digit(2,dis_temp[2]-48,0,-1);

	// Display least sig as a single digit 
	//if (dis_temp[4]!=pdis_temp[4])
	//{
		render_digit(3,8,1,0);												// blank the digit
		render_digit(3,dis_temp[4]-48,0,0);			
	//}

	// Short minus sign when double digit negative number is displayed
	render_digit(0,-4,1,-1);												// blank extra short minus sign
	if ( (dis_temp[0]=='-') && (dis_temp[1]!='0') & (dis_temp[1]!=' ') )
		render_digit(0,-4,0,0);												// render sign
}



int docls=FALSE;
void ICACHE_FLASH_ATTR render_display()
{
	char st[16];
	struct ip_info ipconfig;

	if (docls==TRUE)
	{
    		OLED_CLS();
		docls=FALSE;
	}

	if (display_ip_details_count>0)												// displaying this instead
	{
		wifi_get_ip_info(STATION_IF, &ipconfig);
    		os_sprintf(st,"IP: %d.%d.%d.%d", IP2STR(&ipconfig.ip));
        	OLED_Print(0, 2, st, 1);
    		os_sprintf(st,"SN: %d.%d.%d.%d", IP2STR(&ipconfig.netmask));
        	OLED_Print(0, 3, st, 1);
    		os_sprintf(st,"GW: %d.%d.%d.%d", IP2STR(&ipconfig.gw));
        	OLED_Print(0, 4, st, 1);
	}
	else
	{
		//if (ts[dis_tsidx].ftemp!=ts[dis_tsidx].pftemp)
		//{
			os_printf("Calling render_temp(%d)\n",dis_tsidx);
			render_temp(dis_tsidx);
			ts[dis_tsidx].pftemp=ts[dis_tsidx].ftemp;
		//}
		//OLED_Print(0, 7, dis_botline, 1);
		OLED_Print(0, 3, dis_botline, 2);
	}
}





// 10 Hz
// GPIO goes low only for as long as finger is held on the button
void fast_timer_cb(void *arg)
{
	static int counter=0;
        static int sc=0;                                                                        // seconds counter
	static int fc=0;
	static int ponline=-1;

	if (display_ip_details_count>0)												// displaying this instead
	{
		display_ip_details_count--;
		if (display_ip_details_count==0)
			docls=TRUE;
	}

	if (online!=ponline)
	{
		ponline=online;
		os_printf("online changed to %d\n",online);
		if (online==1)
		{
			display_ip_details_count=20;
			docls=TRUE;
			render_display();
		}
	}


        fc++;
        if (fc>10)
        {
                fc=0;
                sc++;                                                                           // count in seconds
                os_printf("report_interval_seconds = %d\tsc=%d\n",report_interval_seconds, sc);
        }

        if (sc>=report_interval_seconds)
        {
                sample_and_send();
                sc=0;
        }

	counter++;
	if (counter>10 * 5)
	{
		//clear_ts_table();
		//num_ts=do_ds18b20(ONEWIREGPIO);
		sample_temps();
		counter=0;
		render_display();
	}
}


