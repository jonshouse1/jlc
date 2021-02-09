/*
	jcp_client.c

	Ver 0.35
	8 Aug 2019
	Library for talking (j) (c)ontrol (p)rotocol to JLC server

	Code suitable for ESP32 RTOS,  ESP8266 NON RTOS SDK and Linux
	remember to set a define via "gcc -D"

        If you run out of memory on the ESP8266 you probably forgot to add -DESP8266 to CFLAGS in Makefile
*/


#ifndef ESP8266         
	#define ICACHE_FLASH_ATTR						// Meaningful only on ESP8266 compiler
        #include <stdio.h>
#else                                                                           // 8266 specific includes
        #include <ets_sys.h>
        #include <osapi.h>
        #include <os_type.h>
        #include <mem.h>
        #include <ets_sys.h>
        #include <osapi.h>
        #include <user_interface.h>
        #include <espconn.h>
        #include "uart.h"
#endif

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef LINUX
#endif

#include "jlc.h"
#include "jcp_protocol_devs.h"
#define DEVT_MAX	8							// keep sensibly small, uses scarce RAM on ESP8266
#include "jcp_client.h"


// Prototypes
int ICACHE_FLASH_ATTR udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast);
char* printuid(char unsigned *uid);



// JCP replied
uint16_t 	session_id =0;
uint16_t	jcp_message_id=1;						// must not be 0 for first packet
int		jcp_connection_state=0;
char		jcp_server_ipaddr[32];
//int		jcp_timeoutms=0;						// MAX time waiting for an ACK
int		jcp_server_session_id = 0;
int		jcp_num_of_devs=0;
int 		jcp_attempts=0;
int		jcp_tick_interval_ms=0;						// how often our timer tick is called
struct jcp_dev	jb[DEVT_MAX];							// hold the details for devices we register


unsigned char   uidbase[7];							// 6 bytes MAC, one byte device index
char jcp_device_model[DEVICE_MODEL_LEN];
char jcp_device_fw_ver[DEVICE_FW_VER_LEN];


// Copy of a packet to send and its delivery details
static unsigned char jcp_msgbuf[MAX_MSG_LEN];
int jcp_msglen=0;
char jcp_ipaddr[MAX_IPSTR_LEN];
int jcp_bcast=0;
int jcp_waiting_ack=-1;								// -1 not waiting for any ack
int jcp_waiting_on=0;
int jcp_msg_type=0;								// type of last packet sent
int jcp_backoff_timer=0;							// as this gets bigger send rate get slower

int jcp_udp_port=0;								// Port this device listens on for jcp protocol
int jlp_udp_port=0;
int jcp_done_init=FALSE;


//int dosubscribe=FALSE;
//struct jcp_msg_subscribe	ms;						// hold onto a copy of the subscribe message


#ifndef ESP8266
#include <stdarg.h>
int os_printf(char *fmt, ...)
{
        va_list arg;
        char buf[8192];

        va_start(arg, fmt);
        buf[0]=0;
        vsprintf((char*)&buf, fmt, arg);
        if (strlen(buf)>0)
        {
                printf("%s",buf);
        }
        va_end(arg);
	fflush(stdout);
        return(0);
}

void system_restart()
{
	//exit(0);
	exithandler();
}
#endif


int ICACHE_FLASH_ATTR bt_set(int a)
{
        int t=a*50;
        if (t> 50 * 15)								// 15 seconds between packets is slowest
                t=50*15;
        return(t);
}


void ICACHE_FLASH_ATTR jcp_initial_state()
{
	jcp_num_of_devs=0;
	jcp_backoff_timer=0;
	jcp_attempts=0;
	jcp_message_id=rand();
	jcp_waiting_ack=-1;
	bzero(&jcp_server_ipaddr,sizeof(jcp_server_ipaddr));
	jcp_connection_state=0;							// do discover again
}


