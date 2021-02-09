/* main.c

   This source, plus the source in modelsrc and ./compile script builds firmware for many devices.

   This is the code common to all devices
*/


#define FW_VER		"jlcfw V0.16"


#define 	LED_TIMER_HZ		5
#define         SSID                    "yourssidhere"
#define         PASSWORD                "yourpasshere"

#include <math.h>
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <mem.h>
#include "user_config.h"
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <user_interface.h>
#include <espconn.h>
#include "uart.h"
#include "config.h"
#include "jcp_protocol_devs.h"
#include "jcp_client.h"
#include "jlp.h"
#include "prototypes.h"
//#include <stint.h>

// Prototypes
void ICACHE_FLASH_ATTR app_suspend_timers();
void ICACHE_FLASH_ATTR main_suspend_timers();

#include "../modelsrc/espflasher.c"		// This will only work if this code does not move around, so it needs to be here always
void ICACHE_FLASH_ATTR connected();

configrecord    cfg;
#include "mfs.h"
#include "mfs_filecount.h"
struct MFSFileInfo      mfs_fileinfo;
char softap_ssid[33];

#include "pin_map.h"
#define output_low(g)   GPIO_OUTPUT_SET(g,0);
#define output_high(g)  GPIO_OUTPUT_SET(g,1);

static volatile os_timer_t led_timer;
int online=FALSE;
//uint16_t my_session_id=0;
char fwver[32];


#include "common.h"
char jahttp_cgi_variables[NO_jahttp_cgi_variables][16]={ "SSID=",		// 0
							"PASSWORD=",		// 1
							"SWI1=",		// 2
							"SWI2=",		// 3
                                                        "SWI3=",		// 4
							"LOC=",			// 5
							"CMD_FLASH="		// Always Last
							};



#ifdef H801
	#include "../modelsrc/global_h801.c"
#endif
#ifdef SPOTLIGHT_1CH
	#include "../modelsrc/global_spotlight_1ch.c"
#endif
#ifdef T1_UK_1C
	#include "../modelsrc/global_t1_uk.c"
#endif
#ifdef T1_UK_2C
	#include "../modelsrc/global_t1_uk.c"
#endif
#ifdef T1_UK_3C
	#include "../modelsrc/global_t1_uk.c"
#endif
#ifdef DOORBELL
	#include "../modelsrc/global_doorbell.c"
#endif
#ifdef SONOFF_RELAY_TH10
	#include "../modelsrc/global_sonoff_relay_th10.c"
#endif
#ifdef PIR
	#include "../modelsrc/global_pir.c"
#endif
#ifdef SONOFF_RELAY1
	#include "../modelsrc/global_sonoff_relay1.c"
#endif
#ifdef SONOFF_RELAY_DUAL
	#include "../modelsrc/global_sonoff_relay_dual.c"
#endif
#ifdef SONOFF_RELAY4
	#include "../modelsrc/global_sonoff_relay4.c"
#endif
#ifdef DMX_5CH_WS2811
	#include "../modelsrc/global_dmx_5ch_ws2811.c"
#endif
#ifdef MAX7219CLOCK
	#include "../modelsrc/global_max7219clock.c"
#endif
#ifdef MAX7219_4LINE_DISPLAY
	#include "../modelsrc/global_max7219_4line_display.c"
#endif
#ifdef OLED_TEMPSENS
	#include "../modelsrc/global_oled_tempsens.c"
#endif
#ifdef RELAY_OPTO_BOARD
	#include "../modelsrc/global_relay_opto_board.c"
#endif


// Prepare to flash device by stopping all timers
void ICACHE_FLASH_ATTR main_suspend_timers()
{
	os_printf("main_suspend_timers()\n");
        os_timer_disarm(&led_timer);
}


char* ICACHE_FLASH_ATTR ipsta()
{
	struct ip_info ipconfig;
	static char ipaddr[18];

	bzero(&ipaddr,sizeof(ipaddr));
	if (wifi_get_ip_info(STATION_IF, &ipconfig)==TRUE)
		os_sprintf(ipaddr,"%d.%d.%d.%d",IP2STR(&ipconfig.ip));
	return((char*)&ipaddr);
}




