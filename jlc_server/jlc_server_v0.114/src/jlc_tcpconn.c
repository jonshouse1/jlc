/*
	jlc_tcpconn.c
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
#include <errno.h>



#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"


int listenfd=-1;
struct sockaddr_in serv_addr;
extern int conn;
extern char                             version[];
extern struct tcp_connection		conns[MAX_TCPCONN];

extern char* binpayload;



void conn_set_defaults(int i)
{
	//printf("conn_set_defaults %d\n",i); fflush(stdout);
	if (i<0)
		return;
	conns[i].mode		= CONN_MODE_LINEBYLINE;	
	conns[i].fd		= -1;
	conns[i].m		= 0;
	conns[i].colour		= FALSE;
	conns[i].t		= 0;
	conns[i].idx		= 0;
	conns[i].gotheader	= FALSE;

	strcpy(conns[i].prompt, CONN_PROMPT);
	bzero(&conns[i].cmd,sizeof(conns[i].cmd));
	bzero(&conns[i].ipaddr,sizeof(conns[i].ipaddr));
	bzero(&conns[i].hdr, sizeof(struct bin_header));
}

/*
struct tcp_connection
{
        int             mode;                                                   // One of TCP_CONN_MODE
        int             fd;
        uint32_t        m;                                                      // One bit per type of event we want to be told about
        char            cmd[MAX_CMDLEN];                                        // Command line being entered
        char            prompt[MAX_PROMPTLEN];
        int             colour;                                                 // TRUE for ANSI colour sequence, default FALSE=plain text
        char            ipaddr[32];

        // These are for binary mode only
        int             t;                                                      // table (one of BIN_MSG_)
        int             idx;                                                    // index (the next entry to send)

        int             gotheader;                                              // True if the have read the header, but not the payload
        struct bin_header       hdr;
        //char          payload[65537];                                         // unsigned 16 bits worth
};
*/


void init_conn()
{
	int i=0;

        for (i=0;i<MAX_TCPCONN;i++)
                conn_set_defaults(i);
}


int conn_count()
{
	int i=0;
	int c=0;

	for (i=0;i<MAX_TCPCONN;i++)
	{
		if (conns[i].fd>=0)
			c++;
	}
	return(c);
}


// find index from file descriptor,  value is 0 to MAX_TCPCONN or negative if failed
int conn_find_index(int fd)
{
	int i=0;

	for (i=0;i<MAX_TCPCONN;i++)
	{
		if (conns[i].fd==fd)
			return(i);
	}
	return(-1);
}


// Hangup TCP/IP and prepare slot for next connection 
void conn_disconnect(int i, int caller)
{
	//printf("conn_disconnect() caller=%d\ti=%d\tfd=%d\n",caller,i,conns[i].fd); fflush(stdout);
	if (i<0)
		return;										// may really happen
	shutdown(conns[i].fd,SHUT_RDWR);							// Hangup TCP connection
	close(conns[i].fd);
	conn_set_defaults(i);									// clear entry, ready for next connection
	monitor_printf(MSK_TCP,"TCP session FD:%d ended\n",conns[i].fd);			// do this last!
}



// TCP helper,  local printf style function
// Returns TRUE if the write was good
void xprintf(int fd, const char *fmt, ...)
{
	va_list arg;
	char buf[8192];
	int w=0;
	int i=0;

	if (fd<0)
		return;
	va_start(arg, fmt);
	buf[0]=0;
	vsprintf((char*)&buf, fmt, arg);
	if (strlen(buf)>0 && fd>0)
	{
		if (fd>=0)
		{
			w=write(fd,&buf,strlen(buf));
			if (w<0)
			{
				i=conn_find_index(fd);
				if (i>=0)
					conn_disconnect(i,1);
				else
				{
					printf("xprintf() Cant find conn[] with fd=%d\n",fd);
					fflush(stdout);
					shutdown(fd,SHUT_RDWR);
					close(fd); 
					fd=-1;
				}
			}
		}
	}
	va_end(arg);
}



// Setup TCP/IP socket ready for new connections
int init_tcpconn(int port)
{
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
	bzero(&serv_addr,sizeof(struct sockaddr_in));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);

        // If we are in a TCPWAIT for the socket we can bypass the process and use it again
        int y = 1;
        if ( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) == -1 )
        {
                perror("init_tcpconn() - setsockopt SO_REUSEADDR failed");
        }
        if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <0)
        {
                perror("init_tcpconn() - bind failed");
                exit(1);
        }
        listen(listenfd, 1);
        if (fcntl(listenfd,F_SETFL,O_NDELAY) <0)
        {
                perror("init_tcpconn() - Cant set socket O_NDELAY");
                exit(1);
        }
        int flags = fcntl(listenfd,F_GETFL,0);
        if (fcntl(listenfd, F_SETFL, flags | O_NONBLOCK)<0)
        {
                perror("init_tcpconn() - Cant set socket O_NONBLOCK)");
                exit(1);
        }
        if (listenfd>0)
                printf("Listening for telnet on port %d\n",port);
	return(listenfd);
}