// Call this once to get things started, jcp_init() calls this
void ICACHE_FLASH_ATTR jcp_init_b(char *uidb, uint16_t sid, int jcp_udp_iport, int jlp_udp_iport, int tick_interval_ms, char* device_model, char* device_fw_ver)
{
	os_printf("jcp_init()  sid=%d  tick_interval_ms=%d\n",sid, tick_interval_ms);
	session_id=sid;								// Should be a non zero random number

	jcp_udp_port = jcp_udp_iport;						// save init values in global vars
	jlp_udp_port = jlp_udp_iport;

	bzero(&jb[0],sizeof(jb));
	memcpy(&uidbase, uidb, 6);						// copy in MAC part
	uidbase[6]=0;								// zero index
	strncpy((char*)&jcp_device_model, device_model, DEVICE_MODEL_LEN);	// copy padded with 0
	strncpy((char*)&jcp_device_fw_ver, device_fw_ver, DEVICE_FW_VER_LEN);
	jcp_initial_state();
	jcp_tick_interval_ms=tick_interval_ms;					// how often our timer tick is called
	jcp_done_init=TRUE;
}


// Now call this multiple times, one for each device to register
// Topic is a string of 1s and 0s (ASCII) it must be MAX_TOPICS long.
// The position of the 1/0 digit corresponds to JCP_TOPIC definitions, leftmost is JCP_TOPIC_STATUSLINE
void ICACHE_FLASH_ATTR jcp_register_dev(unsigned char idx, int dev_type, int noc, char* topics)
{
	int t=0;

	if (idx>=DEVT_MAX)
	{
		os_printf("Err: increase DEVT_MAX\n");
		return;
	}
	jb[idx].dev_type	= dev_type;					// now it is a non zero value
	jb[idx].jcp_udp_port	= jcp_udp_port;					// global saved from init_b()
	if (dev_type < 200)							// devf (fixture) device?
		jb[idx].jlp_udp_port	= jlp_udp_port;				// devf register JLP and JCP UDP ports
	else	jb[idx].jlp_udp_port	= 0;					// devn devices do not register a JLP port
	jb[idx].noc=noc;
	memcpy(jb[idx].uid,uidbase,6);						// Copy in the fixed part of the uid
	jb[idx].uid[6]=idx+1;							// uid is now 7 bytes unique to this device	

	strcpy(jb[idx].device_model, jcp_device_model);				// All devices report same model and frimware version
	strcpy(jb[idx].device_fw_ver, jcp_device_fw_ver);
	for (t=0;t<MAX_TOPICS;t++)						// parse topics string
		jb[idx].topics[t]=topics[t];					// Copy in the ASCII
	os_printf("jcp_register_dev  IDX:%02d\tUID:%s JCPPORT:%d\n",idx,printuid(jb[idx].uid), jb[idx].jcp_udp_port);
	jcp_num_of_devs++;
}



// Normally the UID is the base UID (mac) with the last digit the idx, allow user to override that with a arbitrary UID
void ICACHE_FLASH_ATTR jcp_set_dev_uid(unsigned char idx, unsigned char *newuid)
{
	if (jb[idx].dev_type!=0)						// basic check, is it a registered device ?
	{
		memcpy(jb[idx].uid, newuid, UID_LEN);				// copy in new UID
	}
	else	os_printf("jcp_set_dev_uid() Error,  idx %d needs to be registered first\n\n");
}