// Write modified cfg structure back to flash
static ICACHE_FLASH_ATTR reflash( void )
{
	os_printf("Reflashing\n");
	main_suspend_timers();

	// Ensure all strings are 0 terminated
	cfg.location[sizeof(cfg.location)-1]=0;
        cfg.swi1[sizeof(cfg.swi1)-1]=0;
        cfg.swi2[sizeof(cfg.swi2)-1]=0;
        cfg.swi3[sizeof(cfg.swi3)-1]=0;

        if (configuration_save(&cfg)!=0)
                os_printf("Error writing config to flash\n");
        os_printf("\n\nFLASHED,Restarting ...\n\n");
        system_restart();
}




// Defaults if no configuration exists in flash
int ICACHE_FLASH_ATTR load_config_defaults(configrecord *cfg)
{
        bzero(cfg,sizeof(configrecord));                                                        // start with all fields padded with 0x00
	strcpy((char*)cfg->swi1,"111");
	strcpy((char*)cfg->swi2,"112");
	strcpy((char*)cfg->swi3,"112");
        os_printf("loaded config defaults\n");
}




// Server is sending us configuration information
void ICACHE_FLASH_ATTR jcp_dev_config(char *dt, int len)
{
	os_printf("cp_dev_config() ");
	// Set location
	if (dt[0]=='L')
	{
		bzero(&cfg.location, sizeof(cfg.location));
		strncpy((char*)&cfg.location, dt+1, sizeof(cfg.location));
		os_printf("Location set to [%s]\n",cfg.location);
		reflash();
	}

	// Set Wifi SSID/Password (insecure password passing via plain text)
	// 66 Bytes should follow 'W' 32 Bytes SSID plus a zero, 32 bytes password plus a zero
	if (dt[0]=='W')
	{
		memcpy(cfg.ssid,dt,sizeof(cfg.ssid));
		memcpy(cfg.password,dt+33,sizeof(cfg.password));
		os_printf("SSID set to [%s]  Pw [%s]\n",cfg.ssid, cfg.password);
		//reflash();
	}
}






// for example $TEMP0 = would be replaced with +NN.XX
// The quotes are passed in and out,  so "$THISVAR" in, substituted with 10 would be "10"______   _=space (32 ASCII)
// This does mean the html has rather long fields, for example "$SSID                           "
void ICACHE_FLASH_ATTR jahttp_cgi_variable_to_value(char *cgivar, int hasquote)
{
        char b='\0';
        int i=0;
        char st[64];

        //os_printf("jahttp_cgi_variable_to_value (%c%c%c%c)\n",cgivar[0],cgivar[1],cgivar[2],cgivar[3]);
        if (strncmp(cgivar,"$SSID",5)==0)
        {
                jahttp_overwrite((char*)cgivar,(char*)&cfg.ssid,hasquote,sizeof(cfg.ssid));
                return;
        }
        if (strncmp(cgivar,"$PASSWORD",9)==0)
        {
                jahttp_overwrite((char*)cgivar,(char*)&b,hasquote,sizeof(cfg.password));                                        // always return a blank password, user must re-enter it
                return;
        }

        if (strncmp(cgivar,"$SWI1",5)==0)
        {
                jahttp_overwrite((char*)cgivar,(char*)&cfg.swi1,hasquote,sizeof(cfg.swi1));
                return;
        }

        if (strncmp(cgivar,"$SWI2",5)==0)
        {
                jahttp_overwrite((char*)cgivar,(char*)&cfg.swi2,hasquote,sizeof(cfg.swi2));
                return;
        }

        if (strncmp(cgivar,"$SWI3",5)==0)
        {
                jahttp_overwrite((char*)cgivar,(char*)&cfg.swi3,hasquote,sizeof(cfg.swi3));
                return;
        }

        if (strncmp(cgivar,"$LOC",4)==0)
        {
                jahttp_overwrite((char*)cgivar,(char*)&cfg.location,hasquote,sizeof(cfg.location));
                return;
        }
}


