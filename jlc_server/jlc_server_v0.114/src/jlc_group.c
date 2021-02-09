/*
	jlc_group.c

	Group is a set of universe/channels.

	Groups may be mono, RGB or RGBW. If mono only W is used
*/


// TODO:  Valid group check for every function


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlc_group.h"

extern struct    universe* uni[MAX_UNIVERSE+1];
extern uint16_t  session_id;
extern char      maskpath[MAX_PATHLEN];


extern struct jlc_group_cfg	grpc[MAX_GROUPS+1];
extern struct jlc_group_val	grpv[MAX_GROUPS+1]; 
extern struct efx               grpefx[MAX_GROUPS+1];



char* gltype_to_text(int gltype)
{
	static char gsts[32];

	bzero(&gsts,sizeof(gsts));
	sprintf(gsts,"UNKNOWN");
	switch (gltype)
	{
		//case GROUP_LIGHT_TYPE_BIT:	sprintf(gsts,"BIT");	break;
		case GROUP_LIGHT_TYPE_MONO:	sprintf(gsts,"MONO");	break;
		case GROUP_LIGHT_TYPE_RGB:	sprintf(gsts,"RGB");	break;
		case GROUP_LIGHT_TYPE_RGBW:	sprintf(gsts,"RGBW");	break;
	}
	return((char*)&gsts);
}



// Is the value "univ" with range, returns TRUE if valid
int group_valid_range(int textout, int gr, int fl)
{
	if (gr<1)
	{
		if (fl==TRUE)
			xprintf(textout,"Group must be >=1\n");
		return(FALSE);
	}
	if (gr>MAX_GROUPS)
	{
		if (fl==TRUE)
			xprintf(textout,"Group must be <=%d\n",MAX_UNIVERSE);
		return(FALSE);
	}
	return(TRUE);
}



// If a group is active then free the memory, always zero the values
void group_clear(int g)
{
	int i=0;

	if (grpc[g].noc>0)
	{
 		monitor_printf(MSK_GRP,"GROUP:%03d cleared\n",g);
 		bin_sendmsgs(BIN_MSG_GROUP_CLEAR, g);
	}

	bzero(grpc[g].name,MAX_GROUPNAME_LEN);
	grpc[g].gltype	= GROUP_LIGHT_TYPE_MONO;					// default 8 bit unsigned value (DMX style)
	grpc[g].noc	= 0;								// mark as inactive
	grpc[g].save_pending        = FALSE;

	grpv[g].R	= 0;
	grpv[g].G	= 0;
	grpv[g].B	= 0;
	grpv[g].W	= 0;
	grpv[g].I	= 0;
	grpv[g].onoff	= 0;								// off
	grpv[g].last_change_time_ms = 0;
	for (i=0;i<MAX_GROUPEL;i++)
	{
		grpc[g].univ[i]=0;
		grpc[g].chan[i]=0;
	}
}



void init_group()
{
	int i=0;

	for (i=0;i<MAX_GROUPS;i++)
		group_clear(i);
}



// For a given group print its values in different ways depending on the groups type
char* group_print_value(int g)
{
	static char pv[512];
	char st[512];

	bzero(&pv,sizeof(pv));
	sprintf(st,"ONOFF:%01d ",grpv[g].onoff);
	strcat(pv,st);

	switch (grpc[g].gltype)
	{
		case GROUP_LIGHT_TYPE_MONO:
		break;

		case GROUP_LIGHT_TYPE_RGB:
		case GROUP_LIGHT_TYPE_RGBW:
			sprintf(st,"R:%03d ", grpv[g].R);
			strcat(pv,st);
			sprintf(st,"G:%03d ", grpv[g].G);
			strcat(pv,st);
			sprintf(st,"B:%03d ", grpv[g].B);
			strcat(pv,st);
			if (grpc[g].gltype==GROUP_LIGHT_TYPE_RGBW)
			{
				sprintf(st,"W:%03d ", grpv[g].W);
				strcat(pv,st);
			}
		break;
	}

	// All group types have intensity
	sprintf(st,"I:%03d ", grpv[g].I);
	strcat(pv,st);
	
	return((char*)&pv);
}



