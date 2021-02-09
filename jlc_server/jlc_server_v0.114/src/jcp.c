/*
	Jons Control Protocol

	This protocol is mainly responsible for discovery, groups and devices
	For small sensors and details "about" lighting units, but mostly
	not for lighting data.
*/


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "jcp_protocol_devs.h"
#include "jlc.h"
#include "jlc_prototypes.h"
#include "jlp.h"


extern uint16_t session_id;
static int message_id=0;
int jcp_server_port=-1;
struct    statss         stats;


// FIXME: Changing this will break some stuff....

// Stuff we got sent the last time we heard from an IP, also note extra fields
struct jcp_lh_time
{
	struct	jcp_header	hdr;
	char			ipaddr[MAX_IPSTR_LEN];
	unsigned int		lastheard_ms;
};
struct  jcp_lh_time		lh[MAX_IPTABLE];



// Show IP addresses and the last time we heard from them
void list_ip_details(int textout)
{
	int i=0;
	unsigned int h=0;

	h=0;
	for (i=0;i<MAX_IPTABLE;i++)
	{
		if (lh[i].ipaddr[0]!=0)					// in use ?
			h++;
	}

	xprintf(textout,"IP Addresses: %d active with server\n",h);
	for (i=0;i<MAX_IPTABLE;i++)
	{
		if (lh[i].ipaddr[0]!=0)					// in use ?
		{
			xprintf(textout," IP:%-15s ",lh[i].ipaddr);
			h = (unsigned int)current_timems();
			h = h - (unsigned int)lh[i].lastheard_ms;
			h = h / 1000;
			xprintf(textout,"last heard %02u seconds ago\n",h);
		}
	}
}




int jcp_init(int port, int fl_reuseport)
{
	int dsockfd=-1;
	//int i=0;
	static struct sockaddr_in recvaddr;

	//for (i=0;i<MAX_IPTABLE;i++)
	//{
		//bzero(&lh[i],sizeof(struct jcp_lh_time));
	//}
	bzero(&lh[0],sizeof(lh));

	jcp_server_port=port;
	dsockfd=socket(PF_INET, SOCK_DGRAM, 0);	
	if (dsockfd<0)
	{
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		fflush(stderr);
		return(-1);
	}

	int flags = fcntl(dsockfd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(dsockfd, F_SETFL, flags) == -1)
	{
		fprintf(stderr,"error,fcnctl failed - could not set socket to nonblocking");
		fflush(stderr);
		return(-1);
	}

	int optval=1;
	if (fl_reuseport==TRUE)
	{
		if (setsockopt(dsockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))<0)
		{
        		fprintf(stderr,"SO_REUSEPORT failed, not available on this kernel ?\n");
		}
	}
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = INADDR_ANY;
	memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);


	if(bind(dsockfd, (struct sockaddr*) &recvaddr, sizeof recvaddr) == -1)
	{
        	fprintf(stderr,"bind() failed\n");
		return(FALSE);
	}
	printf("Listening JCP messages on UDP port %d\n",port);
	return(dsockfd);
}



// Append payload to the end of a header and send it

/*
	TODO
	Some mechanism to send a message requiring an ACK, tick of the ACK or resend if no ACK
*/
void jcp_send_payload(char *ipaddr, int port, int msg_type, char *payload, int payloadlen)
{
	char buf[4096];
        struct jcp_header	*jcph=(struct jcp_header*)&buf;
	int ps=0;

	bzero(&buf,sizeof(buf));
	memcpy(&jcph->magic,JCP_MAGIC,sizeof(JCP_MAGIC)-1);
	jcph->send_ack=FALSE;
	jcph->msg_type=msg_type;
	jcph->session_id=session_id;
	jcph->message_id=message_id++;							// The message we are ACKing
	//jcph->reply_port=jcp_server_port;
	memcpy((char*)&buf[sizeof(struct jcp_header)], payload, payloadlen);		// copy in payload
	ps=sizeof(struct jcp_header)+payloadlen;
	//printf("jcp_send_payload %s:%d ps:%d\n",ipaddr,port,ps); fflush(stdout);
	udp_generic_send((char*)&buf, ps, ipaddr, port, FALSE);
	//dumphex(&buf,ps);

 	stats.pps_jcp++;								// packets per second
	stats.bps_jcp=stats.bps_jcp+ps;							// bytes per second
}



// The ACK acknowledges receipt and having acted upon (even if partially) a message.
void jcp_send_ack(char *ipaddr, int message_id, int port)
{
        struct jcp_header jcph;

	if (port<1024)
		return;
	memcpy(&jcph.magic,JCP_MAGIC,sizeof(JCP_MAGIC)-1);
	jcph.send_ack=FALSE;								// We don't want a loop trying to ACK the ACK
	jcph.msg_type=JCP_MSG_ACK;
	jcph.session_id=session_id;
	jcph.message_id=message_id;							// The message we are ACKing
	//jcph.reply_port=jcp_server_port;
	udp_generic_send((char*)&jcph, sizeof(jcph), ipaddr, port, FALSE);
	monitor_printf(MSK_JCP,">IP:%s:%d\tJCP_MSG_ACK ID:%d\n",ipaddr,port,message_id);
}



