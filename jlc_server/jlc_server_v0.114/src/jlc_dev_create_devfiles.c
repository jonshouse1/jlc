/*
	jlc_dev_create_devfiles.c

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


void dev_create_file(char *filename, char *text)
{
	FILE *f;

	f=fopen(filename,"w");
	if (f==NULL)
	{
		monitor_printf(MSK_ERR,"create_file() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return;
	}
	fprintf(f,"%s",text);
	fclose(f);
}



// Test if scripts exist for this UID one by one, create any missing ones with a helpful comment
void dev_create_devfiles(unsigned char *uid, int dev_type)
{
	char filename[8192];
	char text[8192];
	char t[8192];

	sprintf(filename,"./devscripts/%s_reg",printuid(uid));
	if (access(filename, F_OK)<0)							// does registration script exist ?
	{
		text[0]=0;
		t[0]=0;
		sprintf(text,"# UID:%s TYPE:%s(%d)\n# Registration file, often maps devices to universes or groups\n",
			printuid(uid),dev_type_name(dev_type),dev_type);
		if (dev_type<200)
			sprintf(t,"\n#map %s 1 1\n", printuid(uid));			// DEV_F devices can map to univ/chan
		if (dev_type==DEVN_RELAY || dev_type==DEVN_SWITCHPBT)
			sprintf(t,"\n#map %s group 1\n", printuid(uid));		// relays can map to a group
		if (dev_type==DEVN_CLOCKCAL)
			sprintf(t,"\n#Optionally set display brightness, 0=dimmest, 15=brightest\n#dev %s state 1 15\n", printuid(uid));
		if (dev_type==DEVN_TEMPSENS)
			sprintf(t,"\n#Set temperature update rate to 30 second\n#dev %s state 0 30\n",printuid(uid));
		//if (dev_type==DEVN_MOVEMENTSENSOR)
			//sprintf(t,"\n#How often to run the devices _1f script\n#dev %s retrigger 300\n",printuid(uid));
		strcat(text,t);
		sprintf(t,"#dev %s name mydevice\n",printuid(uid));
		strcat(text,t);
		dev_create_file((char*)&filename,(char*)&text);
		monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
	}

	sprintf(filename,"./devscripts/%s_dereg",printuid(uid));
	if (access(filename, F_OK)<0)
	{
		text[0]=0;
		sprintf(text,"# UID:%s TYPE:%s(%d)\n# De-Registration file, Runs when device is removed\n",
			printuid(uid),dev_type_name(dev_type),dev_type);
		dev_create_file((char*)&filename,(char*)&text);
		monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
	}


	// Create _0 and _1 files for push button devices, expected functions differ by switch type
	switch (dev_type)
	{
		case DEVN_MOVEMENTSENSOR:
			text[0]=0;
			sprintf(filename,"./devscripts/%s_1",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _1 runs when every few seconds while movement is detected (1)\n#group N on\n#gfx <group> PFADEDOWN <delay>",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			text[0]=0;
			sprintf(filename,"./devscripts/%s_1f",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _1f runs when movement sensor is initially triggered\n#can generate an alert or play a sound\n",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			text[0]=0;
			sprintf(filename,"./devscripts/%s_0",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _0 runs when switch changes its state to off (0)\n#group N off",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
		break;

		case DEVN_SWITCH:							
			sprintf(filename,"./devscripts/%s_1",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _1 runs when switch changes its state to on (1)\n#group N on",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			sprintf(filename,"./devscripts/%s_0",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _0 runs when switch changes its state to off (0)\n#group N off",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
		break;

		case DEVN_SWITCHPBM:
			sprintf(filename,"./devscripts/%s_1",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _1 runs when button is initially pushed in(1)",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			sprintf(filename,"./devscripts/%s_0",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _0 runs when button is released",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			sprintf(filename,"./devscripts/%s_l1",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _l1 runs when switch button is held down for a while\n",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
		break;

		case DEVN_SWITCHPBT:
			sprintf(filename,"./devscripts/%s_1",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _1 runs when switch changes its state to on (1)\n#group N on",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			sprintf(filename,"./devscripts/%s_0",printuid(uid));		// device changes state to "off" = 0
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _0 runs when switch changes its state to off (0)\n#group N off",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			sprintf(filename,"./devscripts/%s_l1",printuid(uid));			// long hold while in "on" state 
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _l1 runs when switch button is held down for a while\n",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
			sprintf(filename,"./devscripts/%s_l0",printuid(uid));			// long hold while in "off" state 
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _l0 runs when switch button is held down for a while\n",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
		break;

		case DEVN_DOORBELL:
			text[0]=0;
			sprintf(filename,"./devscripts/%s_1",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _1 runs when doorbell is pushed",
					printuid(uid),dev_type_name(dev_type),dev_type);
				sprintf(t,"#log %s_%s $dateu $\\t Doorbell pushed\n",dev_type_name(dev_type), printuid(uid));
				strcat(text,t);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
		break;

		case DEVN_TEMPSENS:
			sprintf(filename,"./devscripts/%s_update",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _update runs when device sends a new value\n",
					printuid(uid),dev_type_name(dev_type),dev_type);
				sprintf(t,"#log %s_%s $dateu $\\t $%s_vb\n",dev_type_name(dev_type), printuid(uid), printuid(uid));
				strcat(text,t);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
		break;

		case DEVN_POT:
			sprintf(filename,"./devscripts/%s_update",printuid(uid));
			if (access(filename, F_OK)<0)
			{
				text[0]=0;
				sprintf(text,"# UID:%s TYPE:%s(%d)\n# _update runs when device sends a new value\n",
					printuid(uid),dev_type_name(dev_type),dev_type);
				dev_create_file((char*)&filename,(char*)&text);
				monitor_printf(MSK_INF,"file %s did not exist, made one\n",filename);
			}
		break;
	}
}