// Take a pre-built message payload (message excluding header) and send it. Every outbound message should be via this.
// send_ack tells the server to send a reply to the message, requires_ack asks jcp_client to possibly re-try
// sending and wait for positive ack to confirm the server received the message
void ICACHE_FLASH_ATTR jcp_send(char *ipaddr, int bcast, int msg_type, char* payload, int payload_len, int send_ack, int requires_ack)
{
	struct jcp_header *jcph = (struct jcp_header*)&jcp_msgbuf;		// sit the header on our buffer

	jcp_attempts=0;
	memcpy(jcph->magic,JCP_MAGIC,sizeof(JCP_MAGIC)-1);			// build header
	jcph->session_id	= session_id;
	jcph->send_ack		= send_ack; 
	jcph->msg_type		= msg_type;
	jcph->reply_port	= jcp_udp_port;					// JCP UDP port we are listening on
	jcph->message_id	= ++jcp_message_id;
	strcpy(jcp_ipaddr,ipaddr);
	jcp_bcast		= bcast;
	jcp_msg_type		= msg_type;

	if (payload_len>0)							// Got a payload ?
		memcpy(&jcp_msgbuf[sizeof(struct jcp_header)],payload,payload_len); // Append message to end of header
	jcp_msglen=sizeof(struct jcp_header) + payload_len;
//printf("HEADERLEN=%u payload_len=%d\n", sizeof(struct jcp_header), payload_len);
	os_printf("<%-16sJCP SEND   LEN:%-04d TYPE:%-05d ID:%-05d AT:%-05d BT:%-05d SA:%d REQA:%d\n",
		  jcp_ipaddr, jcp_msglen, jcp_msg_type, jcp_message_id, jcp_backoff_timer, jcp_attempts, send_ack, requires_ack);
	udp_generic_send((char*)&jcp_msgbuf, jcp_msglen, jcp_ipaddr, JCP_SERVER_PORT, jcp_bcast);
	jcp_attempts++;								// an attempt, even if the first
	if (requires_ack==TRUE)							// we intend to wait for an ACL
	{
		jcp_waiting_ack=jcp_message_id;					// ack should be for this message 
		jcp_waiting_on=jcp_msg_type;
	}
	//else	jcp_message_id++;						// then setup for the next send
}


// Re-send the last messages unchanged
void ICACHE_FLASH_ATTR jcp_resend()
{
	os_printf("<%-16sJCP RESEND LEN:%-04d TYPE:%-05d ID:%-05d AT:%-05d BT:%-05d\n",
		  jcp_ipaddr, jcp_msglen, jcp_msg_type, jcp_message_id, jcp_attempts, jcp_backoff_timer);
	udp_generic_send((char*)&jcp_msgbuf, jcp_msglen, jcp_ipaddr, JCP_SERVER_PORT, jcp_bcast);
	jcp_attempts++;	
}


// The ACK acknowledges receipt and having acted upon (even if partially) a message.
void ICACHE_FLASH_ATTR jcp_send_ack(char *ipaddr, int message_id, int port)
{
	struct jcp_header jcph;

	if (port<1024)
		return;
	memcpy(&jcph.magic,JCP_MAGIC,sizeof(JCP_MAGIC)-1);
	jcph.send_ack=FALSE;                                                            // We don't want a loop trying to ACK the ACK
	jcph.msg_type=JCP_MSG_ACK;
	jcph.session_id=session_id;
	jcph.message_id=message_id;                                                     // The message we are ACKing
        //jcph.reply_port=jcp_server_port;
	udp_generic_send((char*)&jcph, sizeof(jcph), ipaddr, port, FALSE);
	os_printf(">IP:%s:%d\tJCP_MSG_ACK ID:%d\n",ipaddr,port,message_id);
}




void ICACHE_FLASH_ATTR build_send_regiserdevs()
{
	char	buf[4096];					// keep small or ESP tends to lockup when runs out of stack space
	struct jcp_msg_register_devs *rd = (struct jcp_msg_register_devs*)buf;		// sit registration struct over buffer
	int 	i=0;
	int	c=0;
	int	ps=sizeof(struct jcp_msg_register_devs);

	if (jcp_server_ipaddr[0]==0)
	{
		os_printf("build_send_regiserdevs(), premature,  no server ip_addr yet\n");
		return;
	}

	strcpy(rd->magic,"REGD");
	rd->pver = 1;
	for (i=0;i<DEVT_MAX;i++)							// for all possible devices in table
	{
		if (jb[i].dev_type != DEV_NONE)						// table entry active ?
		{
			memcpy(&rd->dev[c++], &jb[i], sizeof(struct jcp_dev));		// build an array of jcp_dev into buffer
			if (c>1)
				ps=ps+sizeof(struct jcp_dev);				// keep a note of the payload size
		}
	}
	rd->num_of_devs=c;
	os_printf(">IP:%s\tJCP_MSG_REGISTER_DEVS, %d devices  %dbytes\n",jcp_server_ipaddr, c,ps);	// We are registering N devices
	jcp_send((char*)&jcp_server_ipaddr, FALSE, JCP_MSG_REGISTER_DEVS ,(char*)&buf, ps, TRUE, TRUE);
}