// Set every univ/chan within a group to the groups value
void group_set_channels(int textout, int g)
{
	int c=0;

	//printf("group set channels g=%d\n",g); fflush(stdout);
	if (grpc[g].noc>0)						// if group not empty ?
	{
		for (c=0;c<grpc[g].noc;c++)				// for every univ/chan of the group
		{
			if (universe_channel_currently_available(grpc[g].univ[c], grpc[g].chan[c])<0)
			{
				xprintf(textout,"Err, group_set_channels() UNIV:%d CHAN:%d not valid\n",grpc[g].univ[c], grpc[g].chan[c]);
			}
			else	
			{
				switch (grpc[g].gltype)
				{
					// int universe_channel_set(int textout, int u, int c, int v)
					case GROUP_LIGHT_TYPE_MONO:
						if (grpv[g].onoff == 1)		// on switch is on ?
							universe_channel_set(-1, grpc[g].univ[c], grpc[g].chan[c], grpv[g].I);
						else	universe_channel_set(-1, grpc[g].univ[c], grpc[g].chan[c], 0);
					break;

					case GROUP_LIGHT_TYPE_RGB:
					case GROUP_LIGHT_TYPE_RGBW:
						if (grpv[g].onoff == 1)
						{
						}
						else
						{
						}
					break;
				}
			}
		}
	}
	else	xprintf(textout,"Err, group %d is empty\n",g);
}




// All changes to group value should go through here. Values passed as -1 are not set
int group_set_value(int textout, int g, int R, int G, int B, int W, int I, int onoff)
{
	if (group_valid_range(textout,g,TRUE)!=TRUE)
		return(-1);

	if (R>=0)
		grpv[g].R=R;
	if (G>=0)
		grpv[g].G=G;
	if (B>=0)
		grpv[g].B=B;
	if (W>=0)
		grpv[g].W=W;
	if (I>=0)
		grpv[g].I=I;
	if (onoff>=0)								// if 0 or 1 set the value
		grpv[g].onoff=onoff;

	switch (grpc[g].gltype)
        {
                case GROUP_LIGHT_TYPE_MONO:
			group_set_channels(textout, g);				// Update the lights
		break;

		case GROUP_LIGHT_TYPE_RGB:
		case GROUP_LIGHT_TYPE_RGBW:
			if (grpv[g].onoff == 1)					// On, then update the lights
				group_set_channels(textout, g);	
		break;
	}

	device_group_value_change(g);						// Notify others it has changed
	grpc[g].save_pending = TRUE;
	grpv[g].last_change_time_ms = current_timems();
 	monitor_printf(MSK_GRP,"GROUP:%03d\t%s\n",g,group_print_value(g));
 	bin_sendmsgs(BIN_MSG_GROUP_VAL, g);
	return(TRUE);
}





//int group_toggle_value(int textout, int g)
//{
	//rgbw value=0;
	//if (group_valid_range(textout,g,TRUE)!=TRUE)
		//return(-1);

	//if (grpv[g].value >0)
		//value = 0;
	//else	value = 255;
	//group_set_value(textout, g, value);
	//return(TRUE);
//}




