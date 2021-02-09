/*
	jlc_time.c

	Date/time or timer related
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlp.h"
#include "jlc_prototypes.h"

extern struct    universe* uni[MAX_UNIVERSE+1];
extern uint16_t  session_id;
extern struct    statss  stats;
extern int num_devf;
extern int num_devn;
extern int loopcount;
extern int loophz;
extern int desired_loophz;
extern int dly;
extern struct timed_events     te[MAX_TIMED_EVENTS];
extern char    audioplus_md5[64];



#include <sched.h>
void set_realtime(void)
{
        struct sched_param sparam;
        sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(0, SCHED_FIFO, &sparam);
}



// Return 64 bit unsigned for current time
uint64_t current_timems()
{
	struct timeval te;
	gettimeofday(&te, NULL); // get current time
	unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
	return milliseconds;
}


int get_clockseconds()
{
        time_t          t;
        struct tm       *tm;

        time(&t);
        tm=localtime(&t);
        return(tm->tm_sec);
}



// Date time string
// fmt=0		 hh:mm:ss
// ***************************************************************************************************************************
//  int tm_sec;                   /* Seconds.     [0-60] (1 leap second) */
//  int tm_min;                   /* Minutes.     [0-59] */
//  int tm_hour;                  /* Hours.       [0-23] */
//  int tm_mday;                  /* Day.         [1-31] */
//  int tm_mon;                   /* Month.       [0-11] */
//  int tm_year;                  /* Year - 1900.  */
//  int tm_wday;                  /* Day of week. [0-6] */
//  int tm_yday;                  /* Days in year.[0-365] */
//  int tm_isdst;                 /* DST.         [-1/0/1]*/
//
char* datetimes(int fmt)
{
        struct tm *current;
        time_t now;
	static char dt[1024];

        time(&now);
        current = localtime(&now);

	switch (fmt)
	{
		case 0:
			strftime(dt,128,"%H:%M:%S",current);
		break;
		case 1:
			strftime(dt,128,"%a %e %b %Y %H:%M:%S %Z",current);	
		break;
		case 2:	
			strftime(dt,128,"%a %e %b %Y %H:%M:%S %z",current);	
		break;
		case 3:		
			strftime(dt,128,"%a %e %b %Y %H:%M:%S",current);
		break;
		case 4:		
			sprintf(dt,"%d %02d/%02d/%i %02d:%02d:%02d", current->tm_wday, current->tm_mday, current->tm_mon, 
				current->tm_year+1900, current->tm_hour, current->tm_min, current->tm_sec);
		break;
		case 5:		
			sprintf(dt,"%02d/%02d/%i %02d:%02d:%02d", current->tm_mday, current->tm_mon, 
				current->tm_year+1900, current->tm_hour, current->tm_min, current->tm_sec);
		break;
		case 11:
			strftime(dt,128,"%H:%M:%S",current);
		break;
		case 12:
			strftime(dt,128,"%H:%M",current);
		break;
		case 13:
			strftime(dt,128,"%S",current);
		break;

		case 20:
			sprintf(dt,"%02d_%02d_%i",current->tm_mday, current->tm_mon,current->tm_year+1900);
		break;
		case 30:
			current = gmtime(&now);						// UTC time
			sprintf(dt,"%s", asctime(current));
			dt[strlen(dt)-1]=0;
			strcat(dt," UTC");
		break;



		// Partial date/time strings
		case 40:								// Day of Week "Tue" for example
			strftime(dt,128,"%a",current);	
		break;
		case 41:								// "25 May"
			strftime(dt,128,"%d %b",current);	
		break;
		case 42:								// "2020"
			strftime(dt,128,"%Y",current);	
		break;


		default:
			//printf("datetimes() Error, something asked for unknown fmt=%d\n",fmt);
			//exit(1);
			current = gmtime(&now);						// UTC time
			sprintf(dt,"%s", asctime(current));
			dt[strlen(dt)-1]=0;
			strcat(dt," UTC");
		break;
	}
	return((char*)&dt);
}




// Format a string st with the number of bytes 'size' in human readable form 
void printsize(size_t size, char *st)
{
	static const char *SIZES[] = { "B", "kB", "MB", "GB" };
	size_t div = 0;
	size_t rem = 0;

	while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES)) {
		rem = (size % 1024);
		div++;
		size /= 1024;
	}
	sprintf(st,"%.1f %s", (float)size + (float)rem / 1024.0, SIZES[div]);
}




