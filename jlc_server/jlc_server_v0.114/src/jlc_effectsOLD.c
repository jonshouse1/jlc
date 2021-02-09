/*
	jlc_effects.c
	Lighting effects, fades, visuals etc
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
#include "jlc_group.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"

extern struct    universe* uni[MAX_UNIVERSE+1];
extern uint16_t  session_id;

extern struct jlc_devf		devf[MAX_DEVF];								// Table of lighting devices
extern struct jlc_devn		devn[MAX_DEVN];              


// Visuals
extern int     vis_vu_max;
extern int     vis_vu_left;
extern int     vis_vu_right;
extern int     vis_vu_leftpeak;
extern int     vis_vu_rightpeak;
extern int     vis_flag_agc;




void fx_cancel(int u, int c)
{
	int i=0;

	//printf("fx cancelled u=%d  c=%d\n",u,c); fflush(stdout);
	uni[u]->ch[c].fx.fx = FX_NONE;
	for (i=0;i<FX_VALMAX;i++)
	{
		uni[u]->ch[c].fx.vals[i] = 0;
		uni[u]->ch[c].fx.s[i] = 0;
	}
	uni[u]->ch[c].fx.creation_timems = 0;
	uni[u]->ch[c].fx.start_timems = 0;
	uni[u]->ch[c].fx.run_everyms  = 0;
	uni[u]->ch[c].fx.fx_duration_ms = 0;
}




// Called for each channel with an active effect whose timer indicates it us ready to be updated
void running_fx(int fx, int u, int c)
{
	unsigned int ed=0;
	//printf("running fx %d\n",fx); fflush(stdout);

	uni[u]->active		= TRUE;									// FX running on a u/c makes univ active
	uni[u]->last_changed 	= current_timems();
	switch (fx)
	{
		case FX_PFADEUP:
			if (uni[u]->ch[c].value<255)
				uni[u]->ch[c].value++;
			else	fx_cancel(u, c);
			if (uni[u]->ch[c].fx.vals[3] >0)						// group also tracks value
			{
				//int group_set_value(int textout, int g, int R, int G, int B, int W, int onoff);
				//group_set_value(-1, uni[u]->ch[c].fx.vals[3], uni[u]->ch[c].value);	// tell devices mapped to group
			}
		break;

                case FX_PFADEDOWN:
			if (uni[u]->ch[c].value>0)
				uni[u]->ch[c].value--;
			else	fx_cancel(u, c);
			if (uni[u]->ch[c].fx.vals[3] >0)
			{
				//group_set_value(-1, uni[u]->ch[c].fx.vals[3], uni[u]->ch[c].value);
			}
		break;

                case FX_STROBE:
			ed = (unsigned int)current_timems() - uni[u]->ch[c].fx.creation_timems;		// effect duration so far
			if (ed > uni[u]->ch[c].fx.vals[0])						// time to finish effect ?
			{
				uni[u]->ch[c].value = uni[u]->ch[c].fx.vals[4];				// Restore initial channel value
				fx_cancel(u, c);
			}
			else	uni[u]->ch[c].value =  ~uni[u]->ch[c].value;				// Invert bits
			// set value call maybe ?
		break;
	}
}



// For a given universe run the effects
void universe_effects(int u)
{
	int c=0;

	if (uni[u]==NULL)
		return;

	for (c=1;c<=uni[u]->noc;c++)							// for every channel is this universe
	{
		if (uni[u]->ch[c].fx.fx != FX_NONE)					// got an active FX?
		{
			if ( (unsigned int)current_timems() >= uni[u]->ch[c].fx.start_timems )	// time to start it running ?
			{
				// schedule next run
				uni[u]->ch[c].fx.start_timems = (unsigned int)current_timems() + uni[u]->ch[c].fx.run_everyms;
				running_fx(uni[u]->ch[c].fx.fx, u, c);
			}
		}
	}
}




// Given an effect name (in caps), return its number, returns -1 on error
int fx_lookup (char *fxname)
{
	if (strcmp(fxname,"NONE")==0)
		return(FX_NONE);
	if (strcmp(fxname,"PFADEUP")==0)
		return(FX_PFADEUP);
	if (strcmp(fxname,"PFADEDOWN")==0)
		return(FX_PFADEDOWN);
	if (strcmp(fxname,"STROBE")==0)
		return(FX_STROBE);
	return(-1);
}


/*
struct cfx
{
        uint16_t        fx;
        unsigned int    creation_timems;
        int             vals[FX_VALMAX];                                        // values passed into effect when created
        int             s[FX_VALMAX];                                           // state, temporary working values
};
struct channel
{
        unsigned char   value;                                                  // 8 bit unsigned brightness (DMX style)
        unsigned char   mask;                                                   // R G B etc from MASK_KEY
        struct  cfx     fx;
};
struct universe
{
        int             active;
        int             noc;                                                    // Number of channels for this universe
        int             bytes;
        char            univ_name[UNIV_NAME_MAX];
        int             blankcount;
        uint32_t        last_changed;
        struct channel  ch[1];                                                  // always last, 1 defined but memory allocated for more
};
*/