// Send a new state for the device, 
// idx is the index supplied when jcp_register_dev() was called, asciiorbinary 1=ascii
void ICACHE_FLASH_ATTR jcp_send_device_state(int idx, uint16_t value1, uint16_t value2, int asciiorbinary, char* valuebytes, int valuebyteslen)
{
	struct dev_state ds;

	ds.dev_type = jb[idx].dev_type;								// saved for init()
	memcpy(ds.uid, jb[idx].uid, UID_LEN);

	ds.value1 = value1;
	ds.value2 = value2;
	ds.asciiorbinary = asciiorbinary;
	ds.valuebyteslen = valuebyteslen;
	bzero(&ds.valuebytes, sizeof(ds.valuebytes));
	memcpy(&ds.valuebytes, valuebytes, valuebyteslen);
	jcp_send((char*)&jcp_server_ipaddr, FALSE, JCP_MSG_DEV_STATE,(char*)&ds, sizeof(struct dev_state), TRUE, TRUE);
}




// O/S Should call this at the expected frequency
void ICACHE_FLASH_ATTR jcp_timertick_handler()
{
	static int fcounter=0;									// fast counter
	static int pseccount=0;									// count in seconds until time to ping

	if (jcp_done_init!=TRUE)
		return;
	fcounter++;
	if (fcounter>=20)									// 20Hz call rate
	{
		fcounter=0;
		pseccount++;
	}

//os_printf("bt=%d\two=%d attempts=%d  jcp_connection_state=%d\n",jcp_backoff_timer,jcp_waiting_on, jcp_attempts, jcp_connection_state); 
	if (jcp_backoff_timer>0)
	{
		jcp_backoff_timer--;
		return;
	}


	if (jcp_attempts>8)									// tried 8 in a row fast
	{
		jcp_backoff_timer=jcp_attempts*2;						// start to go slower
		if (jcp_backoff_timer>40)							// server taking ages to respond	
		{
			jcp_initial_state();							// give up, restart from scratch
			//jcp_connection_state=0;
			return;					// Maybe reboot here instead ?
		}
	}

	if (jcp_waiting_on>0 && jcp_connection_state!=0)					// sent a packet, waiting for response
	{
		jcp_resend();
		return;										// one packet already waiting to be delivered
	}

	switch (jcp_connection_state)
	{
		case 0:
			jcp_send("255.255.255.255", TRUE, JCP_MSG_DISCOVER, NULL, 0, TRUE, TRUE);
			jcp_backoff_timer= ( (1000/jcp_tick_interval_ms)) * 15;			// 15 seconds per re-send
		break;


		case 1:										// Discovery complete, now register myself
			build_send_regiserdevs();
		break;


		// normal idle running state, send periodic pingpong, but defer if waiting for an ACK
		case 2:
			if ( (pseccount==10 || pseccount==20 || pseccount==30 || pseccount==40 || pseccount==50) && (fcounter==0) )
				os_printf("pingpong in %d seconds\n",60-pseccount);
			if (pseccount >= 60)							// time to send message ?
			{
				if (jcp_server_ipaddr[0]!=0)					// got an address for the server ?
				{
					jcp_send(jcp_server_ipaddr, FALSE, JCP_MSG_PINGPONG, NULL, 0, TRUE, TRUE);
					os_printf("<%-16sJCP_MSG_PINGPONG  ID:%d  PSECCOUNT:%d\n",jcp_server_ipaddr,jcp_message_id,pseccount);
				}
				pseccount=0;
			}
		break;
	}
}



// For a given UID what is the index in jb[] array ?  Returns index or -1 on error
int ICACHE_FLASH_ATTR jb_find_idx(unsigned char* uid)
{
	int i=0;

	for (i=0;i<DEVT_MAX;i++)
	{
		if (jb[i].dev_type!=0)
		{
			if (memcmp(jb[i].uid, uid, UID_LEN)==0)					// found uid in table ?
				return(i);
		}
	}
	return(-1);
}


