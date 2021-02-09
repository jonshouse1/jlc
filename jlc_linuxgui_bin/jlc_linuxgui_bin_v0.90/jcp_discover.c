/*
	jcp_discover.c
	Discover jlcd lighting controller using jcp protocol.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdint.h>
#include <sys/fcntl.h>


#include "jcp_protocol_devs.h"
//#include "jcp_client.h"
#include "jlc.h"

#define DEVT_MAX	8


// Prototypes
unsigned long long current_timems();
int udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast);
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




// Take a pre-built message payload (message excluding header) and send it. Every outbound message should be via this.
// send_ack tells the server to send a reply to the message, requires_ack asks jcp_client to possibly re-try
// sending and wait for positive ack to confirm the server received the message
void jcp_send(char *ipaddr, int bcast, int msg_type, char* payload, int payload_len, int send_ack, int requires_ack)
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
	printf("<%-16sJCP SEND   LEN:%-4d TYPE:%-5d ID:%-5d AT:%-5d BT:%-5d SA:%d REQA:%d\n",
		  jcp_ipaddr, jcp_msglen, jcp_msg_type, jcp_message_id, jcp_backoff_timer, jcp_attempts, send_ack, requires_ack);
	udp_generic_send((char*)&jcp_msgbuf, jcp_msglen, jcp_ipaddr, JCP_SERVER_PORT, jcp_bcast);
	jcp_attempts++;								// an attempt, even if the first
	if (requires_ack==TRUE)							// we intend to wait for an ACL
	{
		jcp_waiting_ack=jcp_message_id;					// ack should be for this message 
		jcp_waiting_on=jcp_msg_type;
	}
}


// Re-send the last messages unchanged
void jcp_resend()
{
	printf("<%-16sJCP RESEND LEN:%-4d TYPE:%-5d ID:%-5d AT:%-5d BT:%-5d\n",
		  jcp_ipaddr, jcp_msglen, jcp_msg_type, jcp_message_id, jcp_attempts, jcp_backoff_timer);
	udp_generic_send((char*)&jcp_msgbuf, jcp_msglen, jcp_ipaddr, JCP_SERVER_PORT, jcp_bcast);
	jcp_attempts++;	
}



// Returns file descriptor if successful or -1 on error
int jcp_udp_listen(int port)
{
        static struct sockaddr_in recvaddr;
        int flags;
        int dsockfd=-1;

        dsockfd=socket(PF_INET, SOCK_DGRAM, 0);
        if (dsockfd<0)
        {
                fprintf(stderr, "socket() failed: %s\n", strerror(errno));
                fflush(stderr);
                return(-1);
        }

        flags = fcntl(dsockfd, F_GETFL);
        flags |= O_NONBLOCK;
        if (fcntl(dsockfd, F_SETFL, flags) == -1)
        {
                fprintf(stderr,"error,fcnctl failed - could not set socket to nonblocking");
                fflush(stderr);
		close(dsockfd);
                return(-1);
        }

        recvaddr.sin_family = AF_INET;
        recvaddr.sin_port = htons(port);
        recvaddr.sin_addr.s_addr = INADDR_ANY;
        memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);

        if(bind(dsockfd, (struct sockaddr*) &recvaddr, sizeof recvaddr) == -1)
        {
                fprintf(stderr,"bind() failed %s\n",strerror(errno));
		close(dsockfd);
                return(-1);
        }
        else    return(dsockfd);
}



// Send UDP packet 
int udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast)
{
	struct	sockaddr_in cm_sendaddr;
	static int cm_sockfd=-1;
	int numbytes=0;
	int flags;
	
	if (cm_sockfd<0)
		cm_sockfd = socket(PF_INET,SOCK_DGRAM,0);
	if (cm_sockfd<0)
	{
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return (-1);
	}

	flags = fcntl(cm_sockfd, F_GETFL);						// Get the sockets flags
	flags |= O_NONBLOCK;								// Set NONBLOCK flag
	if (fcntl(cm_sockfd, F_SETFL, flags) == -1)					// Write flags back
	{
		perror("send_command_udp() ,fcnctl failed - could not set socket to nonblocking");
		exit(1);
	}

	if (broadcast==TRUE)
	{
		if((setsockopt(cm_sockfd,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof broadcast)) == -1)
		{
			perror("setsockopt - SO_SOCKET for udp tx socket failed");
			exit(1);
		}
	}
	cm_sendaddr.sin_family = AF_INET;
	cm_sendaddr.sin_port = htons(destination_port);
	cm_sendaddr.sin_addr.s_addr = inet_addr(destination_ip);
	memset(cm_sendaddr.sin_zero,'\0',sizeof(cm_sendaddr.sin_zero));

	numbytes = sendto(cm_sockfd, d, len, 0, (struct sockaddr *)&cm_sendaddr, sizeof(cm_sendaddr));
	return(numbytes);
}





// Returns 1 if success, 0 if no server is found and negative on an error
int jcp_find_server(char* ipaddr, int timeoutms)
{
	int p=0;
	int c=0;
	int sockfd=-1;
        char rbuffer[8192];
	int timeout=FALSE;
	int searchtime=0;
	int starttime=0;
	int numbytes=0;
	static struct sockaddr_in recvaddr;
        unsigned int addr_len=sizeof(recvaddr);
	int sendtimer=0;
	struct jcp_header *jcph = (struct jcp_header*)&rbuffer;				// sit the header on our buffer

        // Find a port to listen on, start with the server listing port+1 and work up. Do not specify
        // SO_REUSEPORT or SO_REUSEADDR as we want to be the only listener on the port
	p=JCP_SERVER_PORT+10;
	c=0;
	do
	{
		c++;
		sockfd=jcp_udp_listen(p);						// Try and open a port for listening
		if (sockfd<0)								// open failed
			p++;								// try next port
		if (c>60)
			return (-1);							// Cant find a port to listen on
	} while (sockfd<0);

	jcp_udp_port=p;
	printf("Listening JCP messages on UDP port %d\n",jcp_udp_port);
	fflush(stdout);


	sendtimer=0;
	starttime=current_timems();
	do
	{
		if (sendtimer<=0)
		{
			jcp_send("255.255.255.255", TRUE, JCP_MSG_DISCOVER, NULL, 0, TRUE, TRUE);
			sendtimer=500;							// send ever N milliseconds
		}
		bzero(&rbuffer,sizeof(rbuffer));
        	numbytes = recvfrom (sockfd, &rbuffer, sizeof(rbuffer)-1, 0, (struct sockaddr *)&recvaddr, &addr_len);
        	if (numbytes>0)
        	{
                	sprintf(ipaddr,"%s",(char*)inet_ntoa(recvaddr.sin_addr));
                	if (numbytes>=sizeof(struct jcp_header))
                	{
				if (memcmp(jcph->magic,JCP_MAGIC,sizeof(JCP_MAGIC))==0)	// good magic ?
				{
					printf("Reply from %s, header has good magic [%s]\n",ipaddr,jcph->magic);
					fflush(stdout);
					close(sockfd);
					return(1);
				}
				else
				{
					printf("Got reply from %s but bad magic, ignored\n",ipaddr);
					fflush(stdout);
				}
                	}
        	}
		searchtime = current_timems() - starttime;
		if (searchtime > timeoutms)
		{
			close(sockfd);
			return(0);							// timeout looking
		}
		usleep(1000);								// wait 1ms
		sendtimer--;								// we are 1ms closer to a resend
	} while (timeout==FALSE);

	close(sockfd);
	return(0);
}




