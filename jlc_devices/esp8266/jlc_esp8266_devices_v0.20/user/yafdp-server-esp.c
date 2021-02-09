// Yet Another Functional Discovery Protocol test
// For ESP8266 Non RTOS SDK code
// Version 0.3
// (c) 2017 J.Andrews (jon@jonshouse.co.uk)
//


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
#include "user_config.h"

#include "yet_another_functional_discovery_protocol.h"


struct espconn discovery_conn;
static esp_udp discovery_udp;

int yafdp_debug=TRUE;
int number_of_services=0;


void ICACHE_FLASH_ATTR yafdp_discovery_udp_cb(void* arg, char* p_data, unsigned short len)
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
			yafdp_parse_and_reply((char*)p_data, len, ipaddr);
		}
	}
}



// Call yafdp_server_init() first
void ICACHE_FLASH_ATTR yafdp_start_udp_server()
{
      	discovery_conn.type = ESPCONN_UDP;
      	discovery_conn.state = ESPCONN_NONE;
      	discovery_udp.local_port = YAFDP_DISCOVERY_PORT;
      	discovery_conn.proto.udp = &discovery_udp;
      	espconn_create(&discovery_conn);
      	espconn_regist_recvcb(&discovery_conn, yafdp_discovery_udp_cb);
      	os_printf("\n\rListening for YAFDP discovery packets on port %d\n",discovery_udp.local_port);
}



