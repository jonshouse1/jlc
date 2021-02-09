// jcp-client-linux.c

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>

#include <errno.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <arpa/inet.h>

#include "jcp_protocol_devs.h"
#include "jcp_client.h"
#include "jlc.h"


static int jcp_sock=-1;

// For a given interface return the MAC address
unsigned long long current_timems()
{
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        return milliseconds;
}


int getmac(char *iface, char *mac)
{
        struct ifreq s;
        int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
        int i;

        printf("Get MAC for interface [%s]\n",iface); fflush(stdout);
        strcpy(s.ifr_name, iface);
        if (0 == ioctl(fd, SIOCGIFHWADDR, &s))
        {
                for (i = 0; i < 6; ++i)
                        mac[i]=(unsigned char) s.ifr_addr.sa_data[i];
                return 0;
        }
        return 1;
}

// Returns file descriptor if sucessful or -1 on error
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
        else    return(dsockfd);
}



void jcp_message_poll(int dsockfd)
{
        int numbytes=0;
        char ipaddr[MAX_IPSTR_LEN];
        static struct sockaddr_in recvaddr;
        unsigned int addr_len=sizeof(recvaddr);

        // Allocate a buffer, place two structures ontop of that buffer
        char rbuffer[8192];

        numbytes = recvfrom (dsockfd, &rbuffer, sizeof(rbuffer)-1, 0, (struct sockaddr *)&recvaddr, &addr_len);
        if (numbytes>0)
        {
                sprintf(ipaddr,"%s",(char*)inet_ntoa(recvaddr.sin_addr));
                printf("Got %d bytes from %s\n",numbytes,ipaddr); fflush(stdout);
                if (numbytes>=sizeof(struct jcp_header))
                {
                        jcp_parse_and_reply((char*)&rbuffer, numbytes, (char*)&ipaddr);
                }
        }
}


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


// Retuns the port it is listening on
int jcp_linux_udplisten()
{
	int c=0;
	int p=0;

	// Find a port to listen on, start with the server listing port+1 and work up. Do not specify
	// SO_REUSEPORT or SO_REUSEADDR as we want to be the only listener on the port
	p=JCP_SERVER_PORT+10;
	c=0;
	do
	{
		jcp_sock=jcp_udp_listen(p);							// Try and open a port for listening
		c++;
		if (c>40)
		{
			printf("Tried 40 UDP ports, giving up\n");
			exit(1);
		}
	} while (jcp_sock<0);
	printf("Listening JCP messages on UDP port %d\n",p);
	fflush(stdout);
	return(p);
}



int jcp_poll()
{
	unsigned int ct=0;									// current time
	static unsigned int pt=0;								// previous time

	jcp_message_poll(jcp_sock);
	ct=current_timems();
	if ( (ct-pt) > (1000/JCP_TIMER_FREQ_HZ) )
	{
		jcp_timertick_handler();
		//printf("."); fflush(stdout);
		pt=ct;
	}
	return(jcp_sock);
}
