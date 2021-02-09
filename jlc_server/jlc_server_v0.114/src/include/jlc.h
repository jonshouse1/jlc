/*
	jlc.h
	Jons lighting control, name of both the server and the protocol.

	Version 0.14
	6 Dec 2020
*/

// jlc_effects.h needs to be included before jlc.h
#include <stdint.h>

#include "jlc_audioplus.h"
#include "jlc_effects.h"


#ifndef ESP8266
	#define TRUE				1
	#define FALSE				0
#endif

#define MAX_TCPCONN				64
#define MAX_CMDLEN				2048
#define CONN_PROMPT				"> "				// default prompt for tcp sessions

// devices
#define MAX_DEVF				128				// Max number of lighting devices
#define MAX_DEVN				1024				// Max number of sensors
#define MAX_SUBS				128				// Maximum number of subscriptions

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
#define MSK_SCR		1		// Running scripts
#define MSK_REG		(1 << 1)	// Registration requests/responses
#define MSK_DEV		(1 << 3)	// Device state
#define MSK_NDE		(1 << 4)	// Device name
#define MSK_ERR		(1 << 5)	// Errors
#define MSK_WWW		(1 << 6)	// messages from http server
#define MSK_JCP		(1 << 7)	// Control protocol events
#define MSK_GRP		(1 << 8)	// Monitor groups changing value
#define MSK_INF		(1 << 10)	// Information - Warnings and general messages
#define MSK_TCP		(1 << 11)	// TCP/IP connections to server
#define MSK_SND		(1 << 12)	// Global sounds sample player, also exists as a TOPIC for UDP clients


// Not sure yet ...... 
// "ValueChanged" identify where it has changed.
//#define VC_GROUP	10
//#define VC_DEVN		20
//#define VC_DEVF		30



// Subsctiption change event
#define SE_NEW_SECOND			1
#define SE_NEW_MIN			2
#define SE_STATUSLINE			10
#define SE_DEV_REGISTER			20
#define SE_DEV_TIMEOUT			30
#define SE_DEV_STATE			50
#define SE_PLAYSOUND			60


// Audio
#define NUM_SAMPELS			512					// 512 samples, stereo interleaved, 256 per channel 
#define PEAKHOLDTIME			36					// default time for peak and hold on visuals



// One of these is created for every channel within a universe, each channel is a single 8 bit value only
//struct  __attribute__ ((__packed__))  channel
struct channel
{
        unsigned char   value;							// 8 bit unsigned brigtness (DMX style)
	unsigned char	mask;							// R G B etc from MASK_KEY
	//struct	efx  	fx;
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


// Binary protocol for front end and peers, for TCP transport
#define BIN_MSG_GROUP_CFG	0						// must be 0, sending a struct jlc_group_cfg
#define BIN_MSG_GROUP_VAL	2
#define BIN_MSG_GROUP_CLEAR	4						// Dont send blank group structures, just send this
//#define BIN_MSG_CLRP_CLEAR	5						// colour presets clear, unused
#define BIN_MSG_DEVF		10						// struct jlc_devf
#define BIN_MSG_DEVF_CLEAR	11						// struct jlc_devf
#define BIN_MSG_DEVN		20						// struct jlc_devn
#define BIN_MSG_DEVN_CLEAR	21						// struct jlc_devn
#define BIN_MSG_CLRP		30						// struct colourpresets
#define BIN_MSG_DATETIME	80
#define BIN_MSG_PLAYSOUND	90
#define BIN_MAX_MSGLEN		8192						// maximum message length after header

struct __attribute__ ((__packed__)) bin_header
{
	char		binmagic[4];						// "JLCB"
	uint16_t	msg_type;						// Indicate what message contains ..  BIN_MSG_
	char		spare[6];						// future proof a little
	uint16_t	idx;							// When transferring a partial array this is the index
	uint16_t	msglen;							// Num bytes to follow header
};



#define CONN_MODE_LINEBYLINE		0					// default
#define CONN_MODE_CHARBYCHAR		1					// not working yet, see telnet specification
#define CONN_MODE_BIN			32					// binary interface for front ends
#define CONN_MODE_BIN_INITIAL		34					// just toggled into mode, implies client needs state
struct tcp_connection
{
	int		mode;							// One of TCP_CONN_MODE
	int		fd;
	uint32_t	m;							// One bit per type of event we want to be told about
	char		cmd[MAX_CMDLEN];					// Command line being entered
	char		prompt[MAX_PROMPTLEN];
	int		colour;							// TRUE for ANSI color sequence, default FALSE=plain text
	char		ipaddr[32];

	// These are for binary mode only
	int		t;							// table (one of BIN_MSG_)
	int		idx;							// index (the next entry to send)

	int		gotheader;						// True if the have read the header, but not the payload
	struct bin_header	hdr;
	//char		payload[65537];						// unsigned 16 bits worth
};


#define MAX_TIMED_EVENTS	32
struct timed_events
{
	int		active;
	unsigned int	intervalms;						// run every N ms
	int		rerun;							// TRUE re-run over and over
	unsigned int	lastranms;						// event is next due to run (ms)
	char		cmd[4096];						// pass to interpreter
};



#define MAX_SEQ_SCRIPTS				32				// Max number of scripts we load
#define MAX_TIMEMARKS				86 * 60 * 10			// Hz * 1MIN * MINS
#define MAX_SEQ_LINES				MAX_TIMEMARKS * 20
// Basic information for each lighting sequence file (.lsq)
struct sequencescript
{
	char	filename[MAX_FILENAMELEN];
	char	md5hash[34];

	char*	script;
	int	scriptsize;							// Size of script buffer in bytes
	int	line_offsets[MAX_SEQ_LINES];					// offsets into script[] for the start of each line
	int	line_length[MAX_SEQ_LINES];					// Length of line of text
	int	timemark_line[MAX_TIMEMARKS];					// line number of lines  starting with an audio frame number [N]
	int	timemark_lastline[MAX_TIMEMARKS];				// the last line of the script before the next time mark
	int	num_lines;
	int	num_timemarks;
	int	end_timemark;							// time mark number for last time in script
	int	debug;								// TRUE to show code running, false for normal
	int	framenumber;
};



// audioplus definitions
#define FBSMAX                          86


/*
// Colour presets
#define MAX_COLOUR_PRESETS		32
struct __attribute__ ((__packed__)) colourpresets
{
	uint16_t	active;
	uint16_t	R;
	uint16_t	G;
	uint16_t	B;
	uint16_t	W;
};
*/


