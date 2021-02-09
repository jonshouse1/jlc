/*
	jlc.h
	Jons lighting control, name of both the server and the protocol.

	Version 0.1
	1 Jul 2019
*/

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
#define UNIVERSE_NONE				0				// We don't use universe 0, start from 1
#define UNIVERSE_CREATED			1
#define UNIVERSE_ACTIVE				2


// One of these is created for every channel with a universe
//struct  __attribute__ ((__packed__))  channel
struct channel
{
        unsigned char   value;							// 8 bit unsigned brightness (DMX style)
	unsigned char	mask;							// R G B etc from MASK_KEY
	// per channel effect details would go here
};


struct universe
{
        int             active;
        int             noc;							// Number of channels for this universe
        int             bytes;
        char            univ_name[UNIV_NAME_MAX];
        struct		channel ch[1];						// always last, 1 defined but memory allocated for more
};



// Used by jlc.c and jlc_tcpconn
struct tcp_connection
{
	int     mode;
	int     fd;
	int     m;								// monitor (8 bit unsigned)
	char    cmd   [MAX_CMDLEN];						// Command line being entered
	char    prompt[MAX_PROMPTLEN];
	int	colour;								// TRUE for ANSI colour sequence, default FALSE=plain text
};


// Connection monitoring (see debug)
#define MSK_REG		(1 << 1)	// Registration requests/responses
#define MSK_DEVN	(1 << 2)	// Input device related (sensors)
#define MSK_DEVF	(1 << 3)	// Output device related (fixtures)
#define MSK_ERR		(1 << 4)	// Errors
#define MSK_VER		(1 << 7)	// Misc verbose messages
#define MSK_ALL		0xFF

