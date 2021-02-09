/*
	jcp_protocol.h
	(j) (c)ontrol (p)rotocol

	This defines both the protocol and devices

	(c) Jonathan Andrews 2019  (jon@jonshouse.co.uk)
	Version 0.34
	4 Aug 2019

	This protocol is used for:
		Discovery of lighting controller (one per network only)
		Registering of lights	
		The state of "groups" of lights

	Maximum UDP payload size is 1472 bytes
*/


#include <stdint.h>

#define JCP_SERVER_PORT				1111				// If available default port for JCP protocol
#define JLP_SERVER_PORT				1181				// if available default port for JLP (light) protocol

#define UID_LEN					7
#define STLINE_LEN				1024				// Max length of status line
#define JCP_TIMER_FREQ_HZ			20
#define JCP_TIMEOUT_S				60 * 2				// 60 Seconds * N mins
#define JCP_ANNOUNCE_INTERVAL_S			60
#define MAX_IPSTR_LEN				32				// Max size of ip address string
#define MAX_DEVNAME_LEN				32
#define MAX_IPTABLE				1024
#define MAX_MSG_LEN				2048				// just big enough for largest single UDP message
#define MAX_VALUEBYTESLEN			512				// Upto N bytes in valuebytes of dev_state
#define MAX_TOPICS				16				// One flag per topic
#define MAX_TDATA_LEN				16384				// Upto N bytes of data per topic message
#define DEVICE_MODEL_LEN			32
#define DEVICE_FW_VER_LEN			32

#define JCP_MAGIC				"JCPM"
#define JCP_MSG_ANNOUNCE			1
#define JCP_MSG_PINGPONG			2				// Message out and one back
#define JCP_MSG_ACK				3				// Message ACK with msg_handle=0 used as "I am alive"
#define JCP_MSG_SERVERSESSION			4				// Server sends its session id
#define JCP_MSG_DISCOVER			10				// Client discovers a server
#define JCP_MSG_DISCOVER_REPLY			11
#define JCP_MSG_REGISTER_DEVS			20				// Client registers itself with the server
#define JCP_MSG_DEV_STATE			200				// header is followed by dev_state
#define JCP_MSG_DEV_CONFIG			202				// Pass config data to device
#define JCP_MSG_TOPIC				300			

// These values correspond to index of 'char topics[MAX_TOPICS]'
#define JCP_TOPIC_STATUSLINE			0				// status line, sent when it changes
#define JCP_TOPIC_DEVICEREG			1				// reg or de-reg of devices
#define JCP_TOPIC_DATETIME_B_S			12				// date/time in binary format, every second
#define JCP_TOPIC_DATETIME_B_M			13				// date/time in binary format, every min
#define JCP_TOPIC_DATETIME_S_S			14				// date/time as string, every second
#define JCP_TOPIC_DATETIME_S_M			15				// date/time as string, every min

// Connection state machine
#define JCP_CLIENT_STATE_INITIAL		0
#define JCP_CLIENT_STATE_SENDING_DISCOVER	1
#define JCP_CLIENT_STATE_SENDING_REGISTER	2
#define JCP_CLIENT_STATE_RUNNING		3
#define JCP_CLIENT_STATE_SHUTTINGDOWN		4

// Used to describe table entries and similar, not real devices
#define DEV_NAME_LEN				32				// Friendly name for device
#define DEV_NONE				0				// useful to identify free slots in tables etc
#define DEV_ALL					1
#define DEV_F					2
#define DEV_N					3	

// Fixture (light) type devices, numbers <200
#define DEVF_DMX				10				// Describes a DMX bridge
#define DEVF_LIGHT				11				// Lighting device, one or more channels
#define DEVF_PIXELS				20				// Arrays of pixels, often >512 channels but <65,000 channels
#define DEVF_BITMAP				30				// Display boards, unlike LED_SIGN they are graphical not text


// NOTE:
//   RELAY was a DEV_F device, but it is a better fit as a DEV_N device,  control is via mapping device
//   to a group or by using "dev <UID> state" command

