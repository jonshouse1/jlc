/*
	jlc_universe.c

	Universe related functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"

extern struct    universe* uni[MAX_UNIVERSE+1];
extern int 	 blankcount_top;
extern uint16_t  session_id;
extern char      mask_path[MAX_PATHLEN];




// Is the value "univ" with range ?
int universe_valid_range(int textout, int univ, int fl)
{
	if (univ<1)
	{
		if (fl==TRUE)
			xprintf(textout,"Universe must be >=1\n");
		return(FALSE);
	}
	if (univ>MAX_UNIVERSE)
	{
		if (fl==TRUE)
			xprintf(textout,"Universe must be <=%d\n",MAX_UNIVERSE);
		return(FALSE);
	}
	return(TRUE);
}


// Is the channel or number of channels valid ?
int channel_valid_range(int textout, int ch, int fl)
{
	if (ch<0)
	{
		if (fl==TRUE)
			xprintf(textout,"Channel or 'Number of channels' must be >0\n");
		return(FALSE);
	}
	if (ch>MAX_CHANNEL)
	{
		if (fl==TRUE)
			xprintf(textout,"Channel or 'Number of channels' must be <=%d\n",MAX_CHANNEL);
		return(FALSE);
	}
	return(TRUE);
}



// returns negative if this universe/channel is currently available in the system ? 0 value = ok
int universe_channel_currently_available(int u, int c)
{
	// univ/chan within allowable bounds ?
	if (universe_valid_range(-1, u, FALSE)!=TRUE)
		return(-1);
	if (channel_valid_range(-1, c, FALSE)!=TRUE)
		return(-2);

	if (uni[u] == NULL)
		return(-3);					// universe not currently available
	if (c > uni[u]->noc)					// channel more than the number of channels in this universe?
		return(-4);
	return(0);
}



// Each channel is a single 8 bit value v
int universe_channel_set(int textout, int u, int c, int v)
{
	if (universe_valid_range(textout,u,FALSE)!=TRUE)
		return(FALSE);
	if (channel_valid_range(textout,c,FALSE)!=TRUE)
		return(FALSE);

	//printf("universe_channel_set u=%d  c=%d  v=%d\n",u,c,v); fflush(stdout);
	if (uni[u]==NULL)
	{
		xprintf(textout,"universe_channel_set() univ:%d not active\n",u);
		return(FALSE);
	}
	uni[u]->ch[c].value=v;
	if (v>0)
	{
		uni[u]->active = TRUE;
		uni[u]->blankcount=blankcount_top;
	}
	uni[u]->last_changed = (uint32_t)current_timems();
	return(TRUE);
}



// Each channel is a single 8 bit value
int universe_return_ch_value(int textout, int u, int c)
{
	if (universe_valid_range(-1, u, FALSE)==TRUE)
	{
		if (channel_valid_range(-1, c, FALSE)==TRUE)
		{
			if (uni[u]!=NULL)
				return(uni[u]->ch[c].value);
			else 	return(-1);						// univ not loaded 
		}
		else 	return(-2);							// chan not valid range
	}
	else return(-3);								// univ not valid range
}




// Set every channel of univ u to value v, returns the number of channels set or -1 on error
int universe_set_allchan(int textout, int u, int v)
{
	int i=0;
	int n=0;

	if (universe_valid_range(textout,u,FALSE)!=TRUE)
		return(-1);

	if (uni[u]==NULL)
	{
		xprintf(textout,"universe_channel_set() univ:%d does not exist\n",u);
		return(-1);
	}

	for (i=1;i<=uni[u]->noc;i++)							// for every channel
	{
		uni[u]->ch[i].value=v;							// set value
		n++;
	}
	return(n);
}



// Set colours of universe u to value v, returns the number of channels set or -1 on error
int universe_set_colours(int textout, int u, char* colours, int v)
{
	char*   validmask=VALID_MASK;
	int n=0;
	//int i=0;
	int c=0;
	int ma=0;
	int masklen=strlen(validmask);
	int cs=0;
	int colourslen=strlen(colours);

	//printf("universe_set_colours()  u=%d  v=%d  colourslen=%d   colours=%s  mask=%s  masklen=%d\n",u,v,colourslen,colours,validmask,masklen); fflush(stdout);
	if (universe_valid_range(textout,u,FALSE)!=TRUE)
		return(-1);
	//if (uni[u]->active!=TRUE)
		//return(-1);
		
	for (cs=0;cs<=colourslen;cs++)							// for every character in 'colours'
	{
		for (c=1;c<=uni[u]->noc;c++)						// for every channel of universe u
		{

			for (ma=0;ma<masklen;ma++)					// for every character of mask
			{

				//printf("%c==%c?\n",uni[u]->ch[c].mask,colours[ma]); fflush(stdout);
				if (uni[u]->ch[c].mask==colours[ma])			// got a maching colour mask?
				{
					uni[u]->ch[c].value=v;
					n++;
				}
			}
		}
	}
	return(n);
}



/*

//universe_channels_set(textout, atoi(wordsuc[1]), 0, 0);
// if m==MSK_R set all 'R' to value v, if m=0 set all non 'X' channels
//int universe_channels_set(int textout, int u, char m, int v)
int universe_channels_set(int textout, int u, char m, rgbw v)
{
	int i=0;

	if (universe_valid_range(textout,u,FALSE)!=TRUE)
		return(FALSE);

	if (uni[u]==NULL)
	{
		xprintf(textout,"universe_channel_set() univ:%d does not exist\n",u);
		return(FALSE);
	}

	//printf("trying to set some of %d channels to %d  m=%c\n",uni[u]->noc,v,m); fflush(stdout);
	for (i=1;i<=uni[u]->noc;i++)							// for every channel
	{
		if ( (uni[u]->ch[i].mask != 'X') && (uni[u]->ch[i].mask == m || m=='A') )		// masks match, or mask 'A'll
		{
			//printf("ch[%d]\n",i); fflush(stdout);
			uni[u]->ch[i].value=v;						// then set value
		}
		if (uni[u]->ch[i].value>0)
		{
			uni[u]->active = TRUE;
			uni[u]->blankcount=blankcount_top;
		}
	}
	uni[u]->last_changed = (uint32_t)current_timems();
	return(TRUE);
}
*/



