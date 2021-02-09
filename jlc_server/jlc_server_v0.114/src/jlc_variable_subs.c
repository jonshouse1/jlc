/*
	jlc_variable_subs.c
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlp.h"
#include "jlc_group.h"


// Console
extern uint32_t console_m;
extern char	cprompt[MAX_PROMPTLEN];
extern char	version[];
extern int	cclr;
extern int	loophz;

// TCP connections
extern struct tcp_connection	conns[MAX_TCPCONN];

// Groups
extern struct jlc_group_cfg     grpc[MAX_GROUPS+1];
extern struct jlc_group_val     grpv[MAX_GROUPS+1];


extern struct statss		stats;
extern char			statusline[STLINE_LEN];


// Two tables of device and device states
extern struct jlc_devf		devf[MAX_DEVF];                                                         // Table of lighting devices
extern struct jlc_devn		devn[MAX_DEVN];                                                         // Table of sensor devices




// KB, MB, GB - A kilobyte (KB) is 1,024 bytes. A megabyte (MB) is 1,024 kilobytes. A gigabyte (GB) is 1,024 megabytes. A terabyte (TB) is 1,024 gigabytes.
void printbytes(int textout, int bytes)
{
	unsigned int b=bytes;

	if (bytes<1024)
	{
		xprintf(textout,"%d bytes");
		return;
	}
	b=b/1024;

	if (b<1024)
	{
		//xprintf(textout,"%d KB",b);
		xprintf(textout,"%d K bytes",b);
		return;
	}

	if (b>1024)
	{
		//xprintf(textout,"%d MB",b/1024);
		xprintf(textout,"%d Mega bytes",b/1024);
		return;
	}
	
}




// Find any $variable and replace with value
// format is $UID_V1  or $UID_V2  or $UID_VB
// $  B  8  2  7  E  B  B  F  3  9  1  E  0  1  _  1
// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16

// Return TRUE if a substitution was made
int variable_subs(char *word)
{
	int  i=0;
	int  l=0;
	char uids[16];
	char name[16];

	l=strlen(word);
	if (l<=0)
		return(FALSE);

	//printf("wrd=%s\n",word); fflush(stdout);
	if (word[0]!='$')										// starts with a $ sign ?
		return(FALSE);

	bzero(&uids,sizeof(uids));
	bzero(&name,sizeof(name));
	strncpy((char*)&uids,word+1,14);								// copy in UID string on its own
	strcpy((char*)&name,word+16);
	//printf("uids=[%s] name=[%s]\n",uids,name); fflush(stdout);

	if (strcmp((char*)&name,"V1")==0)
	{
		i=dev_lookup_idx_by_uid_string(DEV_N, (char*)&uids);
		if (i>=0)
		{
			sprintf(word,"%d",devn[i].ds.value1);
			return(TRUE);
		}
	}
	if (strcmp((char*)&name,"V2")==0)
	{
		i=dev_lookup_idx_by_uid_string(DEV_N, (char*)&uids);
		if (i>=0)
		{
			sprintf(word,"%d",devn[i].ds.value1);
			return(TRUE);
		}
	}
	if (strcmp((char*)&name,"VB")==0)
	{
		i=dev_lookup_idx_by_uid_string(DEV_N, (char*)&uids);
		if (i>=0)
		{
			if (devn[i].ds.asciiorbinary==1)
			{
				sprintf(word,"%s",devn[i].ds.valuebytes);
				return(TRUE);
			}
		}
	}
//NEW
	if (strcmp((char*)&name,"VT")==0)								// Temperature "22.1c"
	{
		i=dev_lookup_idx_by_uid_string(DEV_N, (char*)&uids);
		if (i>=0)
		{
			if (devn[i].ds.asciiorbinary==1)
			{
				if (devn[i].ds.valuebytes[1]=='0')					// remove leading 0
					sprintf(word,"%sc",devn[i].ds.valuebytes+2);
				else	sprintf(word,"%sc",devn[i].ds.valuebytes+1);
				return(TRUE);
			}
		}
	}
	if (strcmp((char*)&name,"VTS")==0)								// Temperature, short "22c"
	{
		i=dev_lookup_idx_by_uid_string(DEV_N, (char*)&uids);
		if (i>=0)
		{
			if (devn[i].ds.asciiorbinary==1)
			{
				l=atoi(devn[i].ds.valuebytes+1);
				//if (devn[i].ds.valuebytes[1]=='0')					// remove leading 0
					//sprintf(word,"%sc",devn[i].ds.valuebytes+2);
				//else	sprintf(word,"%sc",devn[i].ds.valuebytes+1);
				sprintf(word,"%dc",l);
				return(TRUE);
			}
		}
	}
	if (strcmp((char*)&name,"VTN")==0)								// Temperature, short "7"
	{
		i=dev_lookup_idx_by_uid_string(DEV_N, (char*)&uids);
		if (i>=0)
		{
			if (devn[i].ds.asciiorbinary==1)
			{
				l=atoi(devn[i].ds.valuebytes+1);
				sprintf(word,"%d",l);
				return(TRUE);
			}
		}
	}


	//if (strncmp(word,"$SP",2)==0)									// Hard space
	//{
		//word[0]=0;
		//return(TRUE);
	//}


	if (strncmp(word,"$DATE1",6)==0)								// Tue 28 Apr 2020 12:50:34 BST
	{
		sprintf(word,"%s",datetimes(1));
		return(TRUE);
	}
	if (strncmp(word,"$DATE2",6)==0)								// Tue 28 Apr 2020 12:51:06 +0100
	{
		sprintf(word,"%s",datetimes(2));
		return(TRUE);
	}
	if (strncmp(word,"$DATE3",6)==0)								// Tue 28 Apr 2020 12:51:24
	{
		sprintf(word,"%s",datetimes(3));
		return(TRUE);
	}
	if (strncmp(word,"$DATE4",6)==0)								// 2 28/03/2020 12:51:40
	{
		sprintf(word,"%s",datetimes(4));
		return(TRUE);
	}
	if (strncmp(word,"$DATE5",6)==0)								// 28/03/2020 12:51:57
	{
		sprintf(word,"%s",datetimes(5));
		return(TRUE);
	}
	if (strncmp(word,"$DATEL",6)==0)								// Date alone, for log files
	{
		sprintf(word,"%s",datetimes(20));
		return(TRUE);
	}
	if (strncmp(word,"$DATEU",6)==0)								// UTC date/time
	{
		sprintf(word,"%s",datetimes(30));
		return(TRUE);
	}
	if (strncmp(word,"$DATE",4)==0)									// Tue 28 Apr 2020 12:52:17 +0100
	{
		sprintf(word,"%s",datetimes(2));
		return(TRUE);
	}
	if (strncmp(word,"$TIMES",5)==0)								// 12:52
	{
		sprintf(word,"%s",datetimes(12));
		return(TRUE);
	}
	if (strncmp(word,"$TIME",4)==0)									// 12:52:30
	{
		sprintf(word,"%s",datetimes(11));
		return(TRUE);
	}
	if (strncmp(word,"$SEC",3)==0)									// 30
	{
		sprintf(word,"%s",datetimes(13));
		return(TRUE);
	}


//NEW
	if (strncmp(word,"$DNAME",5)==0)								// Day name "Tue"
	{
		sprintf(word,"%s",datetimes(40));
		return(TRUE);
	}
	if (strncmp(word,"$DOM",4)==0)									// Day of month "25 May"
	{
		sprintf(word,"%s",datetimes(41));
		return(TRUE);
	}
	if (strncmp(word,"$YEAR",5)==0)									// Year "2020"
	{
		sprintf(word,"%s",datetimes(42));
		return(TRUE);
	}


	// Printf style stuff
	if (strncmp(word,"$\\T",3)==0)
	{
		sprintf(word,"\t");
		return(TRUE);
	}
	//printf("failed to substitute\n"); fflush(stdout);
	return(FALSE);
}



void variable_list(int textout)
{
	xprintf(textout,"Variable\tExample\n");
	xprintf(textout,"$<UID>_V1\t100\n");
	xprintf(textout,"$<UID>_V2\t101\n");
	xprintf(textout,"$<UID>_VB\t+07.1\n");
	xprintf(textout,"$<UID>_VT\t7.1c\n");
	xprintf(textout,"$<UID>_VTS\t7c\n");
	xprintf(textout,"$<UID>_VTN\t7\n");
	xprintf(textout,"$DATE\t\tTue 28 Apr 2020 23:29:59 +0100\n");
	xprintf(textout,"$TIME\t\t23:30:16\n");
	xprintf(textout,"$TIMES\t\t23:30\n");
	xprintf(textout,"$SEC\t\t30\n");
	xprintf(textout,"$DATE1\t\tTue 28 Apr 2020 23:30:30 BST\n");
	xprintf(textout,"$DATE2\t\tTue 28 Apr 2020 23:30:43 +0100\n");
	xprintf(textout,"$DATE3\t\tTue 28 Apr 2020 23:30:56\n");
	xprintf(textout,"$DATE4\t\t2 28/03/2020 23:31:08\n");
	xprintf(textout,"$DATE5\t\t28/03/2020 23:31:21\n");
	xprintf(textout,"$DATEL\t\t28_03_2020\n");
	xprintf(textout,"$DATEU\t\tTue Apr 28 22:31:46 2020 UTC\n");
	xprintf(textout,"$DNAME\t\tTue\n");
	xprintf(textout,"$DOM\t\t28 Apr\n");
	xprintf(textout,"$YEAR\t\t2020\n");
}


/*
List device state
 UID:5CCF7F3B542202 RELAY      V1:0   V2:0   AGE:00:10:44     NAME:shed-relay      
 UID:5CCF7F3B542201 SWITCHPBT  V1:0   V2:0   AGE:00:10:44     NAME:shed-pb         
 UID:000F540E046801 DOORBELL   V1:0   V2:0   AGE:00:10:44     
 UID:6001942BABC101 MOVESENSOR V1:0   V2:0   AGE:00:10:44     NAME:Door PIR        
 UID:A020A6163C5F01 MOVESENSOR V1:0   V2:0   AGE:00:00:11     NAME:Kitchen PIR     
 UID:5CCF7F865A3301 MOVESENSOR V1:0   V2:0   AGE:00:10:44     NAME:Garden PIR      
 UID:A020A616367901 MOVESENSOR V1:0   V2:0   AGE:00:10:44     NAME:Drive PIR       
 UID:E1520800008086 TEMPSENSOR V1:101 V2:0   AGE:00:00:24     VB:[+16.8] NAME:shed-outside    
 UID:FF5767A2160404 TEMPSENSOR V1:101 V2:0   AGE:00:00:24     VB:[+18.8] NAME:shed-inside     
 UID:BC1BCB06000014 TEMPSENSOR V1:101 V2:0   AGE:00:00:37     VB:[+21.3] NAME:understairs     
 UID:000F540E046802 PLAYSOUNDS V1:0   V2:0   AGE:00:10:44     NAME:doorbell-speaker
 UID:ECFABC20BF1101 CLOCKCAL   V1:0   V2:0   AGE:00:10:44     NAME:waisroom        
 UID:A020A617D9D701 CLOCKCAL   V1:0   V2:0   AGE:00:10:44     NAME:livingroom      
 UID:B827EBBF391E01 LEDSIGN    V1:0   V2:0   AGE:00:10:44     VB:[dev B827EBBF391E01 state 0 0] NAME:6linesign       
*/
