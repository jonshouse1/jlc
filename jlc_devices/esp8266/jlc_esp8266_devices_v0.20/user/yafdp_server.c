/* 
	YAFDP Server
	Active parse and response for YAFDP protocol
	Common code, compiles for linux/esp8266/esp32

	Version 0.10
	Last Modified 17 Jul 2019
*/

#ifndef ESP8266		
	#include <stdio.h> 
	// If you run out of memory on the ESP8266 you probably forgot to add -DESP8266 to CFLAGS in Makefile
#else										// 8266 specific includes
	#include <ets_sys.h>
	#include <osapi.h>
	#include <os_type.h>
	#include <mem.h>
	#include <ets_sys.h>
	#include <osapi.h>
	#include <user_interface.h>
	#include <espconn.h>
	#include "uart.h"
#endif

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
//#include "global.h"
#include "yet_another_functional_discovery_protocol.h"
#include "yafdp-user-settings.c"

#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef ESP8266
	#define ICACHE_FLASH_ATTR
#endif

extern int yafdp_debug;
char yafdp_manufacturer[17];
char yafdp_modelname[33];
char yafdp_device_description[33];
char yafdp_location[33];
extern int number_of_services;


#ifndef ESP8266
#include <stdarg.h>
int os_printf(char *fmt, ...) 
{
	va_list arg;
	char buf[8192];

	va_start(arg, fmt);
	buf[0]=0;
	vsprintf((char*)&buf, fmt, arg);
	if (strlen(buf)>0)
        {
                
		printf("%s",buf);
	}
	va_end(arg);
	return(0);
}
#endif



int ICACHE_FLASH_ATTR yafdp_count_services()
{
        int i;

        for (i=0;i<YAFDP_MAX_SERVICES;i++)
        {
                if (strcmp(yafdp_servicelist[i][0],"-1")==0)
                        return(i);                                                                              // hit end of list
        }
        return(0);
}




void ICACHE_FLASH_ATTR yafdp_server_init(char *manufacturer, char *modelname, char *device_description, char *location)
{
	// ensure unused bytes are zero
	bzero(&yafdp_manufacturer,sizeof(yafdp_manufacturer));
	bzero(&yafdp_modelname,sizeof(yafdp_modelname));
	bzero(&yafdp_device_description,sizeof(yafdp_device_description));
	bzero(&yafdp_location,sizeof(yafdp_location));
	// use strncpy to clip string if longer than the structure field length
	strncpy((char*)&yafdp_manufacturer, manufacturer, sizeof(yafdp_manufacturer)-1);
	strncpy((char*)&yafdp_modelname, modelname, sizeof(yafdp_modelname)-1);
	strncpy((char*)&yafdp_device_description, device_description, sizeof(yafdp_device_description)-1);
	strncpy((char*)&yafdp_location, location, sizeof(yafdp_location)-1);
	number_of_services=yafdp_count_services();

	os_printf("YAFDP Server:\n");
	os_printf("Location = %s\n",yafdp_location);
	os_printf("Device Description = %s\n",yafdp_device_description);
	os_printf("Model name = %s\n",yafdp_modelname);
	os_printf("Manufacturer = %s\n",yafdp_manufacturer);
	os_printf("Number of services = %d\n\n",number_of_services);
}