// POST variables. We extract these and push them back into local variables
//len=length of value
void ICACHE_FLASH_ATTR jahttp_cgi_post(int cgi_variables_idx, char *value, int len)
{
        char valuest[128];
	char de_valuest[512];
        int i;

        bzero(&valuest,sizeof(valuest));                                                                                        // important for some things that string is zero padded
        if (len>0)
	{
                memcpy(&valuest,value,len);                                                                                     // copy string from tcp buffer
		bzero(&de_valuest,sizeof(de_valuest));
		percent_decode((char*)&valuest, len, (char*)&de_valuest, sizeof(de_valuest));
	}


        os_printf("POST %d[%s] [%s]\n",cgi_variables_idx,jahttp_cgi_variables[cgi_variables_idx],valuest);
        switch (cgi_variables_idx)
        {
                case 0:         memcpy(&cfg.ssid,&de_valuest,sizeof(cfg.ssid));							// SSID,copy string with padding
                                break;

                case 1:         memcpy(&cfg.password,&de_valuest,sizeof(cfg.password));						// PASSWORD
                                break;

                case 2:         memcpy(&cfg.swi1,&valuest,sizeof(cfg.swi1));							// SWI1
                                break;

                case 3:         memcpy(&cfg.swi2,&valuest,sizeof(cfg.swi2));							// SWI2
                                break;

                case 4:         memcpy(&cfg.swi3,&valuest,sizeof(cfg.swi3));							// SWI3
                                break;

                case 5:		for (i=0;i<sizeof(valuest)-1;i++)
                                        if (valuest[i]=='+')
                                                valuest[i]=' ';
                                memcpy(&cfg.location,&valuest,sizeof(cfg.location));                                            // LOC
                                break;

                case 6:		reflash();                                                                                      // Must always be last in the list
                                break;
        }
}



// Javascript control sends brightness data as a PWM N = message, decode it
void ICACHE_FLASH_ATTR cgi_handler(char* buf, int len)
{
	int i;
	int n;

    	//for (i=0;i<len;i++)                                                                         // Find PWM0= or MOTR=
    	//{
        	//if ( (buf[i]=='R') && (buf[i+1]=='E') && (buf[i+2]=='L') && (buf[i+4]=='=') )
        	//{                                                                                       // sscanf seems to eat ram so improvise
                	//n=buf[i+3]-'0';
                	//if (n<4)
                	//{
                        	//if (buf[i+5]=='1')
                                	//relays[n]=TRUE;
                        	//else    relays[n]=FALSE;
                	//}
                	//break;
        	//}
   	//}
}






// Flash LED on device until we are associated with an access point and have an IP from DHCP
int onoff=0;
void ICACHE_FLASH_ATTR led_timer_cb(void *arg)
{
	int c;

	if (online!=TRUE)													// flash LED if not connected to an AP
	{
		onoff=onoff^1;
		if (onoff==1)
		{
			os_printf("x");
			if (wifi_led>0)
				output_low(wifi_led);
		}
		else	
		{
			if (wifi_led>0)
				output_high(wifi_led);
		}
	}
	else	
	{
		if (wifi_led>0)
			output_low(wifi_led);											// online = steady on
	}
}




static ICACHE_FLASH_ATTR setup_softap( void )
{
        struct softap_config  sac;
        char mac_softap[32];

        bzero(&mac_softap,sizeof(mac_softap));
        //wifi_softap_get_config(&sac);                                                                                         // Get the current settings
        wifi_softap_get_config_default(&sac);                                                                                   // Get the current settings as stored in flash

        //wifi_get_macaddr(SOFTAP_IF, mac_softap);                                                                                // Get the softap MAC address
        wifi_get_macaddr(STATION_IF, mac_softap);                                                                                // Get MAC is access point sees it
        os_sprintf(softap_ssid,"%s_%02X%02X%02X%02X%02X%02X",MODEL,mac_softap[0],mac_softap[1],mac_softap[2],mac_softap[3],mac_softap[4],mac_softap[5]);
        os_printf("Setting softap ssid to: %s\n\r",softap_ssid);

	bzero(sac.ssid,sizeof(sac.ssid));
        strcpy(sac.ssid,softap_ssid);
	bzero(sac.password,sizeof(sac.password));
	sac.authmode=AUTH_OPEN;
        sac.ssid_len=strlen(softap_ssid);                                                                                       // ssid can contain \0 in places other than the end

        wifi_softap_set_config_current(&sac);                                                                                   // set, but dont flash settings
        wifi_softap_set_config(&sac);                                                                                           // set softap details and store in flash
}




static ICACHE_FLASH_ATTR void init_done( void )
{
     struct station_config station_config;
     struct softap_config config;
     int i=0;

     wifi_set_opmode_current(STATIONAP_MODE);
     wifi_set_broadcast_if(3);                                                                                                  // 1:station, 2:soft-AP, 3:both

     bzero(&station_config.ssid,sizeof(station_config.ssid));
     bzero(&cfg,sizeof(cfg));
     if (configuration_load(&cfg)==0)
     {
        os_printf("Got configuration for Wifi from flash, ssid=%s   location=%s\n",cfg.ssid, cfg.location);
        strncpy(station_config.ssid, cfg.ssid, 32);
        strncpy(station_config.password, cfg.password, 32);
    	if (strlen(station_config.ssid)==0 || strlen(station_config.password)==0)
    	{
		strncpy(station_config.ssid, SSID, 32);
		strncpy(station_config.password, PASSWORD, 32);
    	}

        wifi_station_set_config(&station_config);
        if (strlen(station_config.ssid)>0)
	{
		os_printf("wifi_station_connect();\n");
                wifi_station_connect();
	}
     }
     else       
     {
		os_printf("No configuration in flash\n");
		load_config_defaults(&cfg);
		reflash();
     }
}




