// jlc_linuxgui_tcp.c

#include "jcp_protocol_devs.h"
#include "jlc_linuxgui.h"
#include "jlc.h"

#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <ctype.h>
#include "jlc_group.h"

#include "jlc_prototypes.h"


extern int			online;



// TCP helper,  local printf style function
// Returns TRUE if the write was good
void xprintf(int fd, const char *fmt, ...)
{
        va_list arg;
        char buf[8192];
        int w=0;

        if (fd<0)
                return;
        va_start(arg, fmt);
        buf[0]=0;
        vsprintf((char*)&buf, fmt, arg);
        if (strlen(buf)>0 && fd>0)
        {
                w=write(fd,&buf,strlen(buf));
		if (w<0)
                {
			printf("xprintf disconnected [%s]\n",strerror(errno)); fflush(stdout);
                        //shutdown(fd,SHUT_RDWR);
			close(fd);
			online=FALSE;
			return;
                }
        }
        va_end(arg);
}




// Return file descriptor for connected socket
int sockinit(struct sockaddr_in* server, char* server_ipaddr, int server_tcpport, int nonblock)
{
	int sock;

	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) 
	{
		//perror("Could not create socket. Error");
		printf("Could not create socket, %s\n",strerror(errno));
		return -1;
	}

	server->sin_addr.s_addr = inet_addr(server_ipaddr);
	server->sin_family = AF_INET;
	server->sin_port = htons(server_tcpport);

	//Connect to remote server
	if (connect(sock, (struct sockaddr*)server , sizeof(struct sockaddr_in)) < 0) 
	{
		close(sock);
		printf("Connection failed, %s\n",strerror(errno));
		return -1;
	}
    	
	if (nonblock==TRUE)
	{
		int flags = fcntl(sock,F_GETFL,0);
        	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK)<0)
        	{
			printf("sockinit() Cant set socket non blocking, %s\n",strerror(errno));
			close(sock);
			return -1;
        	}
	}
	return(sock);
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



// Read bytes discarding them until we hit character x or timeout
// Returns -1 on timeout, 1 for found character x
int discard_until(int sockfd, int timeout_sec, char x)
{
	int sec=current_times();
	char c=0;
	int n=0;

	do
	{
		n=recv(sockfd, &c, 1, MSG_PEEK | MSG_DONTWAIT);					// Try and read 1
		if (n==1)
		{
			if (c!=x)								// Not the byte we are looking for
			{
				n=read(sockfd, &c, 1);						// read 1 character for real
				//printf("discarded %02X ",(unsigned char)c);
				if (c>=32)
					printf(" [%c]",c);
				//printf("\n");
				//fflush(stdout);
			}
			else	return(1);							// We read upto the byte before x
		}

		if (current_times()-sec > timeout_sec)
			return(-1);								// timeout
	} while (1);
}



// Peek ahead into file until we hit JLCB.  Then read until we just before the 'J'
// Leaves us aligned to the first occurrence of a binary header
int align_for_binary_mode(int sockfd)
{
	int lc=0;

	do
	{
		if (discard_until(sockfd, 1, 'J')==1)
			return(TRUE);
		lc++;
		usleep(1000);									// 1ms
	} while(lc<5000); 									// timeout ms
	return(FALSE);										// timed out
}






// always reads one ASCII line into buf or does no read at all
// returns a positive number with the line length, 0 or something negative if no line is available
int poll_tcpsocket_line(int sockfd, char* buf)
{
	int i=0;
	int n=0;
	char b[8192];
	int offset=0;

	bzero(&b,sizeof(b));
	n=recv(sockfd, &b, sizeof(b)-1, MSG_PEEK | MSG_DONTWAIT);				// Look at all pending data	
	if (n>0)
	{
		for (i=n;i>0;i--)
		{
			if (b[i]=='\n')	
				offset=i;							// look for newlines closest to buffer start
		}
		if (offset!=0)
		{
			n=recv(sockfd,buf,offset+1,MSG_DONTWAIT);				// Read upto first newline
			//printf("offset=%d n=%d\n",offset,n); fflush(stdout);
			return(n);								// return the line length
		}
	}
	return(-1);										// no newline in buffer yet
}



// Returns one ASCII line or possibly a partial line,  or -1
int poll_tcpsocket(int sockfd, char*buf)
{
	int n=0;
	int i=0;
	int newline=FALSE;
	char b[16384];

	buf[0]=0;
	buf[1]=0;
	i=0;
	do
	{
		n=read(sockfd,&b[i],1);
		if (n>0)
		{
			if (b[i]=='\n')
				newline=TRUE;
			i++;
		}
	} while (n>0 && newline!=TRUE);
	b[i]=0;
	if (i>0)
		memcpy(buf,&b,i+1);
	return(i);
}






void poll_parse_monitor(int fd, char* buf)
{
	int n=0;

	n=0;
	do 
	{
		n=poll_tcpsocket_line(fd, buf);
		if (n>0)
		{
			buf[n-1]=0;					// shorten to remove \n
			//parse_server_state(monitorbuf, sizeof(monitorbuf), n);
		}
	} while (n>0);
}