void conn_newconnection(int newfd, char *ipaddr)
{
	int i=0;

	conn=-1;
	for (i=0;i<MAX_TCPCONN-1;i++)
	{
		if (conns[i].fd<0)										// free entry in table ?
		{
			conn=i;
			break;
		}
	}
	if (conn==-1)
	{
		monitor_printf(MSK_TCP,"TCP session Error, No free table entries FD:%d IP:%s\n",newfd,ipaddr);
		return;												// discard connection
	}

	conn_set_defaults(conn);
	conns[conn].fd = newfd;
	strcpy(conns[conn].ipaddr,ipaddr);
	//printf("CONNECT  idx=%d  fd=%d ip=%s\n",conn, conns[conn].fd, conns[conn].ipaddr); fflush(stdout);
	monitor_printf(MSK_TCP,"TCP session started FD:%d IP:%s\n",newfd,ipaddr);

	int flags=fcntl(conns[conn].fd, F_GETFL);
	flags=flags|O_NONBLOCK;
	fcntl(conns[i].fd, F_SETFL, flags);


	// optionally request telnet to send each character as the user types it
	if (conns[conn].mode==CONN_MODE_CHARBYCHAR)
	{
		if (write(conns[conn].fd,"\377\375\042\377\373\001",6)<0)                                       // IAC will echo
			conn_disconnect(conns[conn].fd,2);
	}
        xprintf(conns[conn].fd,"Welcome to jons lighting controller, jlcd Ver:%s\n%s",version,conns[i].prompt);
}




// TODO: Move to jlc_monitor ?

// called by debug, send debug messages about changes in system state to connected hosts who requested them
void conn_monitor(uint32_t e, char *msg)
{
	int i=0;

	for (i=0;i<MAX_TCPCONN-1;i++)										// for every possible active connection
	{
		if (conns[i].fd>0)
		{
			if (conns[i].m&e)									// This bit set ?
			{
				xprintf(conns[i].fd,"*");
				switch (e)
				{
					case MSK_SCR:	xprintf(conns[i].fd,"SCR");	break;
					case MSK_REG:	xprintf(conns[i].fd,"REG");	break;
					case MSK_DEV:	xprintf(conns[i].fd,"DEV");	break;
					case MSK_NDE:	xprintf(conns[i].fd,"NDE");	break;
					case MSK_ERR:	xprintf(conns[i].fd,"ERR");	break;
					case MSK_WWW:	xprintf(conns[i].fd,"WWW");	break;
					case MSK_JCP:	xprintf(conns[i].fd,"JCP");	break;
					case MSK_GRP:	xprintf(conns[i].fd,"GRP");	break;
					case MSK_INF:	xprintf(conns[i].fd,"INF");	break;
					case MSK_TCP:	xprintf(conns[i].fd,"TCP");	break;
					case MSK_SND:	xprintf(conns[i].fd,"SND");	break;
				}
				xprintf(conns[i].fd," %s %s",datetimes(5),msg);					// then they get this type of message
			}
		}
	}
}




void conn_text_poll(int i)
{
	int n=-1;
	char st[2];

	if (conns[i].mode == CONN_MODE_BIN || conns[i].mode == CONN_MODE_BIN_INITIAL)
		return;

	if (conns[i].fd>0)											// active file descriptor ?
		n=read(conns[i].fd,&st,1);									// read data from the socket but one byte at a time
	st[1]=0;
	if (n==1)												// got a character
	{
		if (conns[i].mode==CONN_MODE_CHARBYCHAR)
			xprintf(conns[i].fd,"%c",st[0]);
		if (st[0]!=13)											// Not end of string then add character to string
		{
			// for some reason using CTRL-C on linux telnet client causes the client to ignore
			// all subsequent data sent on the socket.  Detect the condition and just close the socket
			if (st[0]==-12)
				conn_disconnect(conns[i].fd,3);
				
			if (st[0]!=10 && st[0]!=13)
				strcat(conns[i].cmd,st);
		}
		else
		{
			interpreter(conns[i].fd,(char*)&conns[i].cmd, sizeof(conns[i].cmd)-1,FALSE );
			bzero(&conns[i].cmd,sizeof(conns[i].cmd));
			xprintf(conns[i].fd,"%s",conns[i].prompt);
		}
	}
	else
	{
		if (n!=-1)
			conn_disconnect(i,4);
	}
}



// Read commands from connected TCP sessions
void conn_poll()
{
	int i=0;
	int newfd=-1;
 	struct sockaddr_in	their_addr;				// should this be inside conns[i] structure ?

 	socklen_t sin_size=sizeof(struct sockaddr_in);

	// Try to take a new TCP connection
	if (conn<MAX_TCPCONN-1)
	{
		newfd = accept(listenfd, (struct sockaddr*)&their_addr, &sin_size);				// Offer to take a connection
		if (newfd>0)											// got one ?
			conn_newconnection(newfd, inet_ntoa(their_addr.sin_addr));
	}


	for (i=0;i<MAX_TCPCONN-1;i++)										// for every possible active connection
	{
		switch (conns[i].mode)
		{
			case CONN_MODE_BIN_INITIAL:	
				bin_send_state(i);
			break;
			case CONN_MODE_BIN:
				bin_msg_poll(i, binpayload);
			break;
			case CONN_MODE_LINEBYLINE:
				conn_text_poll(i);
			break;
			case CONN_MODE_CHARBYCHAR:
				conn_text_poll(i);
			break;
		}
	}

}



// A peer just entered bin mode, the current server state needs to be pushed to it.
// see bin_sync_state()
void conn_set_binmode(int fd)
{
	int i=0;

	i=conn_find_index(fd);
	if (i>=0)
	{
		xprintf(fd,"Entering binary mode.\n");
		conns[i].mode = CONN_MODE_BIN_INITIAL;

		//Set socket back to blocking
        	//int flags = fcntl(fd,F_GETFL,0);
		//flags ^= O_NONBLOCK;					// toggle off
        	//if (fcntl(listenfd, F_SETFL, flags | O_NONBLOCK)<0)
		//{
		//}
	}
	else	xprintf(fd,"conn_set_binmode() Setting bin mode failed\n");
}

