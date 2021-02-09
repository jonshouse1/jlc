/*
	jlc_devices.c

	Note to self:
	Topic are per device not per IP, a machine could have multiple UDP listeners on different ports
	all on the same IP.
	Design screw up, rewrite here
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlp.h"
#include "jlc_group.h"

extern struct    universe* uni[MAX_UNIVERSE];
extern uint16_t  session_id;

// Two tables of device and device states
extern struct jlc_devf		devf[MAX_DEVF];						// Table of lighting devices
extern struct jlc_devn		devn[MAX_DEVN];						// Table of sensor devices

extern int 	movesens_ignore_retrig_periodms;
extern char     statusline[STLINE_LEN];

char playsample[1024];									// group volume and sample name

extern struct jlc_group_cfg     grpc[MAX_GROUPS+1];
extern struct jlc_group_val     grpv[MAX_GROUPS+1];





// https://stackoverflow.com/questions/18267803/how-to-correctly-convert-a-hex-string-to-byte-array-in-c?lq=1
unsigned char HexChar (char c)
{
    if ('0' <= c && c <= '9') return (unsigned char)(c - '0');
    if ('A' <= c && c <= 'F') return (unsigned char)(c - 'A' + 10);
    if ('a' <= c && c <= 'f') return (unsigned char)(c - 'a' + 10);
    return 0xFF;
}

int hextobin (const char* s, unsigned char * buff, int length)
{
    int result;
    if (!s || !buff || length <= 0) return -1;

    for (result = 0; *s; ++result)
    {
        unsigned char msn = HexChar(*s++);
        if (msn == 0xFF) return -1;
        unsigned char lsn = HexChar(*s++);
        if (lsn == 0xFF) return -1;
        unsigned char bin = (msn << 4) + lsn;

        if (length-- <= 0) return -1;
        *buff++ = bin;
    }
    return result;
}





void dumphex(const void* data, size_t size) 
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

void xdumphex(int textout, int squash, const void* data, size_t size) 
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) 
	{
		xprintf(textout,"%02X", ((unsigned char*)data)[i]);
		if (squash!=TRUE)
			xprintf(textout," ");
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') 
		{
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else 
		{
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) 
		{
			xprintf(textout," ");
			if ((i+1) % 16 == 0) 
			{
				if (squash==TRUE)
					xprintf(textout,"|%s", ascii);
				else	xprintf(textout,"|  %s \n", ascii);
			} else if (i+1 == size) 
			{
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) 
				{
					if (squash!=TRUE)
						xprintf(textout," ");
				}
				for (j = (i+1) % 16; j < 16; ++j) 
				{
					if (squash!=TRUE)
						xprintf(textout,"   ");
				}
				if (squash==TRUE)
					xprintf(textout,"|%s", ascii);
				else	xprintf(textout,"|  %s\n", ascii);
			}
		}
	}
}



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
		sprintf(st,"%02X",(unsigned char)c);
		strcat(hexstring,st);
	}
	return(hexstringp);
}



char* printhex(char *data, size_t size) 
{
	static char bfr[2048];
	char st[8];
	int i=0;

	bfr[0]=0;
	for (i=0;i<size;i++)
	{
		sprintf(st,"%02X",(unsigned char)data[i]);
		strcat(bfr,st);
	}	
	return((char*)&bfr);
}



void devf_clear(int i)
{
	if (i<MAX_DEVF)
	{
		devf[i].uid[0]		= 0;
		devf[i].ipaddr[0]	= 0;
		devf[i].jcp_udp_port	= 0;
		devf[i].jlp_udp_port	= 0;
		devf[i].univ		= 0;
		devf[i].fchan		= 0;
		devf[i].dev_name[0]	= 0; 
		devf[i].dev_type = DEV_NONE;								// last in case of threading,  mark entry as unused
	}
}



// Set a devn slot to its defaults
void devn_clear(int i)
{
	int t=0;

	if (i<MAX_DEVN)
	{
		devn[i].uid[0]		= 0;
		devn[i].ipaddr[0]	= 0;
		devn[i].jcp_udp_port	= 0;
		devn[i].jlp_udp_port	= 0;
		devn[i].group		= 0;
		devn[i].dev_name[0]	= 0; 
		bzero(&devn[i].ds, sizeof(struct dev_state));
		devn[i].last_devstate_update_timems = current_timems() - (movesens_ignore_retrig_periodms+1);
		devn[i].hide		= FALSE;
		devn[i].subscribed	= FALSE;
		for (t=0;t<MAX_TOPICS;t++)
			devn[i].topics[t]='0';
		devn[i].dev_type 	= DEV_NONE;
	}
}


void init_devf()
{
	int i=0;

        for (i=0;i<MAX_DEVF;i++)
                devf_clear(i);
}

void init_devn()
{
	int i=0;

        for (i=0;i<MAX_DEVN;i++)
                devn_clear(i);
}



int dev_find_free_slot(int t)
{
	int i=0;
	switch (t)
	{
		case DEV_F:
			for (i=0;i<MAX_DEVF;i++)
			{
				if (devf[i].dev_type == DEV_NONE)
					return(i);
			}
		break;

		case DEV_N:
			for (i=0;i<MAX_DEVN;i++)
			{
				if (devn[i].dev_type == DEV_NONE)
					return(i);
			}
		break;
	}
	return(FALSE);
}


// How many devices registered of a given type 't'
int dev_count(int t)
{
	int c=0;
	int i=0;

	switch (t)
	{
		case DEV_F:
			for (i=0;i<MAX_DEVF;i++)
			{
				if (devf[i].dev_type != DEV_NONE)
					c++;
			}
		break;
		case DEV_N:
			for (i=0;i<MAX_DEVN;i++)
			{
				if (devn[i].dev_type != DEV_NONE)
					c++;
			}
		break;
	}
	return(c);
}




char* dev_type_name(int dev_type)
{
	static char name[64];
	char *namep=&name[0];

	name[0]=0;
	switch (dev_type)
	{
		case DEVF_DMX:			sprintf(name,"DMX");			break;
		case DEVF_PIXELS:		sprintf(name,"PIXELS");			break;
		case DEVF_BITMAP:		sprintf(name,"BITMAP");			break;
		case DEVF_LIGHT:		sprintf(name,"LIGHT");			break;

		case DEVN_RELAY:		sprintf(name,"RELAY");			break;
		case DEVN_SWITCH:		sprintf(name,"SWITCH");			break;
		case DEVN_SWITCHPBT:		sprintf(name,"SWITCHPBT");		break;
		case DEVN_SWITCHPBM:		sprintf(name,"SWITCHPBM");		break;
		case DEVN_DOORBELL:		sprintf(name,"DOORBELL");		break;
		case DEVN_MOVEMENTSENSOR:	sprintf(name,"MOVESENSOR");		break;
		case DEVN_POT:			sprintf(name,"POT");			break;
		case DEVN_TEMPSENS:		sprintf(name,"TEMPSENSOR");		break;
		case DEVN_MCURRENTSENS:		sprintf(name,"MCURRENTSENS");		break;
		case DEVN_DCURRENTSENS:		sprintf(name,"DCURRENTSENS");		break;
		case DEVN_PLAYSOUNDM:		sprintf(name,"PLAYSOUNDM");		break;
		case DEVN_PLAYSOUNDS:		sprintf(name,"PLAYSOUNDS");		break;
		case DEVN_CLOCKCAL:		sprintf(name,"CLOCKCAL");		break;
		case DEVN_LEDSIGN:		sprintf(name,"LEDSIGN");		break;
		case DEVN_DUMMY:		sprintf(name,"DUMMY");			break;
		default:			sprintf(name,"Unknown!(%d)",dev_type);	break;
	}
	return(namep);
}





// Find 7 byte UID in table of type t, return the index or -1 if not found
int dev_lookup_idx_by_uid(int t, unsigned char* uid)
{
	int i=0;

	//printf("dev_lookup_idx_by_uid t=%d\tuid=%s\n",t,printuid(uid));
	switch (t)
	{
		case DEV_F:
			for (i=0;i<MAX_DEVF;i++)
			{
				if (devf[i].dev_type!=DEV_NONE)
				{
					if (memcmp(&devf[i].uid, uid, UID_LEN)==0)
						return(i);
				}
			}
		break;

		case DEV_N:
			for (i=0;i<MAX_DEVN;i++)
			{
				if (devn[i].dev_type!=DEV_NONE)
				{
					if (memcmp(&devn[i].uid, uid, UID_LEN)==0)
						return(i);
				}
			}
		break;
	}
	return(-1);
}



// Find UID (as a string) in device table, return the index
int dev_lookup_idx_by_uid_string(int t, char *uids)
{
	unsigned char uid[UID_LEN];
	int i=0;

	bzero(&uid,sizeof(uid));
	if (strlen(uids)<14)						// 7 bytes as hex text should be 14 characters
		return(-1);
	hextobin((char*)uids, (unsigned char*)&uid, UID_LEN);		// convert hex string to 7 bytes of binary
	i=dev_lookup_idx_by_uid(t, (unsigned char*)&uid);		// find in table
	return(i);							// return table index
}




int dev_lookup_idx_by_ipaddr(int t, char *ipaddr)
{
	int i=0;

	switch (t)
	{
		case DEV_F:
			for (i=0;i<MAX_DEVF;i++)
			{
				if (memcmp(&devf[i].ipaddr, ipaddr, strlen(ipaddr))==0)
					return(i);
			}
		break;

		case DEV_N:
			for (i=0;i<MAX_DEVN;i++)
			{
				//printf("[%s]==[%s]?\n",devn[i].ipaddr,ipaddr); fflush(stdout);
				if (memcmp(&devn[i].ipaddr, ipaddr, strlen(ipaddr))==0)
					return(i);
			}
		break;
	}
	return(-1);
}



char* dev_lookup_ip_from_uid(int t, char *uid)
{
	static char ip[64];
	char *ipp=&ip[0];
	int i=0;

	ip[0]=0;
	switch (t)
	{
		case DEV_F:
			for (i=0;i<MAX_DEVF;i++)
			{
				if (memcmp(&devf[i].uid, uid, UID_LEN)==0)		// Found our UID
					strcpy(ip,devf[i].ipaddr);
			}
		break;

		case DEV_N:
			for (i=0;i<MAX_DEVN;i++)
			{
				if (memcmp(&devn[i].uid, uid, UID_LEN)==0)		// Found our UID
					strcpy(ip,devn[i].ipaddr);
			}
		break;
	}
	return(ipp);
}




// Tell connected front ends about changes in device state
void dev_state_changed(struct dev_state* ds, int idx)
{
	char uids[32];

	bin_sendmsgs(BIN_MSG_DEVN, idx);						// Inform front ends of new device state
	sprintf(uids,"%s",printuid(ds->uid));						// UID as string
	if (ds->asciiorbinary==1)
		monitor_printf(MSK_DEV,"UID:%s\tTYPE:%05d\tV1:%d\tV2:%d\tAB:%d\tLEN:%d\tVB:%s\n",
				uids,ds->dev_type,ds->value1,ds->value2,ds->asciiorbinary,ds->valuebyteslen,ds->valuebytes);
	else	monitor_printf(MSK_DEV,"UID:%s\tTYPE:%05d\tV1:%d\tV2:%d\tAB:%d\tLEN:%d\tVB:%s\n",
				uids,ds->dev_type,ds->value1,ds->value2,ds->asciiorbinary,ds->valuebyteslen,printhex(ds->valuebytes,ds->valuebyteslen));
}




// Give a device a name
void dev_set_name(int textout, char* uids, char* name)
{
	int i=0;
	int t=DEV_F;
	int l=strlen(name);

	i=dev_lookup_idx_by_uid_string(t, uids);
	if (i<0)									// index starts at 0, negative is error
	{
		t=DEV_N;
		i=dev_lookup_idx_by_uid_string(t, uids);
	}
	if (i<0)
	{
		xprintf(textout,"Cant find device with UID:%s\n",uids);
		return;
	}
	if (l>DEV_NAME_LEN-1)
		l=DEV_NAME_LEN-1;

	if (t==DEV_F)
	{
		bzero(devf[i].dev_name, DEV_NAME_LEN);
		strncpy(devf[i].dev_name, name, l);
		xprintf(textout,"set DEVF device UID:%s to NAME:%s\n",uids, name);
	}
	else
	{
		bzero(devn[i].dev_name, DEV_NAME_LEN);
		strncpy(devn[i].dev_name, name, l);
		monitor_printf(MSK_NDE,"UID:%s\tTYPE:%05d\tNAME:%s\n",uids, devn[i].dev_type, devn[i].dev_name);
		dev_state_changed(&devn[i].ds, i);
	}
}



// Got a new device to register
void dev_jlc_register(struct jcp_dev *d ,char *ipaddr)
{
	int i=0;
	int t=0;
	char filename[8192];

	if (d->dev_type<100)								// Fixture ?
	{
		i=dev_lookup_idx_by_uid(DEV_F,d->uid);					// Do we already have this device in table ?
		if (i<0)								// no, find a new table slot and create it
		{
			i=dev_find_free_slot(DEV_F);
		}
		devf[i].dev_type 	= d->dev_type;					// update table entry
		memcpy(&devf[i].uid, d->uid, UID_LEN);
		strcpy((char*)&devf[i].ipaddr, ipaddr);
		strcpy(devf[i].dev_name,printuid(devf[i].uid));				// register script should overwrite this later
		devf[i].jcp_udp_port	= d->jcp_udp_port;
		devf[i].jlp_udp_port	= d->jlp_udp_port;;
		devf[i].noc=d->noc;
		if (d->noc<=0)
			monitor_printf(MSK_ERR,"Err, UID:%s registered d->dev_type=%d  registered a dev_f device with noc=%d\n",printuid(d->uid),d->dev_type,d->noc);
		devf[i].last_send_time_ms = current_timems();
		devf[i].update_rate_hz = 40;
		strcpy(devf[i].device_model, d->device_model);
		strcpy(devf[i].device_fw_ver, d->device_fw_ver);
		monitor_printf(MSK_REG,"UID:%s\tTYPE:%05d\tACT:REG\n",printuid((unsigned char*)&devf[i].uid),d->dev_type);
		bin_sendmsgs(BIN_MSG_DEVF, i);						// Inform front ends of new device state
	}
	else										// else sensor
	{
		i=dev_lookup_idx_by_uid(DEV_N,d->uid);
		if (i<0)
		{
			i=dev_find_free_slot(DEV_N);
		}
		devn[i].dev_type 	= d->dev_type;
		memcpy(&devn[i].uid, d->uid, UID_LEN);
		strcpy((char*)&devn[i].ipaddr, ipaddr);
		strcpy(devn[i].dev_name, printuid(devn[i].uid));
		devn[i].jcp_udp_port	= d->jcp_udp_port;
		devn[i].jlp_udp_port	= d->jlp_udp_port;;
		devn[i].group		= 0;

		strcpy(devn[i].device_model, d->device_model);
		strcpy(devn[i].device_fw_ver, d->device_fw_ver);
		devn[i].subscribed	= FALSE;
		for (t=0;t<MAX_TOPICS;t++)
		{
			devn[i].topics[t] = d->topics[t];
			if (devn[i].topics[t]=='1')
				devn[i].subscribed = TRUE;				// Set flag if any topic is set
		}

		// devn[i] also holds a copy of the device state. Fill it in with default values, will be overwritten when device reports state
		bzero(&devn[i].ds, sizeof(struct dev_state));
		devn[i].ds.dev_type = d->dev_type;
		memcpy(&devn[i].ds.uid, d->uid, UID_LEN);
		monitor_printf(MSK_REG,"UID:%s\tTYPE:%05d\tACT:REG\tMOD:%s\tFWVER:%s\n",
				printuid((unsigned char*)&devn[i].uid), devn[i].dev_type, devn[i].device_model, devn[i].device_fw_ver);
		bin_sendmsgs(BIN_MSG_DEVN, i);						// Inform front ends of new device state
	}
	// Registering a device runs a script, the script normally sets up a mapping for the device
	dev_create_devfiles(d->uid, d->dev_type);					// create any missing files
	sprintf(filename,"%s_reg",printuid(d->uid));					// registration script for device
	run_script(-1, filename);

	for (t=0;t<MAX_TOPICS;t++)							// for every possible topic
	{
		if (devn[i].topics[t]=='1')						// is device subscribed to topic ?
			dev_send_topic(i, t);						// then send it an update
	}
}




// Remove all references to a device, notify peers of its demise 
void dev_deregister(int t, int i)
{
	char filename[8192];
	switch (t)
	{
		case DEV_F:
			sprintf(filename,"%s_dereg",printuid(devf[i].uid));
			run_script(-1, filename);
			monitor_printf(MSK_REG,"UID:%s\tTYPE:%05d\tACT:REM\tNAME:%s\n",
					printuid((unsigned char*)&devf[i].uid), devf[i].dev_type, devf[i].dev_name);
			bin_sendmsgs(BIN_MSG_DEVF_CLEAR, i);
			devf_clear(i);								// always last
		break;

		case DEV_N:
			sprintf(filename,"%s_dereg",printuid(devn[i].uid));
			run_script(-1, filename);
			monitor_printf(MSK_REG,"UID:%s\tTYPE:%05d\tACT:REM\tNAME:%s\n",
					printuid((unsigned char*)&devn[i].uid), devn[i].dev_type, devf[i].dev_name);
			bin_sendmsgs(BIN_MSG_DEVN_CLEAR, i);
			devn_clear(i);
		break;
	}
}




#define NUMOFNAMES	14
char devtypes[NUMOFNAMES*2][32]={	"RELAY","400",
					"SWITCH", "500",
					"SWITCHPBT","501",
					"SWITCHPBM","502",
					"DOORBELL", "510",
					"MOVESENSOR", "520",
					"POT", "540",
					"TEMPSENSOR", "560",
					"CURRENTSENS", "600",
					"PLAYSOUNDM", "700",
					"PLAYSOUNDSAMPLE", "701",
					"CLOCKCAL", "800",
					"LED_SIGN", "900",
					"DUMMY", "999" };

// name must be upper case ASCII, int is -1 on error or positive if dev_type is found
int lookup_dev_type_from_name(char *devtypename)
{
	int i=0;
	int v=0;

	//printf("lookup_dev_type_from_name()  devtypename=[%s]\n",devtypename); fflush(stdout);
	for (i=0;i<NUMOFNAMES*2;i=i+2)
	{
		//printf("[%s]==[%s]?\n",devtypename,devtypes[i]);
		if (strcmp(devtypename,devtypes[i])==0)				// found it ?
		{
			v=atoi(devtypes[i+1]);
			//printf("devname %s = %d\n",devtypename,v);
			return(v);
		}
	}
	return(-1);
}



// List model and version info for devices
void list_device_ver(int textout, int devtype)
{
	int i=0;
	if (devtype==99999)
	{
		for (i=0;i<MAX_DEVF;i++)
		{
			if (devf[i].dev_type != DEV_NONE)
			{
				xprintf(textout,"IP:%-15s ",devf[i].ipaddr);
				xprintf(textout,"UID:%s ",printuid(&devf[i].uid[0]));
				xprintf(textout,"MOD:%-22s",devf[i].device_model);
				xprintf(textout,"FWVER:%s ",devf[i].device_fw_ver);
				if (strlen(devf[i].dev_name)>0)
					xprintf(textout,"NAME:%-16s",devf[i].dev_name);
				xprintf(textout,"\n");
			}
		}
	}

	for (i=0;i<MAX_DEVN;i++)
	{
		if (devn[i].dev_type==devtype && devn[i].hide!=TRUE)
		{
			xprintf(textout,"IP:%-15s ",devn[i].ipaddr);
			xprintf(textout,"UID:%s ",printuid(&devn[i].uid[0]));
			xprintf(textout,"MOD:%-22s",devn[i].device_model);
			xprintf(textout,"FWVER:%s ",devn[i].device_fw_ver);
			if (strlen(devn[i].dev_name)>0)
				xprintf(textout,"NAME:%-16s",devn[i].dev_name);
			xprintf(textout,"\n");
		}
	}
}





// List all devices of devtypename.
// most devices are DEV_N, cheat, if devtypename is DEV_F then list fixture devices, otherwise list DEV_N device with matching devtypename
// if fulldetails is not true then use a shorter summary
void list_device(int textout, int devtype, int fulldetails)
{
	int i=0;
	int show=TRUE;

	if (devtype==99999)
	{
		for (i=0;i<MAX_DEVF;i++)
		{
			if (devf[i].dev_type != DEV_NONE)
			{
				xprintf(textout," F ");
				if (fulldetails==TRUE)				
					xprintf(textout,"IP:%-15s ",devf[i].ipaddr);
				xprintf(textout,"UID:%s ",printuid(&devf[i].uid[0]));
				xprintf(textout,"%-10s ",dev_type_name(devf[i].dev_type));
				xprintf(textout,"NOC:%-04d", devf[i].noc);
				if (fulldetails==TRUE)
				{
					xprintf(textout,"JCP:%-05d ",devf[i].jcp_udp_port);
					xprintf(textout,"JLP:%-05d ",devf[i].jlp_udp_port);
					xprintf(textout,"UDR:%-03d", devf[i].update_rate_hz);
				}
				if (devf[i].univ>0)
				{
					xprintf(textout,"MAPPED U:%d FCHAN:%d\t",devf[i].univ, devf[i].fchan);
				}
				if (strlen(devf[i].dev_name)>0)
					//xprintf(textout,"NAME:%-16s",devf[i].dev_name);
					xprintf(textout,"NAME:%s",devf[i].dev_name);
				xprintf(textout,"\n");
			}
		}
		return;
	}


	for (i=0;i<MAX_DEVN;i++)
	{
		// Work out which entries to show
		show=TRUE;
		if (devn[i].hide == TRUE)					// if it is hidden 
			show=FALSE;						// don't show
		if (fulldetails ==TRUE )					// unless it is a full listing
			show=TRUE;						// in that case show everything
		if (show == TRUE)
		{
			if (devn[i].dev_type==devtype)
			{
				if (devn[i].hide==TRUE)
				{
					if (iscolour(textout)==TRUE)
						xprintf(textout,"%c[38;2;%d;%d;%dm",27,140,140,140);
					xprintf(textout,"HN");
				}
				else	xprintf(textout," N");
				xprintf(textout," ");
				if (fulldetails==TRUE)
					xprintf(textout,"IP:%-15s ",devn[i].ipaddr);
				xprintf(textout,"UID:%s ",printuid(&devn[i].uid[0]));
				xprintf(textout,"%-10s ",dev_type_name(devn[i].dev_type));
				if (fulldetails==TRUE)
				{
					xprintf(textout,"JCP:%-05d ",devn[i].jcp_udp_port);
					xprintf(textout,"JLP:%-05d ",devn[i].jlp_udp_port);
				}
				if (strlen(devn[i].dev_name)>0)
					xprintf(textout,"NAME:%-16s",devn[i].dev_name);
				if (devn[i].group>=1)
					xprintf(textout,"MAPPED GROUP:%d ",devn[i].group);
				xprintf(textout,"\n");
				if (iscolour(textout)==TRUE)
					xprintf(textout,"%c[38;2;%d;%d;%dm",27,255,255,255);
			}
		}
	}
}




// List of all devices, excludes device state
// For neatness group devices by type
// W1 device name  W2 optional [Full]
// or W1 [Full]
void list_devices(int textout, char* w1, char* w2)
{
	int i=0;
	int full=FALSE;
	int listall=FALSE;
	int listver=FALSE;
	int devt=0;

	xprintf(textout,"List devices: %d devices,  %d F devices, %d N devices\n", dev_count(DEV_F)+dev_count(DEV_N), dev_count(DEV_F), dev_count(DEV_N));
	if ( (strlen(w1)<=0 && strlen(w2)<=0) )							// no args at all ?
		listall=TRUE;
	if (strlen(w2)<=0 && w1[0]=='F')							// no arg2 and arg1=(F)ull ?
	{
		full=TRUE;
		listall=TRUE;
	}
	if (strncmp(w2,"FULL",1)==0)								// arg2 is (F)ull?
		full=TRUE;
	if (strncmp(w1,"DEVF",5)==0)								// explicitly asked for DEVF devices only
	{
		list_device(textout, 99999, full);						// list DEV_F devices only
		return;
	}
	if (strncmp(w1,"VER",1)==0)								// list Versions
	{
		listall=TRUE;
		listver=TRUE;
	}

	if (listall==TRUE)									// list all devices
	{
		if (listver!=TRUE)
			list_device(textout, 99999, full);					// list DEV_F devices 
		else	list_device_ver(textout, 99999);					// list all dev_f device versions
		for (i=0;i<NUMOFNAMES*2;i=i+2)							// list each type of DEV_N device one type at a time
		{
			devt=lookup_dev_type_from_name(devtypes[i]);
			if (listver!=TRUE)
				list_device(textout, devt, full); 
			else	list_device_ver(textout, devt);
		}
	}
	else
	{
		if (listver==TRUE)
			return;
		devt=lookup_dev_type_from_name(w1);
		if (devt<0)
		{
			xprintf(textout,"Err, cant find devtypename=%s\n",w1);
			return;
		}
		list_device(textout, devt, full); 
	}
}



// show state for a device
void list_dev_s(int textout, int idx)
{
	//int allzero=TRUE;
	//int i=0;
	unsigned long int tdiff=0;

	if (devn[idx].dev_type != DEV_NONE)
	{
		xprintf(textout," UID:%s ",printuid(&devn[idx].uid[0]));
		xprintf(textout,"%-10s ",dev_type_name(devn[idx].dev_type));
		xprintf(textout,"V1:%-05d V2:%-05d ",devn[idx].ds.value1, devn[idx].ds.value2);

		tdiff=current_timems() - devn[idx].last_devstate_update_timems;
		xprintf(textout,"AGE:%-11s ",ms_to_timetext(tdiff));

		if (devn[idx].ds.valuebyteslen > 0)
		{
			xprintf(textout,"VB:");
			if (devn[idx].ds.asciiorbinary == 1)						// ASCII ?
				xprintf(textout,"[%s] ",devn[idx].ds.valuebytes);			// display it as a string
			else	xdumphex(textout, TRUE, &devn[idx].ds.valuebytes[0], devn[idx].ds.valuebyteslen);	// show as hex and ASCII
			xprintf(textout,"\t");
		}
		if (devn[idx].ds.valuebyteslen>MAX_VALUEBYTESLEN)					// sanity check
		{
			printf("list_dev_s() Err  UID:%s has valuebyteslen(%d) > >MAX_VALUEBYTESLEN(%d)\n",
				printuid(&devn[idx].uid[0]),devn[idx].ds.valuebyteslen,MAX_VALUEBYTESLEN);
			exit(1);
		}


		if (strlen(devn[idx].dev_name)>0)
				xprintf(textout,"NAME:%-16s",devn[idx].dev_name);

		if (devn[idx].group>=1)
			xprintf(textout,"MAPPED GROUP:%d ",devn[idx].group);
		xprintf(textout,"\n");
	}
}







// List state of devices, if uids is blank then list state of all devices ordered by type
void list_dev_state(int textout, char *uids)
{
	int i=0;
	int c=0;
	int devt=0;

	//xprintf(textout, "List device state\n");
	xprintf(textout,"List device state: %d devices,  %d F devices, %d N devices\n", dev_count(DEV_F)+dev_count(DEV_N), dev_count(DEV_F), dev_count(DEV_N));
	if (strlen(uids)<=0)								// no UID supplied
	{
		for (c=0;c<NUMOFNAMES*2;c=c+2)						// list each type of DEV_N device one type at a time
		{
			devt=lookup_dev_type_from_name(devtypes[c]);
			for (i=0;i<MAX_DEVN;i++)					// all devices
			{
				if (devn[i].dev_type == devt && devn[i].hide !=TRUE)	// if device is our type
					list_dev_s(textout, i);				// show it
			}
		}
		return;
	}
	i=dev_lookup_idx_by_uid_string(DEV_N, uids);					// find device
	if (i<0)
	{
		xprintf(textout,"Cant find device with UID %s\n",uids);
		return;
	}
	list_dev_s(textout, i);
}




// Output a list of devices in the same format "mon devices" uses.  Used to sync clients to the 
// state of the system
void list_dev_monitor(int textout)
{
	int i=0;
	struct dev_state* ds=NULL;
	char uids[32];

	uids[0]=0;
	for (i=0;i<MAX_DEVN;i++)
	{
		if (devn[i].dev_type>0 && devn[i].hide!=TRUE)
		{
			ds=&devn[i].ds;
			xprintf(textout,"*DEV %s ",datetimes(5));
			sprintf(uids,"%s",printuid(ds->uid));				// UID as string
			if (ds->asciiorbinary==1)
				xprintf(textout,"UID:%s\tTYPE:%05d\tV1:%d\tV2:%d\tAB:%d\tLEN:%d\tVB:%s\n",
						uids,ds->dev_type,ds->value1,ds->value2,ds->asciiorbinary,ds->valuebyteslen,ds->valuebytes);
			else	xprintf(textout,"UID:%s\tTYPE:%05d\tV1:%d\tV2:%d\tAB:%d\tLEN:%d\tVB:%s\n",
						uids,ds->dev_type,ds->value1,ds->value2,ds->asciiorbinary,ds->valuebyteslen,printhex(ds->valuebytes,ds->valuebyteslen));

		}
	}
}




void list_dev_names(int textout)
{
	int i=0;

	for (i=0;i<MAX_DEVF;i++)
	{
		if (devf[i].dev_type != DEV_NONE)
		{
			xprintf(textout,"F UID:%s TYPE:%04d NAME:%s\n", printuid(devf[i].uid), devf[i].dev_type, devf[i].dev_name);
		}
	}

	for (i=0;i<MAX_DEVN;i++)
	{
		if (devn[i].dev_type != DEV_NONE)
		{
			xprintf(textout,"N UID:%s TYPE:%04d NAME:%s\n", printuid(devn[i].uid),devn[i].dev_type, devn[i].dev_name);
		}
	}
}


// List temperatures
void list_temperatures(int textout)
{
	int i=0;
	int t=0;

	for (i=0;i<MAX_DEVN;i++)
		if (devn[i].dev_type == DEVN_TEMPSENS)
			t++;
	if (t==0)
	{
		xprintf(textout,"No temperature sensor devices currently registered\n");
		return;
	}
	for (i=0;i<MAX_DEVN;i++)
	{
		if (devn[i].dev_type == DEVN_TEMPSENS)
		{
			xprintf(textout," N ");
			xprintf(textout,"UID:%s ",printuid(&devn[i].uid[0]));
			xprintf(textout,"%-10s ",dev_type_name(devn[i].dev_type));
			xprintf(textout,"\n");
		}
	}
}






// This IP timed out, remove all devices that are registered with it
// TODO:  run de-register script for device here
void dev_timeout(char *ipaddr)
{
	int idx=0;

	//printf("dev_timeout %s\n\n",ipaddr); fflush(stdout);
	do									// keep looking in tables for ip until no more
	{
		idx=dev_lookup_idx_by_ipaddr(DEV_F, ipaddr);
		if (idx>=0)
			dev_deregister(DEV_F, idx);
	} while (idx>=0);							// keep going until we get a -1

	do
	{
		idx=dev_lookup_idx_by_ipaddr(DEV_N, ipaddr);
		if (idx>=0)
			dev_deregister(DEV_N, idx);
	} while (idx>=0);
}





// The value of a group has changed, notify devices
void device_group_value_change(int g)
{
	int i=0;
	int tellfrontends=FALSE;

	//printf("device_group_value_change() g=%d\n",g); fflush(stdout);
	for (i=0;i<MAX_DEVN;i++)						// for every DEV_N device
	{
		if (devn[i].dev_type != DEV_NONE)				// device active ?
		{
			if (devn[i].group == g)					// device mapped to this group ?
			{
				switch (devn[i].dev_type)			// pick devices whose state we can set
				{
					case DEVN_SWITCH:			// latching switch, no point in telling it a state
					break;

					case DEVN_SWITCHPBT:			// push button toggle switch, update its state
						if (devn[i].ds.value1 != grpv[g].onoff)
						{				// Only if value has changed
							devn[i].ds.value1 = grpv[g].onoff;
							tellfrontends=TRUE;
						} 
					break;

					case DEVN_RELAY:			// relay state tracks group value
						if (devn[i].ds.value1 != grpv[g].onoff)
						{				// Only if value has changed
							devn[i].ds.value1 = grpv[g].onoff;
							tellfrontends=TRUE;
						} 
					break;

					case DEVN_POT:
						//devn[i].ds.value1 = value;
					break;
				}
				jcp_send_payload(devn[i].ipaddr, devn[i].jcp_udp_port, JCP_MSG_DEV_STATE, (char*)&devn[i].ds, sizeof(struct dev_state));
				if (tellfrontends==TRUE)
					dev_state_changed(&devn[i].ds, i);	// Notify front ends
			}
		}
	}
}



// map <uid> to <univ> <fist channel>
void device_map_uc(int textout, char *uids, int univ, int fchan)
{
	int i=0;

	i=dev_lookup_idx_by_uid_string(DEV_F, uids);
	if (i<0)
	{
		xprintf(textout,"device_map_uc() Err, cant find UID or not DEV_F device %s\n",uids);
		return;
	}
	devf[i].univ  = univ;
	devf[i].fchan = fchan;
	xprintf(textout,"device UID:%s mapped to univ %d first channel %d\n",printuid(&devf[i].uid[0]), devf[i].univ, devf[i].fchan);
}




// List all devices that are mapped to universes
void list_map(int textout)
{
	xprintf(textout,"device_list_map\n");
}





// Map a group onto a device, mapping to group=0 removes mapping
void device_map_group(int textout, char *uids, int group)
{
	int i=0;

	i=dev_lookup_idx_by_uid_string(DEV_N, uids);
	if (i<0)
	{
		xprintf(textout,"device_map_group() Err, cant find UID or not DEV_N device %s\n",uids);
		return;
	}

	if ( (devn[i].dev_type!=DEVN_RELAY) && (devn[i].dev_type!=DEVN_SWITCHPBT) && (devn[i].dev_type!=DEVN_SWITCHPBM) )
	{
		xprintf(textout,"map_group ignored, device UID:%s dev_type %s makes mapping pointless\n",printuid(&devn[i].uid[0]), dev_type_name(devn[i].dev_type));
		return;
	} 
	devn[i].group=group;	
	device_group_value_change(group);					// send state to all group registered devs
	xprintf(textout,"device UID:%s mapped to group %d [%s]\n",printuid(&devn[i].uid[0]), devn[i].group, grpc[group].name);
}



// Called by jcp.c, server received a message containing  device state
void device_update_state(char *ipaddr, struct dev_state* ds)
{
	char uids[32];
	int idx=0;
	char filename[8192];
	unsigned int tdiff=0;							// time difference

	sprintf(uids,"%s",printuid(ds->uid));					// UID as string
	idx=dev_lookup_idx_by_uid(DEV_N, ds->uid);				// find device in table
	if (idx<0)
	{
		monitor_printf(MSK_ERR,"device_update_state() Err, Device UID:%s not registered\n",uids);
		return;
	}


	// Save new device state locally
	memcpy((char*)&devn[idx].pds, (char*)&devn[idx].ds, sizeof(struct dev_state));
	memcpy((char*)&devn[idx].ds, ds, sizeof(struct dev_state));		// Take a copy of the new state

	switch (ds->dev_type)
	{
		case DEVN_SWITCH:
		break;

		case DEVN_SWITCHPBT:						// value1 is switch state, value2 is 1 if long push
			if (ds->value2 != 0)					// long push
			{
				sprintf(filename,"%s_l%d",printuid(ds->uid),ds->value1);
				run_script(-1, filename);
				return;
			}
			sprintf(filename,"%s_%d",printuid(ds->uid),ds->value1);
			run_script(-1, filename);
		break;


// FIXME....
// add a re-trigger interval to the device state itself , retrigger time   "dev UID rt <Seconds>" 
		case DEVN_MOVEMENTSENSOR:
			tdiff = (unsigned int)current_timems() - devn[idx].last_devstate_update_timems;
			//if (ds->value1 == 1)
				//slprintf("UID:%s\tMOVESENSOR (%s) triggered",printuid((unsigned char*)&devn[idx].uid),devn[idx].dev_name);

			monitor_printf(MSK_DEV,"MOVEMENTSENSOR state change to %d,  tdiff=%u\n",ds->value1,tdiff);
			if ( (tdiff > 1000 * 60 * 5) && (ds->value1 == 1) )	// device has been idle for at least 5 mins
			{
				sprintf(filename,"%s_1f",printuid(ds->uid));
				run_script(-1, filename);			// run the _1f script
			}
			sprintf(filename,"%s_%d",printuid(ds->uid),ds->value1);
			run_script(-1, filename);				// always run the _1 or _0 script
		break;




		case DEVN_DOORBELL:
			sprintf(filename,"%s_%d",printuid(ds->uid),ds->value1);
			run_script(-1, filename);				// Just one event, button is pushed, run _1 script for device
		break;


		case DEVN_TEMPSENS:
			sprintf(filename,"%s_update",printuid(ds->uid));	// script normally updates the log file
			run_script(-1, filename);				// Just one event, button is pushed, run _1 script for device
		break;


		case DEVN_MCURRENTSENS:
		break;


		case DEVN_DCURRENTSENS:
		break;
	}
	devn[idx].last_devstate_update_timems = (unsigned int)current_timems();	// note time of update, must do this last
	dev_state_changed(ds, idx);
}




// Hide or unhide a device
void dev_hide(int textout, char*uids, char *value1s)
{
	int i=0;
	i=dev_lookup_idx_by_uid_string(DEV_N, uids);				// Find the device
	if (i<0)
	{
		xprintf(textout,"dev_hide() Failed, Can not find DEV_N device with UID %s\n",uids);
		return;
	}
	if (strlen(value1s)>0)
	{
		if (value1s[0]=='U')						// unhide ?
		{
			devn[i].hide=FALSE;
			xprintf(textout,"device UID:%s unhidden\n",uids);
			return;
		}
	}
	devn[i].hide=TRUE;
	xprintf(textout,"device UID:%s hidden from all but full lists\n",uids);
}





// Update a devices state, this new state is first updated locally then later sent to the device
void dev_set_state(int textout, char*uids, char *value1s, char* value2s, char *valuebytess)
{
	int i=0;
	int l=0;
	int v=0;
	int ps=0;

	i=dev_lookup_idx_by_uid_string(DEV_N, uids);				// Find the device
	if (i<0)
	{
		xprintf(textout,"dev_set_state() Failed, Can not find DEV_N device with UID %s\n",uids);
		return;
	}

	devn[i].ds.value1=0;
	if (strlen(value1s)>0)
	{
		v=atoi(value1s);
		devn[i].ds.value1 = v;
	}

	devn[i].ds.value2=0;
	if (strlen(value2s)>0)
	{
		v=atoi(value2s);
		devn[i].ds.value2 = v;
	}

	bzero(&devn[i].ds.valuebytes, MAX_VALUEBYTESLEN);
	l=strlen(valuebytess);
	if (l>0)
	{
		if (l>sizeof(devn[i].ds.valuebytes)-1)
			l=sizeof(devn[i].ds.valuebytes)-1;
		memcpy(&devn[i].ds.valuebytes, valuebytess, l);
		devn[i].ds.asciiorbinary = 1;								// 1=ASCII
		devn[i].ds.valuebyteslen = l;
	}

	ps=sizeof(struct dev_state) - sizeof(devn[i].ds.valuebytes);					// size of packet with no VB data
	ps=ps+devn[i].ds.valuebyteslen+1;								// add back on actual VB length
	jcp_send_payload(devn[i].ipaddr, devn[i].jcp_udp_port, JCP_MSG_DEV_STATE, (char*)&devn[i].ds, ps);
	device_update_state(devn[i].ipaddr, &devn[i].ds);
}






// ****** FIXME *******
// Hack until message ACK and retry from server to client is implemented.
// At a rate of 10Hz check all groups for those that have recently hit a value of 0
// Continue to notify devices mapped to those groups of the zero value for a few seconds
void recntly_idle_groups()
{
	int g=0;
	unsigned int tdiff=0;

	for (g=0;g<MAX_GROUPS;g++)
	{
		if (grpv[g].onoff == 0)									// only if "off", stop if group starts up again
		{
			tdiff=current_timems() - grpv[g].last_change_time_ms;
			if (tdiff < 1000 * 8)								// N seconds ?
			{
				//printf("group %d changed to 0 %u ms ago\n",g,tdiff); fflush(stdout);
				device_group_value_change(g);						// Tell everyone group is off
			}
		}
	}
}




void dev_config(int textout, char*uids, char *cmd, char*text)
{
	int i=0;
	char payload[8192];
	int pl=0;

	if (strlen(cmd)<=0)
		return;

	i=dev_lookup_idx_by_uid_string(DEV_N, uids);							// Find the device
	if (i<0)
	{
		xprintf(textout,"dev_set_state() Failed, Can not find DEV_N device with UID %s\n",uids);
		return;
	}

	bzero(&payload,sizeof(payload));
	if (strncmp(cmd,"LOCATION",1)==0)
	{
		sprintf(payload,"L%s",text);
		pl=strlen(payload);
		jcp_send_payload(devn[i].ipaddr, devn[i].jcp_udp_port, JCP_MSG_DEV_CONFIG, (char*)&payload, pl);
		printf("sent location [%s] %d bytes\n",text,pl); fflush(stdout);
	}
}



// Send dev_n device an update on this topic
void dev_send_topic(int i, int topic)
{
	struct jcp_msg_topic	mt;
	unsigned int pl=0;

	if (devn[i].dev_type == DEV_NONE)
	{
		printf("dev_send_topic() FAIL\n");
		exit(1);
	}
	bzero(&mt,sizeof(struct jcp_msg_topic));
	bzero(&mt.tdata,sizeof(mt.tdata));
	memcpy(mt.uid, devn[i].uid, UID_LEN);
	mt.topic = topic;
	mt.tlen = 0;
	switch (topic)
	{
		case JCP_TOPIC_STATUSLINE:
			strcpy((char*)&mt.tdata, statusline);
			mt.tlen=strlen((char*)&mt.tdata);
		break;

		case JCP_TOPIC_DEVICEREG:
		break;

		case JCP_TOPIC_DATETIME_B_S:
		break;

		case JCP_TOPIC_DATETIME_B_M:
		break;

		case JCP_TOPIC_DATETIME_S_S:
			sprintf((char*)&mt.tdata,"%s", datetimes(4));
			mt.tlen=strlen((char*)&mt.tdata);
		break;

		case JCP_TOPIC_DATETIME_S_M:
			sprintf((char*)&mt.tdata,"%s", datetimes(4));
			mt.tlen=strlen((char*)&mt.tdata);
		break;

		case JCP_TOPIC_SND:
			if (playsample[0]!=0)								// we have a new sample to play ?
			{
				sprintf((char*)&mt.tdata,"%s", playsample);
				mt.tlen=strlen((char*)&mt.tdata);
			}
		break;
	}

	if (mt.tlen>0)
	{
		pl=sizeof(struct jcp_msg_topic);							// the largest it could ever be
		pl=pl-(MAX_TDATA_LEN - mt.tlen);							// shorten payload to actual length of tdata[]
		jcp_send_payload(devn[i].ipaddr, devn[i].jcp_udp_port, JCP_MSG_TOPIC, (char*)&mt, pl);
	}
}



// Find all active devices subscribed to a given topic
void devs_send_topic(int topic)
{
	int i=0;

	for (i=0;i<MAX_DEVN;i++)									// for every devn device registered
	{
		if (devn[i].dev_type != DEV_NONE && devn[i].subscribed == TRUE)				// active device, with active topic(s)
		{
			if (devn[i].topics[topic]=='1')							// is this topic active for this device ?
				dev_send_topic(i, topic);						// then send it an update
		}
	}
}





// When something changes this tells us
void dev_subs_event(int etype)
{
	switch (etype)
	{
		case SE_STATUSLINE:
			devs_send_topic(JCP_TOPIC_STATUSLINE);
		break;
		case SE_NEW_SECOND:
			devs_send_topic(JCP_TOPIC_DATETIME_B_S);
			devs_send_topic(JCP_TOPIC_DATETIME_S_S);
		break;
		case SE_NEW_MIN:
			devs_send_topic(JCP_TOPIC_DATETIME_B_M);
			devs_send_topic(JCP_TOPIC_DATETIME_S_M);
		break;
		case SE_DEV_REGISTER:
		break;
		case SE_DEV_TIMEOUT:
		break;

		case SE_PLAYSOUND:
			devs_send_topic(JCP_TOPIC_SND);
		break;
	}
}





// Subscribe to topics
void list_subs(int textout, char *w)
{
	int i=0;
	int t=0;

	xprintf(textout,"List subscriptions\n");
	for (i=0;i<MAX_DEVN;i++)
	{
		if (devn[i].dev_type != DEV_NONE)
		{
			xprintf(textout,"UID:%s ",printuid(&devn[i].uid[0]));
			xprintf(textout,"TYPE:%-11s ",dev_type_name(devn[i].dev_type));

			xprintf(textout,"SUB:");
			for (t=0;t<MAX_TOPICS;t++)
				xprintf(textout,"%c",devn[i].topics[t]);				// should contain '0' or '1' ASCII	
			if (strlen(devn[i].dev_name)>0)
				xprintf(textout," NAME:%-16s",devn[i].dev_name);
			xprintf(textout,"\n");
		}
	}
}



// Sound sound sample play instructions to machines
void dev_playsound(int g, int v, char* samplename)
{
	monitor_printf(MSK_SND,"G:%d\tV:%d\tSAM:%s\n",g,v,samplename);					// tell all TCP clients
	sprintf(playsample,"%d %d %s",g,v,samplename);
	dev_subs_event(SE_PLAYSOUND);									// tell all subscribed UDP clients
	bzero(&playsample,sizeof(playsample));								// ensure we don't re-send it
}