int universe_destroy(int u)
{
	if (uni[u]==NULL)
	{
		fprintf(stderr,"destroy_universe() Err, universe %d does not exist?\n",u);
		fflush(stderr);
		return(FALSE);
	}
	free(uni[u]);
	uni[u]=NULL;
	return(TRUE);
}


void universe_initial_settings(int u)
{
	int c=0;

	uni[u]->active=FALSE;
	uni[u]->blankcount=0;
	uni[u]->last_changed = 0;
	for (c=0;c<uni[u]->noc;c++)
	{
		uni[u]->ch[c].value=0;
		//uni[u]->ch[c].fx.fx=FX_NONE;
	}
}


// Create universe u of c channels, return the number of bytes it occupies or zero on error
int universe_create(int u, int noc)
{
	int bytes=0;

	if (u<0 || u>MAX_UNIVERSE)
		return(FALSE);
	if (uni[u]!=NULL)
		universe_destroy(u);							// create or re-create

	// Channel 0 is unused, so need one more than the number of channels
	bytes=sizeof(struct universe) + (sizeof(struct channel) * (noc+1));
	uni[u]=malloc(bytes);
	if (uni[u]==NULL)
		return(0);								// malloc failed, crash out at this point ?
	bzero(uni[u],bytes);
	uni[u]->bytes=bytes;
	uni[u]->noc=noc;
	universe_initial_settings(u);
	return(bytes);
}


// Is anything going on with this universe ?
int universe_check_if_idle(int u)
{
	int c=0;
	int idle=FALSE;

	if (universe_valid_range(-1,u,FALSE)!=TRUE)
		return(-1);

	if (uni[u]==NULL)
		return(-1);

	if (uni[u]->active==FALSE)
		return(FALSE);

	for (c=0;c<uni[u]->noc;c++)
		if (uni[u]->ch[c].value!=0)
			idle=FALSE;
	return(idle);
}