// Process replies from server
void ICACHE_FLASH_ATTR jcp_parse_and_reply(char *data, int len, char *ipaddr)
{
	struct jcp_header *jcpheader = (struct jcp_header*)data;				
	struct dev_state  *ds        = (struct dev_state *)(data+sizeof(struct jcp_header));
	char* dt		     = (char*)(data+sizeof(struct jcp_header));			// Pointer to start of data
	struct jcp_msg_topic *mt     = (struct jcp_msg_topic*)(data+sizeof(struct jcp_header));	
	int idx=0;

	//os_printf("jcp_parse_and_reply, got %d bytes from %s\n",len,ipaddr);
	if (len<sizeof(struct jcp_header))							// Less than header size then it cant be valid
		return;

	if (memcmp(jcpheader->magic,JCP_MAGIC,sizeof(JCP_MAGIC)-1)==0)
	{
		if (jcpheader->session_id != jcp_server_session_id)				// Server re-started, we need to re-register
		{
			os_printf(">%-16sJCP server session_id changed from %d to %d\n",ipaddr,jcp_server_session_id,jcpheader->session_id);
			if (jcp_server_session_id==0)						// was not set
			{
				jcp_server_session_id=jcpheader->session_id;
				os_printf(">%-16sJCP server session_id set to %d\n",ipaddr,jcp_server_session_id);
			}
			else
			{
				os_printf(">%-16sJCP server session_id changed from %d to %d, restarting\n",ipaddr,jcp_server_session_id,jcpheader->session_id);
				system_restart();
			}
		}

		//switch (
		if (jcpheader->msg_type == JCP_MSG_DEV_STATE)					// server is sending new device state
		{
			idx=jb_find_idx(ds->uid);
			jcp_dev_state_cb(idx, (struct dev_state*)ds);
		}

		if (jcpheader->msg_type == JCP_MSG_TOPIC)					// Device sends info on a topic
		{
			idx=jb_find_idx((unsigned char*)mt->uid);
			jcp_topic_cb(idx, mt->uid, mt->topic, mt->tlen, mt->tdata);		// pass back to application
		}

		if (jcpheader->msg_type == JCP_MSG_DEV_CONFIG)					// Update device config record in flash
		{
			jcp_dev_config(dt, len-sizeof(struct jcp_header));			// Pass pointer to buffer and bytes passed
		}


		if (jcpheader->msg_type == JCP_MSG_ACK)
		{
			os_printf(">%-16sJCP ACK ID:%d  LAST SENT ID:%d\n",ipaddr,jcpheader->message_id,jcp_message_id);
			if (jcpheader->message_id == jcp_message_id)					// Is it Our ACK ?
			{
				if (jcp_waiting_ack>0)							// Are we waiting for an ACK?
				{
					os_printf("                 JCP Was our expected ACK ID:%d\n",jcpheader->message_id);
					switch (jcp_waiting_on)
					{
						case JCP_MSG_DISCOVER:
							os_printf("                 JCP Found server %s\n",ipaddr);
							strcpy(jcp_server_ipaddr,ipaddr);
							jcp_connection_state=1;	
						break;

						case JCP_MSG_REGISTER_DEVS:
							os_printf("                 JCP Got ACK for JCP_MSG_REGISTER_DEVS\n");
							jcp_dev_state_cb(-100, (struct dev_state*)&ds);
							jcp_connection_state=2;	
						break;
					}
					jcp_backoff_timer=0;
					jcp_waiting_ack=-1;						// Got our ack, no longer waiting
					jcp_waiting_on=0;
				}
			}
			else os_printf("                 JCP ACK ID:%d was unexpected\n",jcpheader->message_id);
		}

		if (jcpheader->send_ack==TRUE)
			jcp_send_ack((char*)&ipaddr, jcpheader->message_id, jcpheader->reply_port);
	}
}

