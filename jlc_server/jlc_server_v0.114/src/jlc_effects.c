/*
	jlc_effects.c
	Lighting effects, fades, visuals etc

	Effects for groups are applied to a group effects array rather than to the individual universe/channels.
	This allows group value changes to be synced with front ends without any extra code complexity.
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


// Groups
extern struct jlc_group_cfg     grpc[MAX_GROUPS+1];
extern struct jlc_group_val     grpv[MAX_GROUPS+1];
extern struct efx               grpefx[MAX_GROUPS+1];							// Group based effects


// Visuals
extern int     vis_vu_max;
extern int     vis_vu_left;
extern int     vis_vu_right;
extern int     vis_vu_leftpeak;
extern int     vis_vu_rightpeak;
extern int     vis_flag_agc;


/*
        uint16_t        fx;
        uint64_t        timems_created;                                         // ms since epoc when effect was started
        uint64_t        timems_run_next;                                        // time effect will (first or next) start running
        uint64_t        timems_run_every;                                       // frequency the effect runs
        uint64_t        timems_duration;                                        // how long to run for, for non self cancelling effects

        int             vals[FX_VALMAX];                                        // values passed into effect when created
        int             state[FX_VALMAX];                                       // state, variables for the effect
*/


// Group based effects
void grpefx_clear(int g)
{
	int x=0;

	grpefx[g].fx	           = FX_NONE;
        grpefx[g].timems_created   = 0;
        grpefx[g].timems_run_next  = 0;
        grpefx[g].ms_run_every     = 0;
        grpefx[g].ms_duration      = 0;
	for (x=0;x<FX_VALMAX;x++)
	{
		grpefx[g].vals[x]=0;
		grpefx[g].state[x]=0;
	}
}



// Clear all effects state
void init_effects()
{
	int g=0;

	for (g=1;g<MAX_GROUPS+1;g++)
		grpefx_clear(g);
}




// Given an effect name (in caps), return its number, returns -1 on error
int fx_lookup (char *fxname)
{
	if (strcmp(fxname,"NONE")==0)
		return(FX_NONE);
	if (strcmp(fxname,"CLEAR")==0)
		return(FX_NONE);
	if (strcmp(fxname,"PFADEUP")==0)
		return(FX_PFADEUP);
	if (strcmp(fxname,"PFADEDOWN")==0)
		return(FX_PFADEDOWN);
	if (strcmp(fxname,"STROBE")==0)
		return(FX_STROBE);
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





/*
     g       w1        w2         w3
 GFX <group> NONE                                    - Cancel any effects for group
 GFX <group> PFADEUP   <delay>    [fade rate Hz]     - Wait duration, then fade up
 GFX <group> PFADEDOWN <delay>    [fade rate Hz]     - Wait duration, then fade down 
 GFX <group> STROBE    <duration>                    - Strobe group 

struct efx
{
        uint16_t        fx;
        uint64_t        timems_created;                                         // ms since epoc when effect was started
        uint64_t        timems_run_next;                                        // time effect will (first or next) start running
        int             ms_run_every;                                           // frequency the effect runs
        int             ms_duration;                                            // how long effect lasts, for non self cancelling effects

        int             vals[FX_VALMAX];                                        // values passed into effect when created
        int             state[FX_VALMAX];                                       // state, variables for the effect
};
*/



void group_apply_fx(int textout, int g, char* w1, char* w2, char* w3, char *w4)
{
	int fx=0;
	int x=0;

	if (group_valid_range(textout, g, TRUE)!=TRUE)                                          // group not valid 
		return;
	if (strlen(w1)<=0)
		return;										// No arguments passed ?

	fx=fx_lookup (w1);
	if (fx<0)
	{
		xprintf(textout,"Err, FX name %s unknown\n",w1);
		return;
	}

	// Defaults for all effects
	grpefx_clear(g);									// Start building values from scratch
	grpefx[g].timems_created   = current_timems();
	grpefx[g].fx	           = fx;							// Make effect active
	grpefx[g].ms_run_every     = (1000/60);							// One second / 60Hz = N ms

	// Per effect settings
	switch (fx)
	{
		case FX_NONE:
			grpefx_clear(g);
			return;
		break;

		case FX_PFADEUP:
			grpefx[g].timems_run_next = current_timems() + timetext_to_ms (w2);	// Start running in Now in ms + future time in ms
			x=atoi(w3);								// Optional fade rate in Hz
			if (x>0)
				grpefx[g].ms_run_every = (1000/x);				// value in milliseconds
		break;

		case FX_PFADEDOWN:
			grpefx[g].timems_run_next = current_timems() + timetext_to_ms (w2);
			x=atoi(w3);
			if (x>0)
				grpefx[g].ms_run_every = (1000/x);
		break;

		case FX_STROBE:
		break;
	}
}




void group_effects_tick(int g)
{
	int i=0;												// Intensity
	if (grpefx[g].fx == FX_NONE)										// No group effect active ?
		return;

	if (current_timems() >= grpefx[g].timems_run_next)							// time now or past time to run effect again?
	{
		//printf("Running FX %d for group %d ..\n",grpefx[g].fx,g); fflush(stdout);
		switch (grpefx[g].fx)
		{
			case FX_PFADEUP:
				grpefx[g].timems_run_next = current_timems() + grpefx[g].ms_run_every;		// schedule next time to run effect
				i = grpv[g].I++;
				if (i>=255)									// Hit max brightness
				{
					i=255;
					grpefx_clear(g);							// Cancel myself
				}
				group_set_value(-1, g, -1, -1, -1, -1, i, 1);					// fade up, onoff=on and is left on
			break;

			case FX_PFADEDOWN:
				grpefx[g].timems_run_next = current_timems() + grpefx[g].ms_run_every;		// schedule next time to run effect
				i = grpv[g].I;
				i--;
				group_set_value(-1, g, -1, -1, -1, -1, i, 1);					// fade intensity down, group is switched on
				if (i==0)									// faded to black now
				{
					grpefx_clear(g);							// Cancel myself
					group_set_value(-1, g, -1, -1, -1, -1, i, 0);				// group is switched off now (onoff=0)
				}
			break;

			case FX_STROBE:
			break;
		}
	}
}




// Timer tick, around 1Khz
void effects_tick()
{
	int i=0;

	for (i=0;i<MAX_GROUPS;i++)
	{
		group_effects_tick(i);
	}
}