// Load a file from disk, setup a group structure
int load_group(int textout, int g)
{
	FILE *fp;
	char filename[8192];
	char line[8192];
	char lineuc[8192];
	char ty[1024];
	int  un=0;
	int  ch=0;
	int  i=0;
	int  s=0;
	int  x=0;
	//char c=0;
	int gotline=FALSE;

	sprintf(filename,"./groups/group_%d",g);
	fp=fopen(filename,"r");
	if (fp==NULL)
	{
		//xprintf(textout,"load_group() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return(-1);
	}

	while  ( fgets((char*)&line, sizeof(line), fp) !=NULL )
	{
		gotline=FALSE;
		bzero(&lineuc,sizeof(lineuc));
		for (x=0;x<strlen(line);x++)
			lineuc[x]=toupper(line[x]);

		if (lineuc[0]=='#')
			gotline=TRUE;
		if (strncmp(lineuc,"ONO",3)==0)
		{
			sscanf(lineuc,"ONOFF\t%d\n",&x);
			if (x==0 || x==1)						// valid value ?
				grpv[g].onoff=x;
			gotline=TRUE;
		}
		if (strncmp(lineuc,"R",1)==0 && gotline!=TRUE)
		{
			sscanf(lineuc,"R\t%d\n",&x);					// sscanf needs a real int*
			grpv[g].R=x;
			gotline=TRUE;
		}
		if (strncmp(lineuc,"G",1)==0 && gotline!=TRUE)
		{
			sscanf(lineuc,"G\t%d\n",&x);
			grpv[g].G=x;
			gotline=TRUE;
		}
		if (strncmp(lineuc,"B",1)==0 && gotline!=TRUE)
		{
			sscanf(lineuc,"B\t%d\n",&x);
			grpv[g].B=x;
			gotline=TRUE;
		}
		if (strncmp(lineuc,"W",1)==0 && gotline!=TRUE)
		{
			sscanf(lineuc,"W\t%d\n",&x);
			grpv[g].W=x;
			gotline=TRUE;
		}
		if (strncmp(lineuc,"I",1)==0 && gotline!=TRUE)
		{
			sscanf(lineuc,"I\t%d\n",&x);
			grpv[g].I=x;
			gotline=TRUE;
		}

		if (lineuc[0]=='N' && lineuc[1]=='A' && gotline!=TRUE)			// Crudely parse line starting NAME 
		{
			s=strlen(line)-5;
			bzero(&grpc[g].name,sizeof(grpc[g].name));
			for (x=0;x<s;x++)
			{
				if (line[x+5]!=10 && line[x+5]!=13)
					grpc[g].name[x]=line[x+5];
			}
			gotline=TRUE;
		}

		if (lineuc[0]=='T' && lineuc[1]=='Y' && gotline!=TRUE)			// Line starting TYPE
		{
			bzero(&ty, sizeof(ty));
			s=strlen(line)-5;
			for (x=0;x<s;x++)
			{
				if (line[x+5]!=10 && line[x+5]!=13)
					strncat(ty, &lineuc[x+5], 1);
			}
			gotline=TRUE;
		}


		if (strlen(line)>3 && gotline!=TRUE)					// line not parsed yet ?
		{
			sscanf(line,"%d\t%d",&un,&ch);					// read universe and channel
			if (un>=1 && ch>=1)						// valid univ/chan?
			{
				grpc[g].univ[i] = un;	
				grpc[g].chan[i] = ch;	
				i++;
			}
		}
		line[0]=0;
	}
	grpc[g].noc = i;								// number of channels in this group	

	//printf("TYPE=%s\n",ty); fflush(stdout);
	if (strncmp(ty, "BIT", 1)==0)							// "B"IT
		grpc[g].gltype = GROUP_LIGHT_TYPE_BIT;
	if (strncmp(ty, "MONO", 1)==0)							// "M"ono
		grpc[g].gltype = GROUP_LIGHT_TYPE_MONO;
	if (strncmp(ty, "RGB", 3)==0)
	{
		if (ty[3]=='W')								// "RGBW" ?
			grpc[g].gltype = GROUP_LIGHT_TYPE_RGBW;
		else	grpc[g].gltype = GROUP_LIGHT_TYPE_RGB;
	}

	xprintf(textout,"group %d [%s] type %s contains %d channels\n",g ,grpc[g].name, ty, grpc[g].noc);
 	monitor_printf(MSK_GRP,"GROUP:%03d\t%s\n",g,group_print_value(g));
	fclose(fp);
	return(TRUE);
}






// Read all groups from files, populate structs, this can be done more than once
void group_reload(int textout)
{
	int i=0;
	int grps=0;

	for (i=1;i<MAX_GROUPS;i++)
	{
		group_clear(i);
		if (load_group(textout, i)==TRUE)
		{
			grps++;
 			bin_sendmsgs(BIN_MSG_GROUP_VAL, i);
		}
 		bin_sendmsgs(BIN_MSG_GROUP_CFG, i);
	}
	xprintf(textout,"Loaded %d groups\n",grps);
}





