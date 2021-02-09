/*
	jcp-client-esp.c
	ESP8266 Non RTOS SDK specific code for jcp_client.c
	
	Version 0.3
	(c) 2017 J.Andrews (jon@jonshouse.co.uk)
*/

#include <math.h>
#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>
#include <mem.h>
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <user_interface.h>
#include <espconn.h>
#include "uart.h"
#include "jcp_protocol_devs.h"
#include "jcp_client.h"

struct espconn discovery_conn;
static esp_udp discovery_udp;
static volatile os_timer_t jcp_timer;
unsigned char jcp_uidbase[6];
extern char fwver[32];

extern int online;
struct espconn conn2;
static esp_udp udpsd2;


// Prototypes


// Used by jlc_client.c
char* printuid(char unsigned *uid)
{
        int i=0;
        unsigned char c=0;
        static char hexstring[32];
        char *hexstringp=&hexstring[0];
        char st[3];

        bzero(&hexstring,sizeof(hexstring));
        for (i=0;i<UID_LEN;i++)
        {
                c=uid[i];
                os_sprintf(st,"%02X",(unsigned char)c);
                strcat(hexstring,st);
        }
        return(hexstringp);
}



// Entry point for receiving UDP data
void ICACHE_FLASH_ATTR jcp_cb(void* arg, char* p_data, unsigned short len)
{
	struct espconn* conn = (struct espconn*)arg;
	char ipaddr[32];

	// Extract the IP address of sender as a string
	if(conn->type == ESPCONN_UDP) 
	{
      		remot_info *remote = NULL;
      		if(espconn_get_connection_info(conn, &remote, 0) == 0) 
		{
			// Extract the IP address of sender as a string
			yip2str((char*)&ipaddr,remote->remote_ip);
			//os_printf("Got packet from [%s] %d bytes\n",ipaddr,len);

			if (len>=8)
			{
				if (strncmp(p_data, "FLASH", 5)==0)
				{
					flasher((char*)p_data, len, ipaddr);
					return;
				}
			}
			jcp_parse_and_reply((char*)p_data, len, ipaddr);
		}
	}
}


// Timer tick
void ICACHE_FLASH_ATTR jcp_timer_cb( void )
{
	if (online!=TRUE)
		return;
	jcp_timertick_handler();
}



// Kills the protocol, reboot to recover
void ICACHE_FLASH_ATTR jcp_suspend_timer()
{
	os_timer_disarm(&jcp_timer);
}


// Start UDP listening and timer callback
void ICACHE_FLASH_ATTR jcp_init(int jcpport, int jlpport, int timertickinterval)
{
	char mymac[7];

        conn2.type = ESPCONN_UDP;
        conn2.state = ESPCONN_NONE;
        udpsd2.local_port = jcpport;					// port for server replies (our listening port)
        conn2.proto.udp = &udpsd2;
        espconn_create(&conn2);
        espconn_regist_recvcb(&conn2, jcp_cb);
        os_printf("jcp_init() timertickinterval=%d, Listening for JCP mesages on UDP port %d\n",timertickinterval,udpsd2.local_port);

	os_timer_setfn(&jcp_timer,(os_timer_t *)jcp_timer_cb,NULL);
	os_timer_arm(&jcp_timer,timertickinterval,1);

	wifi_get_macaddr(STATION_IF,mymac);
//void ICACHE_FLASH_ATTR jcp_init_b(char *uidb, uint16_t sid, int jcp_udp_iport, int jlp_udp_iport, int tick_interval_ms, char* device_model, char* device_fw_ver)
	jcp_init_b(mymac, (uint16_t)os_random(), jcpport, jlpport, timertickinterval, MODEL, fwver);
}