// Look at the table of IPs and last heard times, find our IP in table, failing that make a new entry
void jcp_update_lastheard(char *ipaddr, struct jcp_header *jh)
{
	int i=0;
	int idx=-1;

	for (i=0;i<MAX_IPTABLE;i++)							// does the IP exist in the table
	{
		//printf("byte = %d \n",lh[i].ipaddr[0]); fflush(stdout);
		if (lh[i].ipaddr[0]==0)							// blank record ?
			idx=i;								// take a note of it
		else
		{
			//printf("%d [%s]==[%s]?\n",i, lh[i].ipaddr, ipaddr); fflush(stdout);
			if (strcmp(lh[i].ipaddr, ipaddr)==0)				// found address in table?
			{
				//printf("lh found ip index %d , updating time\n",i); fflush(stdout);
				idx=i;
				break;							// exit for loop
			}
		}
	}										// now we found a slot
	if (idx==-1)
	{
		printf("jcp_update_lastheard() No more table entries?\n");
		exit(1);
	}

	//printf("modifying idx %d\n",idx); fflush(stdout);
	strcpy(lh[idx].ipaddr, ipaddr);							// new table entry, add ip
	lh[idx].lastheard_ms=current_timems();						// and time
	memcpy(&lh[idx].hdr, jh, sizeof(struct jcp_header));				// take a copy of the header
}



// When was this IP last heard from (milliseconds)
int jcp_lastheard_ms(char *ipaddr)
{
	int i=0;
	for (i=0;i<MAX_IPTABLE;i++)							// does the IP exist in the table
	{
		if (strcmp(lh[i].ipaddr, ipaddr)==0)					// found address in table
			return(lh[i].lastheard_ms);
	}
	return(0);
}


// Have we seen this exact same message before ?
// returns true if found
int jcp_detect_duplicate(char *ipaddr, int message_id)
{
	int i=0;

	for (i=0;i<MAX_IPTABLE;i++)							// does the IP exist in the table
	{
		if (lh[i].ipaddr[0]!=0)							// active table entry ?
		{
			if (strcmp(lh[i].ipaddr,ipaddr)==0)				// Our IP ?
			{
				//printf("lh[%d].hdr.message_id=%d == %d?\n",i,lh[i].hdr.message_id, message_id); fflush(stdout);
				if (lh[i].hdr.message_id == message_id)
				{
					lh[i].lastheard_ms = current_timems();		// Message is the same, but update the time	
					return(TRUE);
				}
			}
		}
	}
	return(FALSE);
}