// Given a string such as '1s' '1m' '1h' '1d' return time in milliseconds or -1 for error
int timetext_to_ms(char *text)
{
	int tl=strlen(text);
	char st[32];
	char t;
	int v=0;

	if (tl<=0)
		return(-1);							// error, no text

	strcpy(st, text);							// save a copy of the string
	t=st[tl-1];								// save last character of string
	//printf(" timetext_to_ms st=%s  t=%c\n",st,t); fflush(stdout);
	if (t>'A')								// last character not a number?
		st[tl]=0;							// shorten string
	v=atoi(st);								// get numeric part of string
	if (v<0)
		return(-1);
	if (t=='S')
	{
		v=v*1000;							// adjust value for millisecond in a second
		return(v);
	}
	if (t=='M')
	{
		v=v*(1000*60);							// milliseconds per min
		return(v);
	}
	if (t=='H')
	{
		v=v*(1000*60*60);
		return(v);
	}
	if (t=='D')
	{
		v=v*(1000*60*60*24);
		return(v);
	}
	return(v);								// no suffix, value was milliseconds
}



// TODO: Finish this, mins hours days ...
//char* ms_to_timetext(int tms)
char* ms_to_timetext(unsigned long int tms)
{
	static char st[32];
	int ts=tms/1000;

	unsigned long int day = ts / (24 * 3600); 
	ts = ts % (24 * 3600); 
	unsigned long int hour = ts / 3600; 
	ts %= 3600; 
	unsigned long int minutes = ts / 60 ; 
	ts %= 60; 

	bzero(&st,sizeof(st));
	sprintf(st,"%lud%02luh%02lum",day,hour,minutes);
	return((char*)&st);
}




// 10ms
void timer_tenms()
{
	//printf("10ms\n"); fflush(stdout);
}


// 100ms
void timer_hundredms()
{
	//printf("100ms\n"); fflush(stdout);
	universe_timeout_check();
	recntly_idle_groups();
}



// 1000ms
void timer_onehz()
{
	static int ploopcount=0;

	//printf("1000ms\n"); fflush(stdout);
	jcp_timeout_check();
	num_devf = dev_count(DEV_F);
	num_devn = dev_count(DEV_N);

	loophz=loopcount-ploopcount;
	if (loophz<desired_loophz)                                                      // running slow
		dly=dly-10;
	if (loophz>desired_loophz)
		dly=dly+10;
	//printf("main loop %d Hz, dly=%d\n",loophz,dly); fflush(stdout);
	ploopcount=loopcount;


	// Update stats
	stats.ppps_jlp = stats.pps_jlp;
	stats.pps_jlp  = 0;
	stats.pbps_jlp = stats.bps_jlp;
	stats.bps_jlp  = 0;

	stats.ppps_jcp = stats.pps_jcp;
	stats.pps_jcp  = 0;
	stats.pbps_jcp = stats.bps_jcp;
	stats.bps_jcp  = 0;

	stats.ppps_audioplus = stats.pps_audioplus;
	stats.pps_audioplus=0;
	if (stats.ppps_audioplus==0)
		audioplus_clear_buffers();
		//audioplus_md5[0]=0;

	bin_onehz_tick();
	group_onehz_tick();
}




int te_clear(int i)
{
	if (te[i].active != TRUE)
		return(-1);
	te[i].intervalms= 0;
	te[i].rerun	= FALSE;
	te[i].lastranms	= 0;
	bzero(&te[i].cmd,sizeof(te[i].cmd));
	//printf("sizeof(te[i].cmd)=%u\n",sizeof(te[i].cmd));
	te[i].active	= FALSE;
	return(0);
}


void init_te()
{
	int i=0;

	for (i=0;i<MAX_TIMED_EVENTS;i++)
		te_clear(i);
}



