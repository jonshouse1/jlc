/*
	jlc_runscript.c

	Read lines from a file and pass each line to the command interpreter
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"

extern char script_path[MAX_PATHLEN];
extern int flag_tracecode;
extern struct jlc_devn		devn[MAX_DEVN];								// Table of sensor devices



// Returns TRUE if file was found and read
// pass textfd=-1 for silent output
// re=TRUE, relative path, false filename is absolute path
int run_script(int textout, char *scriptname)
{
	FILE *lsqfile=NULL;
	char filename[MAX_FILENAMELEN];
	char textline[8192];
	int  running=TRUE;
	int  ir=0;											// interpreter result
	int  ln=0;											// line number
	int  idx=-1;
	char uids[64];

	sprintf(filename,"%s/%s",script_path,scriptname);						// Try scriptpath first
	if (access( filename, F_OK )==-1)								// file not exist ?
	{
		sprintf(filename,"devscripts/%s",scriptname);						// then try devscripts path instead
	}

	lsqfile=fopen(filename,"r");
	if (lsqfile==NULL)
	{
		xprintf(textout,"run_script() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		monitor_printf(MSK_SCR,"Script %s not found [%s]\n",filename,strerror(errno));
		return(FALSE);
	}

	monitor_printf(MSK_SCR,"[%s]\n",filename);
	running=TRUE;
	do
	{
		textline[0]=0;
		if (fgets((char*)&textline,sizeof(textline)-2,lsqfile)<=0)				// get next line from file
			running=FALSE;
		else
		{											// we got a line
			ln++;										// line number starting at 1
			if (flag_tracecode==TRUE)
				xprintf(textout,"%s:  [%s",filename,textline);
			ir=interpreter(textout,(char*)&textline,sizeof(textline)-2,FALSE);
			if (ir==SCRIPT_STOP)								// interpreter cmd explicitly stopped script?
				running=FALSE;
		}
	}
	while (running==TRUE);
	if (ir==SCRIPT_STOP)
	{
		strcpy(uids,scriptname);								// script name starts with the UID, copy it
		uids[14]=0;										// shorten it
		idx=dev_lookup_idx_by_uid_string(DEV_N, (char*)&uids);
		if (idx>=0)										// name lookup worked ?
			xprintf(textout,"%s script %s (%s) stopped\n",datetimes(30),filename,devn[idx].dev_name);
		else	xprintf(textout,"%s script %s stopped\n",datetimes(30),filename);		// else just leave out the device name
	}
	fclose(lsqfile);
	return(TRUE);
}