// Receive and decode messages from clients,  some messages ask for an positive acknowledgement
void jcp_message_poll(int dsockfd)
{
	int i=0;
	int numbytes=0;
	char ipaddr[MAX_IPSTR_LEN];
	static struct sockaddr_in recvaddr;
	unsigned int addr_len=sizeof(recvaddr);
        struct jcp_header txheader;							//  For sending

        // Allocate a buffer, place two structures on top of that buffer
        char rbuffer[8192];
        struct jcp_header 		*jcpheader =(struct jcp_header*)&rbuffer;
	//struct dev_state		*ds	   =(struct dev_state*)(&rbuffer+sizeof(struct jcp_header));
	struct jcp_msg_register_devs	*rdevs     =(struct jcp_msg_register_devs*)((char*)&rbuffer+sizeof(struct jcp_header));


	numbytes = recvfrom (dsockfd, &rbuffer, sizeof(rbuffer)-1, 0, (struct sockaddr *)&recvaddr, &addr_len);
	if (numbytes>0)
	{
		sprintf(ipaddr,"%s",(char*)inet_ntoa(recvaddr.sin_addr));
		if (numbytes>=sizeof(struct jcp_header))
		{
			if (memcmp(jcpheader->magic,JCP_MAGIC,sizeof(JCP_MAGIC)-1)==0)
			{
				if (jcpheader->reply_port<1024)
				{
					monitor_printf(MSK_JCP,"<IP:%s\tID:%d MSG_TYPE:%d RP:%d  Error, reply_port must be >1024\n",ipaddr,
					jcpheader->message_id,jcpheader->msg_type,jcpheader->reply_port);
					return;						// Ignore this packet
				}

				// Check if received message is a duplicate, if so we don't want to process it again, but do ack it
				if (jcp_detect_duplicate((char*)&ipaddr, jcpheader->message_id)==TRUE)
				{
					monitor_printf(MSK_JCP,"<IP:%s\tDUP\tMSG_TYPE:%d ID:%d\n",ipaddr,jcpheader->msg_type,jcpheader->message_id);
					if (jcpheader->send_ack==TRUE)
						jcp_send_ack((char*)&ipaddr, jcpheader->message_id, jcpheader->reply_port);
					return;
				}

				jcp_update_lastheard((char*)&ipaddr, jcpheader);
				switch(jcpheader->msg_type)
				{
					case JCP_MSG_PINGPONG:		// Clients should not do this until they have registered all devs
						monitor_printf(MSK_JCP,"<IP:%s\tJCP_MSG_PINGPONG ID:%d RP:%d\n",ipaddr,jcpheader->message_id, jcpheader->reply_port);
					break;


					case JCP_MSG_DISCOVER:
						monitor_printf(MSK_JCP,"<IP:%s\tJCP_MSG_DISCOVER ID:%d\n",ipaddr,jcpheader->message_id);
						memcpy(&txheader.magic,JCP_MAGIC,sizeof(JCP_MAGIC)-1);
						txheader.send_ack	= FALSE;
						txheader.msg_type	= JCP_MSG_DISCOVER_REPLY;
						txheader.session_id	= session_id;
						txheader.message_id	= jcpheader->message_id;
						txheader.reply_port	= jcp_server_port;
						udp_generic_send((char*)&txheader, sizeof(txheader), ipaddr, jcpheader->reply_port, FALSE);
						monitor_printf(MSK_JCP,">IP:%s\tJCP_MSG_DISCOVER_REPLY ID:%d RPORT:%d\n",
								ipaddr, jcpheader->message_id, jcpheader->reply_port);
					break;


					case JCP_MSG_REGISTER_DEVS:
						//dumphex(&rbuffer,256);
						if (memcmp(rdevs->magic,"REGD",4)==0)
						{
							monitor_printf(MSK_JCP,"<IP:%s\tJCP_MSG_REGISTER_DEVS ID:%d  NOD:%d\n",
									ipaddr,jcpheader->message_id, rdevs->num_of_devs);
							//printf("Got %d devices\n",rdevs->num_of_devs); fflush(stdout);
							for (i=0;i<rdevs->num_of_devs;i++)		// For each device described in message
							{
								if (rdevs->dev[i].dev_type!=0)
									dev_jlc_register(&rdevs->dev[i], ipaddr);
							}
						}
						else
						{
							monitor_printf(MSK_JCP,"IP:%s Bad magic for register devs\n",ipaddr); 
							fflush(stdout);
						}
					break;


					case JCP_MSG_ACK:				// does nothing, but if send_ack is true header+ack goes back
					break;

					case JCP_MSG_DEV_STATE:
						device_update_state(ipaddr, (struct dev_state*)&rbuffer[sizeof(struct jcp_header)]);
					break;

					//case JCP_MSG_SUBSCRIBE:
						//subscribe_topics(ipaddr, jcpheader->reply_port, (struct jcp_msg_subscribe*)&rbuffer[sizeof(struct jcp_header)]);
					//break;
				}
				if (jcpheader->send_ack==TRUE)
					jcp_send_ack((char*)&ipaddr, jcpheader->message_id, jcpheader->reply_port);
			}
			else monitor_printf(MSK_JCP,"<IP:%s LEN:%d BAD MAGIC\n",ipaddr,numbytes);
		}
		bzero(jcpheader,sizeof(struct jcp_header));
	}
}



// POSSIBLE BUG: ipaddr not being set maybe ?
// parse [*REG 11/09/2020 11:20:43 <IP:7G  TIMEOUT after 120 seconds]
// parse [*REG 11/09/2020 11:20:43 <IP:g   TIMEOUT after 120 seconds]
// parse [*REG 11/09/2020 11:20:43 <IP:5g  TIMEOUT after 120 seconds]
// parse [*REG 11/09/2020 11:20:43 <IP:6g  TIMEOUT after 120 seconds]
// parse [*SCR 11/09/2020 11:20:43 [scripts/kitchensign]]


void jcp_timeout_check()
{
	int i=0;

	for (i=0;i<MAX_IPTABLE;i++)
	{
		if (lh[i].ipaddr[0]!=0)					// entry in table ?
		{
			if ((unsigned int)current_timems() - lh[i].lastheard_ms > (JCP_TIMEOUT_S*1000))
			{
				//printf("timeout, i=%d  ip=[%s] lastheard_ms=%u now=%u\n",i,lh[i].ipaddr,lh[i].lastheard_ms,current_timems()); fflush(stdout);
				monitor_printf(MSK_REG,"IP:%s\tTIMEOUT after %d seconds\n",lh[i].ipaddr,JCP_TIMEOUT_S);
				dev_timeout(lh[i].ipaddr);
				bzero(&lh[i].ipaddr,MAX_IPSTR_LEN);
				lh[i].lastheard_ms=0;
			}
		}
	}
}