// Fast as possible
void timer_fast()
{
	static unsigned int lastran_tenms=0;
	static unsigned int lastran_hundredms=0;
	static unsigned int lastran_onehz=0;
	unsigned int ms=0;
	static int pcs;
	int cs=0;
	static int teidx=0;
	char text[4096];

	ms=current_timems();
	if (ms-lastran_tenms>10)
	{
		lastran_tenms=ms;
		timer_tenms();
	}

	if (ms-lastran_hundredms>100)
	{
		lastran_hundredms=ms;
		timer_hundredms();
	}

	if (ms-lastran_onehz>1000)
	{
		lastran_onehz=ms;
		timer_onehz();
	}

	cs=get_clockseconds();
	if (cs!=pcs)
	{
		dev_subs_event(SE_NEW_SECOND);
		//printf("S"); fflush(stdout);
		pcs=cs;
		if (cs==0)
			dev_subs_event(SE_NEW_MIN);
			//printf("M"); fflush(stdout);
	}

	teidx++;
	if (teidx>=MAX_TIMED_EVENTS-1)
		teidx=0;
	if (te[teidx].active == TRUE)
	{
		if ( (unsigned int)current_timems() - te[teidx].lastranms > te[teidx].intervalms)		// due to run?
		{
			bzero(&text,sizeof(text));
			strcpy(text,te[teidx].cmd);								// interpreter destroys cmd so copy it
			te[teidx].lastranms = (unsigned int)current_timems();
			interpreter(STDOUT_FILENO, (char*)&text, sizeof(text)-1, FALSE);
		}
		if (te[teidx].rerun != TRUE)
		{
			te_clear(teidx);
		}
	}
}
 



/*
struct timed_events
{
        int             active;
        unsigned int    intervalms;                                             // run every N ms
        int             rerun;                                                  // TRUE re-run over and over
        unsigned int    lastranms;                                              // event is next due to run (ms)
        char            cmd[4096];                                              // pass to interpreter
};

*/



void te_list(int textout)
{
	int i=0;
	int a=0;

	for (i=0;i<MAX_TIMED_EVENTS;i++)
	{
		if (te[i].active==TRUE)
			a++;
	}
	xprintf(textout,"%d active timed events\n",a);

	if (a>0)
	{
		for (i=0;i<MAX_TIMED_EVENTS;i++)
		{
			if (te[i].active==TRUE)
			{
				//xprintf(textout,"%d\tintervalms=%d\trerun=%d\tcmd=[%s]\n",i, te[i].intervalms, te[i].rerun, te[i].cmd);
				xprintf(textout," %-06d ",i);
				xprintf(textout,"EVERY:%-14s ",ms_to_timetext(te[i].intervalms));
				//xprintf(textout,"%-02d ",te[i].rerun);
				xprintf(textout,"CMD:[%s]",te[i].cmd);
				xprintf(textout,"\n");
			}
		}
	}
}



void te_set(int textout, int i, int intervalms, int rerun, char* cmd)
{
	if ( i<0 || i>MAX_TIMED_EVENTS)
	{
		xprintf(textout,"te_set, %d out of range\n",i);
		return;
	}
	te[i].intervalms= intervalms;
	if (te[i].intervalms<100)
	{
		xprintf(textout,"intervalms %d too small, changed to 100ms\n",intervalms);
		te[i].intervalms = 100;
	}
	te[i].rerun	= rerun;
	te[i].lastranms = (unsigned int)current_timems();
	strcpy(te[i].cmd, cmd);
	te[i].active	= TRUE;
}



// Adds a new timed event, returns table index or -1 on error
int te_add(int textout, int intervalms, int rerun, char* cmd)
{
	int i=0;
	int x=0;

	//xprintf(textout,"te_add()\tintervalms=%d\trerun=%d\tcmd=[%s]\n",intervalms,rerun,cmd);
	x=-1;
	for (i=0;i<MAX_TIMED_EVENTS;i++)
	{
		if (te[i].active!=TRUE)
		{
			x=i;
			break;
		}
	}

	if (x>=0)
	{
		te_set(textout, x, intervalms, rerun, cmd);
	}
	return(x);
}





// Returns the index in the te table with matching command, returns index or -1
int te_find_idx_by_cmd(int textout, char*cmd)
{
	int i=0;

	for (i=0;i<MAX_TIMED_EVENTS;i++)
	{
		if (te[i].active==TRUE)
		{
			if (strncmp(te[i].cmd,cmd,sizeof(te[i].cmd)-1)==0)	// Found command in table
			{
				return(i);
			}
		}
	}
	return(-1);
}