// Low data rate devices, switches sensors etc, numbers >=400
#define DEVN_RELAY				400				// Relay.  Relays are always registered individually
#define DEVN_SWITCH				500				// simple on/off switch, latching
#define DEVN_SWITCHPBT				501				// Push button toggle switch, push for on, again for off
#define DEVN_SWITCHPBM				502				// Push button momentary, device sends 1 when first pushed in, 0 after release
#define DEVN_DOORBELL				510				// momentary action push button
#define DEVN_MOVEMENTSENSOR			520
#define DEVN_POT				540
#define DEVN_TEMPSENS				560				// temp sensor, requires an 8 bytes UID, DS18B20 has own ROM
#define DEVN_MCURRENTSENS			600				// Mains current sensor
#define DEVN_DCURRENTSENS			601				// DC Current sensor
#define DEVN_PLAYSOUNDM				700				// Play a sound, mechanical, makes only one noise
#define DEVN_PLAYSOUNDS				701				// Plays a sound sample, valuecbytes is filename
#define DEVN_CLOCKCAL				800				// Clock (and or) calendar
#define DEVN_LEDSIGN				900				// LED (scrolling) sign display text information
#define DEVN_DUMMY				999				// Does nothing, but can be registered, use if IP just wants to subscribe

#define SCRIPT_STOP				-1000				// Stop running the script now

// Colour KEY related
#define MASK_KEY "R=Red  G=Green  B=Blue  Y=Yellow  W=White  O=Orange  P=Purple  I=pInk  U=UV  X=Unused"
#define VALID_MASK				"RGBYWOPIUX"
#define MASK_R					'R'
#define MASK_G					'G'
#define MASK_B					'B'
#define MASK_Y					'Y'
#define MASK_W					'W'
#define MASK_O					'O'
#define MASK_P					'P'
#define MASK_I					'I'
#define MASK_U					'U'
#define MASK_X					'U'


// Basic fields common to all messages, discover, pingpong, and deregister use only the header
struct __attribute__ ((__packed__)) jcp_header
{
	char			magic[5];
	char			send_ack;					// TRUE if sender is asking for an ACK packet back
	uint16_t		msg_type;
	uint16_t		session_id;					// unique number for session, used by client and server
	uint16_t		message_id;					// the same message may repeat, message_id identifies a unique msg
	uint16_t		reply_port;					// client tells us what UDP port to reply to messages on
};


// Sent to client after receipt of a header with message_type=JCP_MSG_DISCOVER
struct __attribute__ ((__packed__)) jcp_msg_discover_reply
{
	struct jcp_header	hdr;
	uint16_t		interpreter_tcp_port;
};


// Describes one device for registration, not all fields are used by all devices
struct __attribute__ ((__packed__)) jcp_dev
{
	uint16_t		dev_type;
	unsigned char		uid[UID_LEN];					// each device has an 7 byte uid, 6 bytes are often the MAC
	uint16_t		noc;						// only valid for lights
	uint16_t		jcp_udp_port;					// device listens for control data on port
	uint16_t		jlp_udp_port;					// device listens for lighting data on port, 0=unused
	char			device_model[DEVICE_MODEL_LEN];			// Describes device type, ESP8266 or ARMLinux for example
	char			device_fw_ver[DEVICE_FW_VER_LEN];		// Describes firmware version number and date
	// The following field is not used for DEV_F devices
	char			topics[MAX_TOPICS];				// Array of TRUE or FALSE values for notification by topic
};



// Client registers all its devices at once via this message, simplifies sender state machine
struct __attribute__ ((__packed__)) jcp_msg_register_devs
{
	char			magic[4];					// 'REGD'
	uint16_t		pver;						// protocol version
	unsigned char		num_of_devs;					// how many struct jcp_dev follow ...
	struct jcp_dev		dev[1];						// Describe one here, but multiple may be here
};