// This can also be used as an announce if called with ipaddr 255.255.255.255 and request_handle 0
void ICACHE_FLASH_ATTR send_yafdp_discovery_reply_device(char *ipaddr,  int vers, int req_handle, int port)
{
	char txbuf[sizeof(struct yafdp_reply_device)];
	struct yafdp_reply_device *yafdpreplydev=(struct yafdp_reply_device*)&txbuf;
	int udp_port=YAFDP_DISCOVERY_PORT;								// default for version 0.1 of protocol
	int bcast=FALSE;

	if (strcmp(ipaddr,"255.255.255.255")==0)
		bcast=TRUE;

	bzero(&txbuf,sizeof(txbuf));
	if (port>1024)
		udp_port=port;

	// Build reply packet
	strcpy(yafdpreplydev->magic,YAFDP_MAGIC);
	yafdpreplydev->ptype = YAFDP_TYPE_DISCOVERY_REPLY_DEVICE;
	yafdpreplydev->pver[0] = 0;
	yafdpreplydev->pver[1] = vers;
	yafdpreplydev->request_handle = req_handle;
	yafdpreplydev->number_of_services=number_of_services;

	// use strncpy to clip string if longer than the structure field length
	strcpy(yafdpreplydev->device_manufacturer, yafdp_manufacturer);
	strcpy(yafdpreplydev->device_modelname, yafdp_modelname);
	strcpy(yafdpreplydev->device_description, yafdp_device_description);
	strcpy(yafdpreplydev->device_location, yafdp_location);
	udp_generic_send((char*)&txbuf, sizeof(struct yafdp_reply_device), (char*)ipaddr, udp_port, bcast);
	if (yafdp_debug==TRUE)
	{
		os_printf(">%s\tYAFDP_TYPE_DISCOVERY_REPLY_DEVICE, VER:%d, REQH:%d PORT:%d BCASET:%d\n",ipaddr,vers,req_handle,udp_port,bcast);
	}
}



