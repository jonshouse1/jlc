/*
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


/* Could require up to 16 bytes */
char * ICACHE_FLASH_ATTR yip2str ( char *buf, unsigned char *p )
{
    os_sprintf ( buf, "%d.%d.%d.%d", p[0], p[1], p[2], p[3] );
    return buf;
}

