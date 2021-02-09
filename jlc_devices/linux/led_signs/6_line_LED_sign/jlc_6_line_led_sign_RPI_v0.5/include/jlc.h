/*
	jlc.h
	Jons lighting control, name of both the server and the protocol.

	Version 0.1
	1 Jul 2019
*/

// jlc_effects.h needs to be included before jlc.h
#include <stdint.h>

#ifndef ESP8266
	#define TRUE					1
	#define FALSE					0
#endif

#define MAX_TCPCONN				64
#define MAX_CMDLEN				2048
#define CONN_MODE_LINEBYLINE			0
#define CONN_MODE_CHARBYCHAR			1
#define CONN_PROMPT				"> "				// default prompt for tcp sessions

// devices
#define MAX_DEVF				128				// Max number of lighting devices
#define MAX_DEVN				1024				// Max number of sensors

// interpreter
#define MAXWORDS				64
#define MAXWORDLEN				512

// universes
#define MAX_UNIVERSE				64				// This many universes
#define MAX_CHANNEL				65000				// Max universe size
#define UNIV_NAME_MAX				128				// Text name for universe
#define DEFAULT_CHANNELS_PER_UNIVERSE		512				// As default this many channels per universe
#define MAX_CHANNELS				16384				// Maximum universe size 
#define DEFAULT_UNIV_UPDATE_RATE_HZ		40

// files
//#define MAX_IPSTRING				32				// Max size of ip address string
#define MAX_FILENAMELEN				4096
#define MAX_PATHLEN				2048
#define MAX_PROMPTLEN				64

// Network
#define MAX_UDP_MESSAGESIZE			65520				// 65,507 bytes for IPv4 and 65,527 bytes IPV6 

// About universes
#define UNIVERSE_NONE				0				// We dont use universe 0, start from 1
#define UNIVERSE_CREATED			1
#define UNIVERSE_ACTIVE				2

// Connection monitoring (see debug)
#define MSK_REG		(1 << 1)	// Registration requests/responses
#define MSK_DEVN	(1 << 2)	// Input device related (sensors)
#define MSK_DEVF	(1 << 3)	// Output device related (fixtures)
#define MSK_ERR		(1 << 4)	// Errors
#define MSK_WWW		(1 << 5)	// messages from http server
#define MSK_VER		(1 << 7)	// Misc verbose messages
#define MSK_ALL		0xFF



// Effects
#define FX_VALMAX			8					// max number of variables for an effect
#define FX_NONE                         0
#define FX_PFADEUP			10
#define FX_PFADEDOWN 			11
#define FX_FADEDOWN_AFTER		20
#define FX_STROBE                       100


// This structure is added to each channel of the universe
struct cfx
{
	uint16_t	fx;
	unsigned int	creation_timems;
	unsigned int	start_timems;						// time effect will start running, 0=if always running
	unsigned int	run_everyms;						// frequency the effect runs
	unsigned int 	fx_duration_ms;						// how long to run for, for non self cancelling effects
	int		vals[FX_VALMAX];					// values passed into effect when created
	int		s[FX_VALMAX];						// state, tempory working values
};




// One of these is created for every channel within a universe
//struct  __attribute__ ((__packed__))  channel
struct channel
{
        unsigned char   value;							// 8 bit unsigned brigtness (DMX style)
	unsigned char	mask;							// R G B etc from MASK_KEY
	struct	cfx  	fx;
};


struct universe
{
	int             active;	
	int             noc;							// Number of channels for this universe
	int             bytes;
	char            univ_name[UNIV_NAME_MAX];
	int		blankcount;	
	uint32_t	last_changed;
	struct channel  ch[1];							// always last, 1 defined but memory allocated for more
};



// Used by jlc.c and jlc_tcpconn
struct tcp_connection
{
	int     mode;
	int     fd;
	int     m;								// monitor (8 bit unsigned)
	char    cmd   [MAX_CMDLEN];						// Command line being entered
	char    prompt[MAX_PROMPTLEN];
	int	colour;								// TRUE for ANSI color sequence, default FALSE=plain text
};