// Save current group settings to a text file
int save_group(int textout, int g)
{
        FILE *fp;
        char filename[8192];
	int c=0;

	//printf("save_group()  g=%d\n",g); fflush(stdout);
	if (grpc[g].noc<=0)									// group not active ?
		return(-2);
	sprintf(filename,"./groups/group_%d",g);
	fp=fopen(filename,"w");
	if (fp==NULL)
	{
		xprintf(textout,"save_group() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return(-1);
	}

        fprintf(fp,"# Univ Chan\n");
	fprintf(fp,"NAME\t%s\n",grpc[g].name);
	fprintf(fp,"TYPE\t%s\n",gltype_to_text(grpc[g].gltype));
	fprintf(fp,"ONOFF\t%d\n",grpv[g].onoff);
	fprintf(fp,"R\t%d\n",grpv[g].R);
	fprintf(fp,"G\t%d\n",grpv[g].G);
	fprintf(fp,"B\t%d\n",grpv[g].B);
	fprintf(fp,"W\t%d\n",grpv[g].W);
	fprintf(fp,"I\t%d\n",grpv[g].I);
	for (c=0;c<grpc[g].noc;c++)
	{
		fprintf(fp,"%d\t%d\n",grpc[g].univ[c], grpc[g].chan[c]);
	}
        fclose(fp);
        return(TRUE);
}





void list_group(int textout, int g)
{
	if (group_valid_range(textout, g, TRUE)!=TRUE)                                          // group not valid 
	{
		xprintf(textout,"Group %d invalid\n",g);
		return;
	}
	if (grpc[g].noc<=0)
	{
		xprintf(textout,"Group %d not active\n",g);
		return;
	}

	xprintf(textout,"GR:%-03d TYPE:%-4s NOC:%-05d",g, gltype_to_text(grpc[g].gltype), grpc[g].noc); 
	if (grpc[g].name[0]!=0)
		xprintf(textout," NAME:%-32s",grpc[g].name);
	xprintf(textout,"%s",group_print_value(g));
	xprintf(textout,"\n");
}


void list_groups(int textout)
{
	int g=0;

	//xprintf(textout,"List groups\n");
	for (g=1;g<MAX_GROUPS;g++)
	{
		if (grpc[g].noc >0)
			list_group(textout, g);
		//{
			//xprintf(textout,"GR:%-03d TY:%-4s NOC:%-05d",g, gltype_to_text(grpc[g].gltype), grpc[g].noc); 
			//if (grpc[g].name[0]!=0)
				//xprintf(textout," NAME:%-32s",grpc[g].name);
			//xprintf(textout,"%s",group_print_value(g));
			//xprintf(textout,"\n");
		//}
	}

}



void list_group_channels(int textout, int g)
{
	int c=0;

	if (grpc[g].noc>0)
	{
		xprintf(textout,"Group %d contains the following universe:channels\n",g);
		for (c=0;c<grpc[g].noc;c++)
		{
			xprintf(textout,"%d:%d\t",grpc[g].univ[c], grpc[g].chan[c]);
		}
		xprintf(textout,"\n");
	}
}





int group_count()
{
	int g=0;
	int c=0;

	for (g=1;g<MAX_GROUPS;g++)
	{
		if (grpc[g].noc >0)
			c++;
	}
	return(c);
}




int group_print(int textout, int g)
{
	if (group_valid_range(textout,g,TRUE)!=TRUE)
	{
		xprintf(textout,"Err group %d out of range\n",g);
		return(-1);
	}

	if (grpc[g].noc<=0)
	{
		xprintf(textout,"Err group %d has no univ/channels set\n",g);
		return(-1);
	}
	xprintf(textout,"group %d has %d channels\n",g,grpc[g].noc);
	return(TRUE);
}




// Once per second
void group_onehz_tick()
{
	int g=0;

	//printf("group_tick()\n"); fflush(stdout);
	for (g=1;g<MAX_GROUPS;g++)
	{
		if (grpc[g].noc >0)
		{
			if (grpc[g].save_pending==TRUE)
			{
				//printf("%llu >= %llu ?\n",current_timems(), grpv[g].last_change_time_ms + (30*1000) );
				if (current_timems() >= grpv[g].last_change_time_ms + (10 * 1000) )	// more then Nms ago ?
				{
					save_group (fileno(stdout), g);	
					grpc[g].save_pending=FALSE;
				}
			}
		}
	}
}