static ICACHE_FLASH_ATTR void event_cb(System_Event_t* event)
{
        int i;
        char st[22];

	if (event->event==EVENT_SOFTAPMODE_PROBEREQRECVED)
		return;

	os_printf("event=%d\n",event->event);
	switch (event->event)
	{
                case EVENT_STAMODE_CONNECTED:											// connected to access point, but not yet online
		break;

		case EVENT_STAMODE_DISCONNECTED:
			online=FALSE;
		break;

		case EVENT_STAMODE_GOT_IP:
			online=TRUE;
			connected();
		break;

		case EVENT_STAMODE_DHCP_TIMEOUT:
			online=FALSE;
		break;

		case EVENT_SOFTAPMODE_STADISCONNECTED:
		break;
        }
}





void ICACHE_FLASH_ATTR list_files()
{
	int bytesleft=0;
    	char asector[MFS_SECTOR];
    	int r,line;
    	char fname[24];

    	for (line=1;line<=MFS_FILECOUNT;line++)
    	{
        	if ( MFSFileList(line, (char*)&fname, &r )==0)                                                                          // while more files
                	os_printf("%d\t%d\t%s\n",line,r,fname);
    	}
    	os_printf("\n");
}


#ifdef H801
	#include "../modelsrc/model_init_h801.c"
#endif
#ifdef SPOTLIGHT_1CH
	#include "../modelsrc/model_init_spotlight_1ch.c"
#endif
#ifdef T1_UK_1C
	#include "../modelsrc/model_init_t1_uk_1c.c"
#endif
#ifdef T1_UK_2C
	#include "../modelsrc/model_init_t1_uk_2c.c"
#endif
#ifdef T1_UK_3C
	#include "../modelsrc/model_init_t1_uk_3c.c"
#endif
#ifdef DOORBELL
	#include "../modelsrc/model_init_doorbell.c"
#endif
#ifdef SONOFF_RELAY_TH10
	#include "../modelsrc/model_init_sonoff_relay_th10.c"
#endif
#ifdef PIR
	#include "../modelsrc/model_init_pir.c"
#endif
#ifdef SONOFF_RELAY1
	#include "../modelsrc/model_init_sonoff_relay1.c"
#endif
#ifdef SONOFF_RELAY_DUAL
	#include "../modelsrc/model_init_sonoff_relay_dual.c"
#endif
#ifdef SONOFF_RELAY4
	#include "../modelsrc/model_init_sonoff_relay4.c"
#endif
#ifdef DMX_5CH_WS2811
	#include "../modelsrc/model_init_dmx_5ch_ws2811.c"
#endif
#ifdef MAX7219CLOCK
	#include "../modelsrc/model_init_max7219clock.c"
#endif
#ifdef MAX7219_4LINE_DISPLAY
	#include "../modelsrc/model_init_max7219_4line_display.c"
#endif
#ifdef OLED_TEMPSENS
	#include "../modelsrc/model_init_oled_tempsens.c"
#endif
#ifdef RELAY_OPTO_BOARD
	#include "../modelsrc/model_init_relay_opto_board.c"
#endif



void ICACHE_FLASH_ATTR user_init()
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000);
	strcpy(fwver,FW_VER);
	strcat(fwver," ");
	strcat(fwver,__DATE__);
	os_printf("\n\n\n\n%s [%s] [%s]\n", MODEL, fwver);
        init_dns(); os_printf("\n"); 

    	wifi_set_event_handler_cb(event_cb); 
	setup_softap();

        list_files();
        init_jahttp(80);
     	configuration_load(&cfg);
	model_init();

	os_timer_setfn(&led_timer,(os_timer_t *)led_timer_cb,NULL);
        os_timer_arm(&led_timer,1000/LED_TIMER_HZ,1);

        system_init_done_cb(init_done);
        system_os_post(0, 0, 0 );
}


