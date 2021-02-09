/*
	jcp_group.h
	Version 0.5
	Last changed 8 Dec 2020
*/

#include <stdint.h>

#define MAX_GROUPS			64
#define MAX_GROUPNAME_LEN		64
#define MAX_GROUPEL			1024



#define GROUP_LIGHT_TYPE_MONO		0				// 1 byte (least sig byte)	000000XX
#define GROUP_LIGHT_TYPE_BIT		1				// just the "onoff" flag is unsed
#define GROUP_LIGHT_TYPE_RGB		3				// 3 bytes			XXXXXX00
#define GROUP_LIGHT_TYPE_RGBW		4				// 4 bytes			XXXXXXXX


// Configuration, rarely changes
struct __attribute__ ((__packed__)) jlc_group_cfg			// Group configuration
{
	int		gltype;						// GROUP_LIGHT_TYPE_
	int		noc;						// number of channels in this group, 0=inactive
	char		name[MAX_GROUPNAME_LEN];
	int		univ[MAX_GROUPEL];
	int		chan[MAX_GROUPEL];
	int		save_pending;					// True if system needs to write group to disk sometime
};


// Values, frequently changes
// Intensity is an 8 bit value for mono group type, in RGB or RGBW types intensity scales the RGB value for fades etc
struct __attribute__ ((__packed__)) jlc_group_val			// Group value
{
	int16_t		R;						// positive 8 bit value, -1 indicates unused
	int16_t		G;
	int16_t		B;
	int16_t		W;
	int16_t		I;						// Intensity, positive 8 bit value, <=0 = unused
	int		onoff;						// 1=On, 0=off. Must only be 0 or 1 as this used to drive relays/switches
	uint64_t	last_change_time_ms;
};





