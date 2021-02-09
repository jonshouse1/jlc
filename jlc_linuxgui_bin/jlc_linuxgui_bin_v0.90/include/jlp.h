/*
	jlp.h

	Jons lighting packet protocol
	Simple message format for packets to lighting devices

	Version 0.3
	9 May 2020


	NOTES:
	include jcp_protocol_devs.h before this header
	The server sends only the number of channels the device needs. For example if the device has 5 channels of output
	the the server will send 5 brightness values only.
	The map_universe, map_fchan is for information only, text displays on devices maybe, other than that it is not used
	
	Unlike the server, values[] starts at index 0, so our example 5 channel device would get values[0] to values[4].

	JCP protocol (Control protocol) is used to register the lighting devices. For the client jcp_register_dev() should be
	used to register the lighting device, uid are shared with sensors, so all UIDs on one IP must be unique
*/

#include <stdint.h>

// https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers
#define JLP_UDP_PORT				6262
#define JLP_DEFAULT_UPDATERATE_HZ		40
#define JLP_MAX_VALUES				1024			// Limit to sending 1k of light data at a time

#define JLP_MAGIC		"JLP"
struct __attribute__ ((__packed__)) jlp_pkt
{
	char            magic[4];					// J L P (0) 
	uint16_t	msg_type;					// always 2 at the moment
	unsigned char	uid[UID_LEN];					// One IP can have multiple JLP devices registered
	uint16_t	update_rate_hz;					// How frequently the server tries to send to us
	uint16_t        host_time_ms;					// wraps over often, so be careful using it
	uint16_t        map_universe;
	uint16_t	map_fchan;					// just for info displays, server has this mapped here

	// Call them 'channels' on the server and 'values' on the device, otherwise it all gets a bit confused
	uint16_t	nov;						// number of values in this message
	uint32_t	voffset;					// values starting at this position
	unsigned char   values[1];					// lighting data
};

// changed noc to nov..
// added voffset
// will need to do multiple sends if device has large number of channels as we cant send more than about 1400 at a go




// Statistics for JLP lighting data
struct statss
{
	// Lighting protocol stats
        int pps_jlp;							// packets per second
        int ppps_jlp;
	int bps_jlp;							// bytes per second
	int pbps_jlp;

	// Control protocol (devices) stats
        int pps_jcp;							// packets per second
        int ppps_jcp;
	int bps_jcp;							// bytes per second
	int pbps_jcp;

	// Audioplus stats
	int pps_audioplus;						// packets per second
	int ppps_audioplus;
};