/*
	Fade frequency is how many times a second an effect is run, 10Hz will change the value 10 times a second

	Effect			vals[0]			vals[1]			vals[2]			vals[3]
	FX_PFADEUP		fade frequencyHz							Optional group
	FX_PFADEDOWN		fade frequencyHz							Optional group
	FX_STROBE		duration_ms		finishing value					Optional group
	FX_CYCLE		duration_ms 0=forever	fade frequencyHz	
*/
// Apply and effect 'fx' to universe 'u' channel 'c',  using values supplied. Returns 0 if ok, negative if fail
int fx_apply(int u, int c, int fx, int vals[FX_VALMAX])
{
	int i=0;

	if (universe_valid_range(-1, u, FALSE)!=TRUE)
		return(-1);
	if (channel_valid_range(-1, c, FALSE)!=TRUE)
		return(-2);
	if (uni[u]==NULL)
		return(-3);

//int timetext_to_ms(char *text)

	uni[u]->ch[c].fx.fx = fx; 						// effect now active
	for (i=0;i<FX_VALMAX;i++)
	{
		uni[u]->ch[c].fx.vals[i] = vals[i];				// save values passed in for effect
		uni[u]->ch[c].fx.s[i] = vals[i];				// save same values into 'state'
	}
	uni[u]->ch[c].fx.creation_timems = current_timems();			// note when effect was created
	uni[u]->ch[c].fx.start_timems = 0;					// not all effects have one
	uni[u]->ch[c].fx.run_everyms  = (1000/60);				// 1 second / Freq Hz
	uni[u]->ch[c].fx.fx_duration_ms = 0;					// 0 for self cancelling effects


	// Set initial state for effect, the meaning of vals[] and s[] depend on the effect
	if (fx==FX_PFADEUP)
	{
		uni[u]->ch[c].fx.start_timems = (unsigned int)current_timems() + vals[0];	// when effect starts running
		return(0);
	}

	if (fx==FX_PFADEDOWN)
	{
		uni[u]->ch[c].fx.start_timems = (unsigned int)current_timems() + vals[0];
		return(0);
	}

	if (fx==FX_STROBE)
	{
		uni[u]->ch[c].fx.start_timems = current_timems();		// starting now
		uni[u]->ch[c].fx.run_everyms  = (1000/10);			// Slow effect right down so we can see it
		uni[u]->ch[c].fx.fx_duration_ms = vals[0];			// how many ms to run effect for
		uni[u]->ch[c].fx.vals[4]      = uni[u]->ch[c].value;		// Save value before FX started
		return(0);
	}
	return(-1);
}



// List the names of all effects
void list_fx(int textout)
{
	xprintf(textout,"NONE\t\t%d\tNo effect\n",FX_NONE);
	xprintf(textout,"PFADEUP\t\t%d\tPause, then fade up\n",FX_PFADEUP);
	xprintf(textout,"PFADEDOWN\t%d\tPause, then fade down\n",FX_PFADEDOWN);
	xprintf(textout,"STROBE\t\t%d\tStrobe channels for N seconds\n",FX_STROBE);
}




// Timer tick, around 1Khz
void effects()
{
	int i=0;

	//for (i=1;i<=MAX_UNIVERSE;i++)							// for every universe
	//{
		//if (uni[i]!=NULL)
			//universe_effects(i);
	//}


	for (i=0;i<MAX_GROUPS;i++)
	{
	}
}