// How many universes are created
int universe_count(int mode)
{
	int i=0;
	int c=0;


	for (i=0;i<MAX_UNIVERSE;i++)
	{
		if (mode==UNIVERSE_CREATED)
		{
			if (uni[i]!=NULL)
				c++;
		}
		if (mode==UNIVERSE_ACTIVE)
		{
			if (uni[i]!=NULL)
			{
				if (uni[i]->active==TRUE)
					c++;
			}
		}
	}
	return(c);
}



/*
UNIV:1
NOC :512
1:      GBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWG
65:     BWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGB
129:    WGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBW
193:    GBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWG
257:    BWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGB
321:    WGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBW
385:    GBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWG
449:    BWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGBWGB

univ=1 u=1 noc=512
allocated 1170 bytes of memory for universe 1 of 512 channels
ch[1].mask=N
Warning N not valid mask character
ch[2].mask=O
ch[3].mask=C
Warning C not valid mask character
ch[4].mask=G
*/


// Load a pre-saved universe mask file,  call universe_create to malloc enough space,  parse mask file and populate  univ[x]...  
int universe_load(int textout, int u)
{
	FILE *f=NULL;
	char filename[MAX_FILENAMELEN];
	char line[8192];
	int univ=0;
	int noc=0;
	int i;
	int oset;
	int cc=1;		// channel count
	int didcreate=FALSE;
	int bytes=0;

	if (universe_valid_range(textout, u, TRUE)!=TRUE)
		return(FALSE);

	sprintf(filename,"%s/MASK_%d",mask_path,u);
	f=fopen(filename,"r");
	if (f==NULL)
	{
		xprintf(textout,"universe_load() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		return(FALSE);
	}


	// Read it
	while  ( fgets((char*)&line, sizeof(line), f) !=NULL )
	{
		if (strncmp(line,"UNIV:",5)==0)
			sscanf((char*)&line, "UNIV:%d",&univ);
		if (strncmp(line,"NOC",3)==0)
			sscanf((char*)&line, "NOC :%d",&noc);

		if (univ!=0 && noc !=0 && didcreate!=TRUE)
		{
			//printf("univ=%d u=%d noc=%d\n",univ,u,noc); fflush(stdout);
			bytes=universe_create(u, noc);					// allocate memory for universe
			didcreate=TRUE;
			if (bytes >0)
				xprintf(textout,"allocated %d bytes of memory for universe %d of %d channels\n",bytes,univ,noc);
			else
			{
				xprintf(textout,"universe_create() malloc failed, %d bytes\n",bytes);
				return(FALSE);
			}
		}
		else
		{
			if (line[0]!='#' && didcreate==TRUE)
			{
				for (i=0;i<32;i++)
				{
					if (line[i]==9)
						oset=i;					// Find the offset into line of the tab
				}

				for (i=oset; i<strlen(line); i++)
				{
					if (line[i]<128 && line[i]>'A' && line[i]!=10 && line[i]!=13)
					{
						//printf("ch[%d].mask=%c\n",cc,line[i]); fflush(stdout);
						uni[univ]->ch[cc].value=0;
						if (mask_valid_char(line[i])!=TRUE)
							xprintf(textout,"Warning %c not valid mask character\n",line[i]);
						uni[univ]->ch[cc].mask=line[i];	
						cc++;
					}
				}
			}
			line[0]=0;
		}
	}
	fclose(f);
	xprintf(textout,"load %d channels into universe %d\n",cc-1,u);
	return(TRUE);
}



// details about universe u, or u=0=all
void list_universes(int textout, int u)
{
	int i=0;
	int l=0;		// low, first univ	
	int h=0;		// high last univ

	xprintf(textout,"Universes:\n");
	h=MAX_UNIVERSE;
	if (u!=0)
	{
		if (universe_valid_range(-1,u,FALSE)!=TRUE)
			return;
		l=u;
		h=u;
	}

	//printf("l=%d h=%d i=%d\n",l,h,i); fflush(stdout);
	for (i=l;i<=h;i++)
	{
		if (uni[i]!=NULL)				// active slot ?
		{
			xprintf(textout," univ:%-04d ",i);
 			xprintf(textout,"noc:%-08d",uni[i]->noc);
			xprintf(textout,"Kbytes: %-05d ",uni[i]->bytes/1024);
			if (uni[i]->active==TRUE)
				xprintf(textout,"active");
			else	xprintf(textout,"inactive");
			xprintf(textout,"\n");
		}
	}
}



// Show details about universe u
void universe_show(int textout, int u, int iscolour)
{
	int c;							// channels
	int lwc=0;

	if (universe_valid_range(textout, u, TRUE)!=TRUE)
                return;

	if (uni[u]==NULL)
	{
		xprintf(textout,"No univ %d\n",u);
		return;
	}
	xprintf(textout,"Universe %d has %d channels\n",u,uni[u]->noc);

	xprintf(textout,"1:\t");
	for (c=1;c<=uni[u]->noc;c++)
	{
//printf("c=%d\n",c); fflush(stdout);
		if (iscolour==TRUE)
			xprintf(textout,"%s",masktoansi(uni[u]->ch[c].mask, uni[u]->ch[c].value));	
		else	xprintf(textout,"%c",uni[u]->ch[c].mask);
		lwc++;
		if (lwc>=64 && c!=uni[u]->noc)
		{
			if (iscolour==TRUE)
				//xprintf(textout,"%c[38;2;255;255;255m",27);
				xprintf(textout,"%c[0m",27);			// default background colour
			xprintf(textout,"\n%d:\t",c+1);
			lwc=0;
		}
	}
	if (iscolour==TRUE)
		//xprintf(textout,"%c[38;2;255;255;255m\n",27);			// white text
		xprintf(textout,"%c[0m\n",27);					// default background colour
	else	xprintf(textout,"\n");
}






void universe_dump(int textout, int u)
{
	int ch=0;
	int c=0;

	if (universe_valid_range(textout, u, TRUE)!=TRUE)
                return;
	if (uni[u]==NULL)
	{
		xprintf(textout,"No univ %d\n",u);
		return;
	}
	xprintf(textout,"Universe %d has %d channels, ",u,uni[u]->noc);
	if (uni[u]->active==TRUE)
		xprintf(textout,"active\n");
	else	xprintf(textout,"inactive\n");

	c=0;
	xprintf(textout,"%5d: ",1);
	for (ch=1;ch<=uni[u]->noc;ch++)
	{
		xprintf(textout,"%c%-03d ",uni[u]->ch[ch].mask, uni[u]->ch[ch].value);
		c++;
		if (c>=14)
		{
			c=0;
			xprintf(textout,"\n%5d: ",ch+1);
		}
	}
	xprintf(textout,"\n");
}



// pick next universe available, is it active, if so it all black
void universe_timeout_check()
{
	static int uc=0;                                                                // universe counter
	int c=0;
	int allblack=TRUE;

	for (uc=1;uc<MAX_UNIVERSE;uc++)
	{
		if (uni[uc] != NULL)							// pointer points to some RAM?
		{
			if (uni[uc]->active == TRUE)					// marked active ?
			{
				for (c=1;c<uni[uc]->noc;c++)				// for every channel
				{
					if (uni[uc]->ch[c].value!=0)			// is it not black
						allblack=FALSE;
				}
			}

//printf("uc=%d  allblack=%d  blankcount=%d\n",uc,allblack,uni[uc]->blankcount);
			if (allblack==TRUE)
			{
				if (uni[uc]->blankcount>0)
					uni[uc]->blankcount--;				// count towards 0
				if (uni[uc]->blankcount==0)				// timer just expired ?
					uni[uc]->active=FALSE;				// then mark universe as inactive
			}
		}
	}
}



void init_universe()
{
	int i=0;

	for (i=0;i<MAX_UNIVERSE;i++)
		uni[i]=NULL;								// Make sure all our pointers are NULL pointers

}
