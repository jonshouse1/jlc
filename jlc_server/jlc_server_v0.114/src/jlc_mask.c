/*
	jlc_mask.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <ctype.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"

extern struct    universe* uni[MAX_UNIVERSE+1];
extern uint16_t  session_id;

extern char      mask_path[MAX_PATHLEN];


// Returns 0 if c is within set, otherwise returns c
int isoneoff(char c, char *set)
{
	int sl=strlen(set);
	int i=0;
	int inset=0;

	inset=FALSE;
	for (i=0;i<sl;i++)
	{
		if (c==set[i])
			inset=TRUE;
	}
	if (inset!=TRUE)
		return(c);
	return(0);
}


// Returns the invalid character or 0 if all is good
int mask_valid(char *ptrn)
{
	int  	ptrnlen=strlen(ptrn);
	char*	validmask=VALID_MASK;
	int	i=0;

	if (ptrnlen<=0)
		return(FALSE);
	for (i=0;i<ptrnlen;i++)
	{
		if (isoneoff(ptrn[i],validmask)!=0)
			return(ptrn[i]);
	}
	return(0);
}


// Is the mask character valid
int mask_valid_char(char m)
{
	char*	validmask=VALID_MASK;
	if (isoneoff(m,validmask)!=0)
		return(FALSE);
	else	return(TRUE);
}



// Turn a mask character into the same character in ANSI colour,  v is optional intensity
char* masktoansi(char m, int bright)
{
	char c;
        static char ansiseq[64];
        char *ansiseqp=&ansiseq[0];

	int r=0;
	int g=0;
	int b=0;
	int br=0;

	br=bright-1;
	ansiseq[0]=0;
	c=toupper(m);
	switch (c)
	{
		//TODO: replace 'R' with MASK_R etc
		case 'R':	r=1+br;		g=0;		b=0;		break;
		case 'G':	r=0;		g=1+br;		b=0; 		break;
		case 'B':	r=0;		g=0;		b=1+br;		break;
		case 'Y':	r=1+br;		g=1+br;		b=0;  		break;
		case 'W':	r=1+br;		g=1+br;		b=1+br;		break;
		//case 'O':	r=24;		g=9;		b=7;		break;
		//case 'P':	r=24;		g=0;		b=24;		break;
		//case 'I':	r=24;		g=2;		b=15;		break;
		//case 'U':	r=24;		g=13;		b=24;		break;
		//case 'X':	r=21;		g=21;		b=21;		break;
		case 'X':	r=0;		g=0;		b=0;		break;
	}

	sprintf(ansiseq,"%c[48;2;%d;%d;%dm%c",27,r,g,b,c);		// background
	return(ansiseqp);
}
//#define MASK_KEY        "R=Red  G=Green  B=Blue  Y=Yellow  W=White  O=Orange  P=Purple  I=pInk  U=UV  X=Unused"



// MASK CREATE <univ num> <num of channels> <pattern>
int mask_create(int textout, int uni, int noc, char*ptrn)
{
	int i=0;
	char filename[MAX_FILENAMELEN];
	char name[32];
	FILE *f=NULL;
	int  linewidth=64;
	int  l=0;
	int  c=0;
	int  ptrnlen=strlen(ptrn);

	if (universe_valid_range(textout, uni, TRUE)!=TRUE)
		return(FALSE);
	if (channel_valid_range(textout, noc, TRUE)!=TRUE)
		return(FALSE);
	c=mask_valid(ptrn);
	if (c!=0)
	{
		xprintf(textout,"mask_create() Pattern contains illegal character %c\n",c);
		return(FALSE);
	}


	sprintf(name,"%d",uni);
	sprintf(filename,"%s/MASK_%s",mask_path,name);
	printf("mask_create() got filename [%s]\n",filename); fflush(stdout);

	f=fopen(filename,"w");
	if (f==NULL)
	{
		xprintf(textout,"mask_create() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return(FALSE);
	}

	fprintf(f,"UNIV:%d\n",uni);
	fprintf(f,"NOC :%d",noc);
	c=1;
	l=0;
	for (i=0;i<noc;i++)
	{
		if ( (c % linewidth == 0) || (i==0) )				// This is a new line
		{
			//printf("c=%d\n",c);
			fprintf(f,"\n%d:\t",(l*linewidth)+1);			// output line number
			l++;
			c=0;
		}
		fprintf(f,"%c",ptrn[i%ptrnlen]);
		c++;
	}
	fprintf(f,"\n\n#KEY: %s\n",MASK_KEY);	
	fclose(f);

	return(TRUE);
}


void mask_show(int textout, int univ)
{
	char filename[MAX_FILENAMELEN];
	FILE *f=NULL;
	char line[8192];

	if (universe_valid_range(textout, univ, TRUE)!=TRUE)
		return;

	sprintf(filename,"%s/MASK_%d",mask_path,univ);
	f=fopen(filename,"r");
	if (f==NULL)
	{
		xprintf(textout,"mask_show() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return;
	}
	xprintf(textout,"file [%s]\n",filename);

//char *fgets(char *s, int size, FILE *stream);
	while  ( fgets((char*)&line, sizeof(line), f) !=NULL )
		xprintf(textout,"%s",line);
	fclose(f);
	//xprintf(textout,"\n");
}