// All devices share this struct, some fields are blank on some devices, sent in either direction
struct __attribute__ ((__packed__)) dev_state
{
	uint16_t		dev_type;					// DEVF_DMX etc or DEVN_SWITCH etc
	unsigned char		uid[UID_LEN];
	uint16_t		value1;
	uint16_t		value2;
	// Variable length packet, these fields must be last, the meaning depends on the dev_type
	char			asciiorbinary;					// 1=ASCII string, 0=binary data
	uint16_t		valuebyteslen;					// number of bytes in valuebytes[]
	char			valuebytes[MAX_VALUEBYTESLEN];			// structure defined at its maximum size
};



// Sent from the server to the client, normally containing details of some sort of event
struct __attribute__ ((__packed__)) jcp_msg_topic
{
	unsigned char		uid[UID_LEN];					// the following is destined for this device
	uint16_t		topic;						// what this message is about
	uint16_t		tlen;						// length of data to follow
	unsigned char		tdata[MAX_TDATA_LEN];				// shorten packet to clip tdata to length
};



// TODO: Maybe move these to another file later
// ************ Server tables, these are not network messages **********
struct dev_dmx
{
	int			noc;						// number of channels
	int			univ;						// universe this is mapped into
	int			fchan;						// first channel in universe this maps into
};


struct dev_pixels
{
	int			nop;						// number of pixels
	int			pixeltype;
};


struct dev_bitmap
{
	int			nop;						// number of pixels
	int			pixeltype;
	int			w;						// width
	int			h;						// height
};


// For ESP8266 based devices, see espflaher.c
#define FLASHER_CMD_REQ_FWVER		8
#define FLASHER_CMD_PREPARE		10
#define FLASHER_CMD_FLASHCHUNK		11
#define FLASHER_CMD_RESTART		100
struct __attribute__ ((__packed__)) flasher_esp
{
	char			magic[8];					// FLASH\0\0
	uint16_t		cmd;
	uint16_t		count;
	uint16_t		destport;					// reply to flash util on this port
	uint32_t		addr;
	uint16_t		chunk;						// 0,1,2 and 3
	char			flashchunk[1024];				// 4k per flash sector, so 4 of these for each flash write
};



// System holds this table for registered devices, 
struct jlc_devf
{
	uint16_t		dev_type;
	unsigned char		uid[UID_LEN];
	char			ipaddr[MAX_IPSTR_LEN];
	char			dev_name[DEV_NAME_LEN];
	int			jcp_udp_port;
	int			jlp_udp_port;					// UDP ports device is listening on
	int			noc;						// number of channels this device supports
	int			univ;						// universe it is mapped to
	int			fchan;						// first channel from that universe device syncs with
	uint16_t		update_rate_hz;					// How frequently we try and refresh this
	unsigned int		last_send_time_ms;				// last time we sent this device data
	char			device_model[DEVICE_MODEL_LEN];			// H801 or ESP_RELAY_TH10 for example
	char			device_fw_ver[DEVICE_FW_VER_LEN];		// Describes firmware version number and date
};


// System holds a table of these
struct jlc_devn
{
	uint16_t		dev_type;					// The device type when dev registers
	int			hide;						// Hide this device entry
	unsigned char		uid[UID_LEN];
	char			ipaddr[MAX_IPSTR_LEN];
	char			dev_name[DEV_NAME_LEN];
	int			jcp_udp_port;
	int			jlp_udp_port;					// UDP ports device is listening on
	int			group;						// This device maps to this group
	int			subscribed;					// True if any of topics[] is set
	char			topics[MAX_TOPICS];				// which topic a device subscribed to
	char			device_model[DEVICE_MODEL_LEN];			// Describes device type, ESP8266 or ARMLinux for example
	char			device_fw_ver[DEVICE_FW_VER_LEN];		// Describes firmware version number and date

	unsigned int		last_devstate_update_timems;			// last time device sent us dev_state
	struct dev_state	ds;						// last state device sent us
	struct dev_state	pds;						// the previous values
};





