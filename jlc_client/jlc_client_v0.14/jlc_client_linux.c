/*
	jlc_client_linux.c
	Linux test code for jcp_client.c
	
	Version 0.14
	(c) 2017 J.Andrews (jon@jonshouse.co.uk)
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "parse_commandline.h"
#include "jcp_client.h"


unsigned char jcp_uidbase[6];
int quit=FALSE;
int button=FALSE;						// not on

// Prototypes
char console_poll();
int udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast);



// Used by jlc_client.c
char* printuid(char unsigned *uid)
{
	int i=0;
	unsigned char c=0;
	static char hexstring[32];
	char *hexstringp=&hexstring[0];
	char st[3];

	bzero(&hexstring,sizeof(hexstring));
	for (i=0;i<UID_LEN;i++)
	{
		c=uid[i];
		sprintf(st,"%02X",(unsigned char)c);
		strcat(hexstring,st);
	}
	return(hexstringp);
}



void jcp_dev_state_cb(int idx, unsigned char* uid, int value1, int value2, char valuebytes[MAX_VALUEBYTESLEN])
{
	printf("jcp_dev_state_cb()\tidx=%d\tuid=%s\tvalue1=%d\tvalue2=%d\n",idx, printuid(uid), value1, value2);
	if (idx==-1 && value1==-1 && value2==-1)					// registered with server
		return;
	fflush(stdout);
	button=value1;
}


//void jcp_topic_cb(int message_type, char* data, int len)
void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
	//printf("jcp_topic_cb() got message_type=%d, len=%d, [%s]\n",message_type, len, data);
	printf("jcp_topic_cb() got topic=%d, uid=%s, len=%d  data=[%s]\n",topic, printuid(uid), tlen, tdata);
	fflush(stdout);
}


// Server sent us configuration information, stored in flash on devices
void jcp_dev_config(char *dt, int len)
{
}


void xprintf(int fd, const char *fmt, ...)
{
        va_list arg;
        char buf[8192];
        //int w=0;

        va_start(arg, fmt);
        buf[0]=0;
        vsprintf((char*)&buf, fmt, arg);
        if (strlen(buf)>0 && fd>0)
        {
		printf("%s",buf);
                //w=write(fd,&buf,strlen(buf));
                //if (w<0)
                //{
                        //conn_disconnect(fd);
                        //shutdown(fd,SHUT_RDWR);
                //}
        }
        va_end(arg);
}


unsigned long long current_timems()
{
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        return milliseconds;
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
		return(-1);
	}

	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = INADDR_ANY;
	memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);

	if(bind(dsockfd, (struct sockaddr*) &recvaddr, sizeof recvaddr) == -1)
	{
        	fprintf(stderr,"bind() failed %s\n",strerror(errno));
		return(-1);
	}
	else	return(dsockfd);
}



void jcp_message_poll(int dsockfd)
{
	//int i=0;
	int numbytes=0;
	char ipaddr[MAX_IPSTR_LEN];
	static struct sockaddr_in recvaddr;
	unsigned int addr_len=sizeof(recvaddr);
	//int valid=FALSE;

	// Allocate a buffer, place two structures on top of that buffer
	char rbuffer[4096];

	numbytes = recvfrom (dsockfd, &rbuffer, sizeof(rbuffer)-1, 0, (struct sockaddr *)&recvaddr, &addr_len);
        if (numbytes>0)
        {
		sprintf(ipaddr,"%s",(char*)inet_ntoa(recvaddr.sin_addr));
		//printf("Got %d bytes from %s\n",numbytes,ipaddr); fflush(stdout);
		if (numbytes>=sizeof(struct jcp_header))
		{
			jcp_parse_and_reply((char*)&rbuffer, numbytes, (char*)&ipaddr);
		}
		bzero(&rbuffer,sizeof(rbuffer));
	}
}



void usage()
{
	printf("Help\n");
	printf("jlc_client -d NAME\n");
	printf("	SWITCH\n");
	printf("	MOVEMENTSENSOR\n");
	printf("	TEMPSENS\n");
	printf("	SUBS\n");
	exit(0);
}


int main(int argc, char **argv)
{
	int p=0;
	int sockfd=-1;
	int c=0;
	unsigned char mymac[6]={0x01,0x02,0x03,0x04,0x05,0x06};
	uint16_t sid=0;
	char cmstring[1024];
	char st[1024];
	int argnum=0;
	int dev=0;
	int i=0;
	time_t t;

	unsigned int ct=0;						// current time
	unsigned int pt=0;						// previous time

	srand((unsigned) time(&t));
        strcpy(cmstring,"-h");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		usage();
	
	st[0]=0;
        strcpy(cmstring,"-d");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
        {
                argnum=parse_findargument(argc,argv,cmstring);
                if (argnum>0)
                {
                        strcpy(st,argv[argnum]);
			for (i=0;i<strlen(st);i++)
				st[i]=toupper(st[i]);
			if (strlen(st)<1)
			{
				printf("jlc_client -d <name>\n");
				exit(1);
			}
			if (strncmp(st,"SWI",3)==0)
				dev=DEVN_SWITCH;
			if (strncmp(st,"MOVE",4)==0)
				dev=DEVN_MOVEMENTSENSOR;
			if (strncmp(st,"TEMP",4)==0)
				dev=DEVN_TEMPSENS;
			if (strncmp(st,"SUBS",4)==0)
				dev=-100;
		}
	}
	if (dev==0)
		usage();
	sid=rand();

	// Find a port to listen on, start with the server listing port+1 and work up. Do not specify
	// SO_REUSEPORT or SO_REUSEADDR as we want to be the only listener on the port
	p=JCP_SERVER_PORT+1;
	c=0;
	do
	{
		sockfd=jcp_udp_listen(p);				// Try and open a port for listening
		c++;
		if (c>40)
		{
			printf("Tried 30 UDP ports, giving up\n");
			exit(1);
		}
	} while (sockfd<0);
	printf("Listening JCP messages on UDP port %d\n",p);
	fflush(stdout);

	jcp_init_b((char*)&mymac, sid, p, JLP_SERVER_PORT, (1000/JCP_TIMER_FREQ_HZ), "MODEL", "FWVER");	// setup jcp_client
	//jcp_init_b(char *uidb, uint16_t sid, int jcp_udp_iport, int jlp_udp_iport, int tick_interval_ms, char* device_model, char* device_fw_ver)

	if (dev>0)
		jcp_register_dev(0,dev,1,"0000000000000000");		// we have one device, index 0
	else	jcp_register_dev(0,DEVN_DUMMY,0,"0000000000000000");

	if (dev==DEVN_SWITCH)
	{
		printf("Space bar toggles switch,  l simulates long hold\n");
		fflush(stdout);
	}

	while (quit!=TRUE)
	{
		jcp_message_poll(sockfd);				// call into jcp_client
		ct=current_timems();
		//printf("%u\n",(ct-pt)); fflush(stdout);
		if ( (ct-pt) > (1000/JCP_TIMER_FREQ_HZ) )		// call into jcl_client on timer
		{
			jcp_timertick_handler();
			pt=ct;
		}


		switch (dev)
		{
			case -100:
			break;

			case DEVF_DMX:
			break;

			case DEVN_SWITCHPBT:
				c=console_poll();
				if (c==' ')
				{
					button=!button;
					fflush(stdout);
					sprintf(st,"SW=%d\n",button);
					jcp_send_device_state(0, button, 0, 1, (char*)&st, strlen(st));
				}	
				if (c=='l')
				{
					fflush(stdout);
					sprintf(st,"SW=%d\n",button);
					jcp_send_device_state(0, button, 1, 1, (char*)&st, strlen(st));
				}
			break;

			case DEVN_MOVEMENTSENSOR:
			break;

			case DEVN_POT:
			break;

			case DEVN_TEMPSENS:
			break;
		}
		usleep(1000);
	}

	printf("shutting down\n");
	fflush(stdout);
	close(p);
}


