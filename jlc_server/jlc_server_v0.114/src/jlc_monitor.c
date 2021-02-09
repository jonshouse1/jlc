/*
	jlc_monitor.c
	Depending on the state of flags output data about system state changes
*/


#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"

extern uint32_t console_m;


void monitor_printf(int e, const char *fmt, ...)
{
        va_list arg;
        char buf[1024*16];

        va_start(arg, fmt);
        buf[0]=0;
        vsprintf((char*)&buf, fmt, arg);
        if (strlen(buf)>0)
        {
		//fprintf(stderr,"%s",(char*)&buf);
		//fflush(stderr);
        }
        va_end(arg);

	conn_monitor(e, (char*)&buf);		// send messages to tcpconn code	

	//printf("e=%d  console_m=%d\n",e,console_m); fflush(stdout);
	if (console_m>0)			// console asked for messages
	{
		if (console_m&e)		// bit set ?
			printf("%s",buf);	// then it is for us
	}
}


