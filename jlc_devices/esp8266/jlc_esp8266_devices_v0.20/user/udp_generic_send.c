/* 
 */

#include <osapi.h>
#include <os_type.h>
#include <mem.h>
#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#include <espconn.h>


int ICACHE_FLASH_ATTR udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast)
{
        int err=0, es=0;
        static struct espconn sendResponse;
        static esp_udp udptx;

        //os_printf("Send %d bytes to %s, port %d\n",len,destination_ip,destination_port);
        sendResponse.type = ESPCONN_UDP;
        sendResponse.state = ESPCONN_NONE;
        sendResponse.proto.udp = &udptx;
        ipaddr_aton(destination_ip, &sendResponse.proto.udp->remote_ip);			// convert string to 4 byte ipv4 address
        sendResponse.proto.udp->remote_port = destination_port;
        err = espconn_create(&sendResponse);
        es  = espconn_send(&sendResponse, d, len);
        err = espconn_delete(&sendResponse);
        return(es);
}