void ICACHE_FLASH_ATTR yafdp_parse_and_reply(char *rbuffer, int bsize, char *ipaddr)
{
	int r=0;
	int number_of_services=0;
	static int mute_handle=0;
        struct yafdp_request_devices *yafdprd=(struct yafdp_request_devices*)rbuffer;
        struct yafdp_request_services *yafdprs=(struct yafdp_request_services*)rbuffer;
        struct yafdp_request_service *yafdpr=(struct yafdp_request_service*)rbuffer;

        // Overlay these over the buffer for the transmitted reply
	char txbuf[2048];
        //struct yafdp_reply_device *yafdpreplydev=(struct yafdp_reply_device*)&txbuf;
        struct yafdp_reply_service *yafdpreplyser=(struct yafdp_reply_service*)&txbuf;

	number_of_services=yafdp_count_services();
	if (strcmp(yafdprs->magic,YAFDP_MAGIC)==0)
	{
		switch (yafdprd->ptype) 
		{
			case YAFDP_TYPE_DISCOVERY_REQUEST_DEVICES :
				if (yafdp_debug==TRUE)
				{
					os_printf("<%s\tYAFDP_TYPE_DISCOVERY_REQUEST_DEVICES, REQH:%d\n",ipaddr,yafdprd->request_handle);
				}
				if (yafdprd->request_handle != mute_handle)
				{
					send_yafdp_discovery_reply_device(ipaddr,  yafdprd->pver[1], yafdprd->request_handle, yafdprd->udp_reply_port);
				}
			break;


			// Client requested a service by list sequence
			case YAFDP_TYPE_DISCOVERY_REQUEST_SERVICES :
				if (yafdp_debug==TRUE)
				{
					os_printf("<%s\tGot DISCOVERY_REQUEST_SERVICES, REQH:%d\n",ipaddr,yafdprs->request_handle);
				}
				bzero(&txbuf,sizeof(struct yafdp_reply_service));
				if (yafdprs->list_sequence<number_of_services)
				{
					bzero(&txbuf,sizeof(yafdpreplyser));
					r=yafdprs->list_sequence;
					if (yafdp_debug==TRUE)
					{
						os_printf(">%sYAFDP_TYPE_DISCOVERY_REPLY_SERVICE, REQH:%d IDX:%d\n",ipaddr,yafdprs->request_handle,r); 
					}
					strcpy(yafdpreplyser->magic,YAFDP_MAGIC);
					yafdpreplyser->list_sequence = r;
					yafdpreplyser->pver[0] = 0;
					yafdpreplyser->pver[1] = 1;
					yafdpreplyser->ptype = YAFDP_TYPE_DISCOVERY_REPLY_SERVICE;
					yafdpreplyser->request_handle = yafdprs->request_handle;
					yafdpreplyser->list_sequence = yafdprs->list_sequence;
					yafdpreplyser->udp_port = atoi(yafdp_servicelist[r][0]);
					yafdpreplyser->tcp_port = atoi(yafdp_servicelist[r][1]);
					strcpy(yafdpreplyser->service_protocol_name_short, yafdp_servicelist[r][2]);
					strcpy(yafdpreplyser->u1, yafdp_servicelist[r][3]);
					strcpy(yafdpreplyser->u2, yafdp_servicelist[r][4]);
					udp_generic_send((char*)&txbuf, sizeof(struct yafdp_reply_service), (char*)ipaddr, YAFDP_DISCOVERY_PORT, FALSE);
				}
			break;


			// for a given "request_handle" ignore any subsequent "YAFDP_TYPE_DISCOVERY_REQUEST_DEVICES" messages
			// same structure as "request devices, but in this context "request_handle" is the handle we wish to ignore
			case YAFDP_TYPE_DISCOVERY_REQUEST_MUTE :
				mute_handle = yafdprd->request_handle;
				if (yafdp_debug==TRUE)
				{
					os_printf("<%s\tYAFDP_TYPE_DISCOVERY_REQUEST_MUTE, REQH:%d\n",ipaddr,mute_handle);
				}
			break;



			// Do we have a given named service ?
			case YAFDP_TYPE_DISCOVERY_REQUEST_SERVICE :							// look for a single service by name
				if (yafdp_debug==TRUE)
				{
					os_printf("<%s\tDISCOVERY_REQUEST_SERVICE, REQH:%d\n",ipaddr,yafdpr->request_handle);
				}
				for (r=0;r<number_of_services;r++)							// check all services 
				{
					os_printf("asked for service_protocol_name_short=%s\n",yafdpr->service_protocol_name_short); 
					if (strcmp(yafdpr->service_protocol_name_short,yafdp_servicelist[r][2])==0)	// Match by name ?
					{
						os_printf("matched\n"); 
						bzero(&txbuf,sizeof(yafdpreplyser));
						os_printf("sending service index=%d\n",r);
						strcpy(yafdpreplyser->magic,YAFDP_MAGIC);
						yafdpreplyser->list_sequence = r;
						yafdpreplyser->pver[0] = 0;
						yafdpreplyser->pver[1] = 1;
						yafdpreplyser->ptype = YAFDP_TYPE_DISCOVERY_REPLY_SERVICE;
						yafdpreplyser->request_handle = yafdpr->request_handle;
						yafdpreplyser->udp_port = atoi(yafdp_servicelist[r][0]);
						yafdpreplyser->tcp_port = atoi(yafdp_servicelist[r][1]);
						strcpy(yafdpreplyser->service_protocol_name_short, yafdp_servicelist[r][2]);
						strcpy(yafdpreplyser->u1, yafdp_servicelist[r][3]);
						strcpy(yafdpreplyser->u2, yafdp_servicelist[r][4]);
						udp_generic_send((char*)&txbuf, sizeof(struct yafdp_reply_service), (char*)ipaddr, YAFDP_DISCOVERY_PORT, FALSE);
					}
				}
			break;


			case YAFDP_TYPE_DISCOVERY_REPLY_DEVICE :
				if (yafdp_debug==TRUE)
				{
					os_printf("<%s\tYAFDP_TYPE_DISCOVERY_REPLY_DEVICE\n",ipaddr);
				}
			break;

			case YAFDP_TYPE_DISCOVERY_REPLY_SERVICE :
				if (yafdp_debug==TRUE)
				{
					os_printf("<%s\tYAFDP_TYPE_DISCOVERY_REPLY_SERVICE\n",ipaddr);
				}
			break;
		}
	}
}


