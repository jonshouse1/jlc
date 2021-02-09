/*
	jlc_effects.h

	Version 0.1
	4 Dec 2020
*/

#include <stdint.h>


#define FX_VALMAX			8					// max number of variables for an effect

// Effects 
#define FX_NONE                         0
#define FX_PFADEUP			10
#define FX_PFADEDOWN 			11
#define FX_FADEDOWN_AFTER		20
#define FX_STROBE                       100



struct efx
{
	uint16_t	fx;
	uint64_t	timems_created;						// ms since epoc when effect was started
	uint64_t	timems_run_next;					// time effect will (first or next) start running
	int		ms_run_every;						// frequency the effect runs
	int		ms_duration;						// how long effect lasts, for non self cancelling effects

	int		vals[FX_VALMAX];					// values passed into effect when created
	int		state[FX_VALMAX];					// state, varaibles for the effect
};



