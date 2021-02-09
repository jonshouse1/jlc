/*
	jlc_log.c	

If this error comes back, investigate here first
corrupted size vs. prev_size
Aborted (core dumped)

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"

extern int log_sensors;										// Log movement sensor events
extern int log_sensors_e;									// also write log line to stdout
extern int log_temps;
extern int log_temps_e;
extern char log_path[];



// logfile file is placed in logpath
void log_printf(char* logfile, const char *fmt, ...)
{
	char buf[8192];
	char filename[4096];
	char text[4096];
	va_list arg;
	static FILE *lf=NULL;									// log file

	text[0]=0;
	sprintf(filename,"%s/%s-%s",log_path,datetimes(20),logfile);
	//printf("filename=%s\n",filename); fflush(stdout);

	va_start(arg, fmt);
	buf[0]=0;
	vsprintf((char*)&buf, fmt, arg);
	if (strlen(buf)>0)
		strcat(text,buf);
        va_end(arg);


	//printf("writing [%s]\n",text); fflush(stdout);
	lf=fopen(filename,"a");									// Append or create if missing
	if (lf==NULL)
	{
		printf("Error trying to create logfile %s\n",filename);
		fflush(stdout);
		exit(1);
	}
	fprintf(lf,"%s",(char*)&text);
	fflush(lf);
	fclose(lf);
}




