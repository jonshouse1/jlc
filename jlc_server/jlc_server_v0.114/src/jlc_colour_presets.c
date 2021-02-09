/*
	jlc_colour_presets.c

*/


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



// Colour table, Red, Green, Blue, White.
#define MAX_COLOUR_DEFAULTS	17
unsigned char  colour_presets_defaults[MAX_COLOUR_DEFAULTS][4] = {
	{ 0,	0,	0,	0 }, 	 	// black (unused)
	{ 0,	255,	255,	0 },		// aqua
	{ 0,	0,	255,	0 },		// blue
	{ 255,	0,	255,	0 },		// fuchsia
	{ 128,	128,	128,	0 },		// grey
	{ 0,	128,	0,	0 },		// green
	{ 0,	255,	0,	0 },		// lime
	{ 128,	0,	0,	0 },		// maroon
	{ 0,	0,	128,	0 },		// navy
	{ 255,	255,	0,	0 },		// yellow
	{ 128,	0,	128,	0 },		// purple
	{ 255,	0,	0,	0 },		// red
	{ 192,	192,	192,	0 },		// silver
	{ 0,	128,	128,	0 },		// teal
	{ 255,	255,	255,	0 },		// white
	{ 128,	128,	0,	0 },		// olive
	{ 200,	200,	255,	0 }		// warm white
};

extern struct jlc_colourpreset    clrp[MAX_COLOUR_PRESETS+1];						// colour presets 1 to N



void colour_presets_clear(int i)
{
	clrp[i].active=FALSE;
	clrp[i].R  = 0;
	clrp[i].G  = 0;
	clrp[i].B  = 0;
	clrp[i].W  = 0;
	if (i<MAX_COLOUR_DEFAULTS)
	{
		clrp[i].R = colour_presets_defaults[i][0];
		clrp[i].G = colour_presets_defaults[i][1];
		clrp[i].B = colour_presets_defaults[i][2];
		clrp[i].W = colour_presets_defaults[i][3];
		clrp[i].active=TRUE;
	}
	bin_sendmsgs(BIN_MSG_CLRP, i);
}


void init_colour_presets()
{
	int i=0;

	for (i=1;i<=MAX_COLOUR_PRESETS;i++)
		colour_presets_clear(i);
}



int colour_presets_load(int textout)
{
	FILE *fp;
	char filename[8192];
	char line[8192];
	int l=0;
	int r,g,b,w;

	sprintf(filename,"./groups/colour_presets.txt");
	fp=fopen(filename,"r");
	if (fp==NULL)
	{
		xprintf(textout,"colour_presets_load() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return(-1);
	}

	while  ( fgets((char*)&line, sizeof(line), fp) !=NULL )
	{
		if (line[0]!='#')
		{
			if (l<MAX_COLOUR_PRESETS)
			{
				l++;
				sscanf(line,"%d\t%d\t%d\t%d\n",&r,&g,&b,&w);
				clrp[l].R=r;
				clrp[l].G=g;
				clrp[l].B=b;
				clrp[l].W=w;
				clrp[l].active=TRUE;
				bin_sendmsgs(BIN_MSG_CLRP, l);
			}
		}
	}

 	monitor_printf(MSK_INF,"Loaded %d colour presets\n",l);
	fclose(fp);
	return(TRUE);
}



int colour_presets_save(int textout)
{
	FILE *fp;
	char filename[8192];
	int i=0;
	//char line[8192];

	sprintf(filename,"./groups/colour_presets.txt");
	fp=fopen(filename,"w");
	if (fp==NULL)
	{
		xprintf(textout,"colour_presets_save FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return(-1);
	}

	fprintf(fp,"# RED\tGREEN\tBLUE\tWHITE tab separated decimal 8 bit\n");
	for (i=1;i<=MAX_COLOUR_PRESETS;i++)
		fprintf(fp,"%d\t%d\t%d\t%d\n", clrp[i].R, clrp[i].G, clrp[i].B, clrp[i].W);

	fclose(fp);
	return(TRUE);
}



void colour_presets_list(int textout)
{
	int i=0;

	for (i=1;i<=MAX_COLOUR_PRESETS;i++)
	{
		if (clrp[i].active == TRUE)
		{
			xprintf(textout,"preset %02d\tR:%03d\tG:%03d\tB:%03d\tW:%03d\n",i, clrp[i].R, clrp[i].G, clrp[i].B, clrp[i].W);
		}
	}
}

