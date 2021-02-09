/*
	jlc_binmsg.c
	Binary message via TCP to a connected peer
	Ver 42
	Last changed:
	13 Dec 2020
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlp.h"
#include "jlc_group.h"

extern struct    universe* uni[MAX_UNIVERSE];
extern uint16_t  session_id;

// Two tables of device and device states
extern struct jlc_devf		devf[MAX_DEVF];						// Table of lighting devices
extern struct jlc_devn		devn[MAX_DEVN];						// Table of sensor devices
extern struct tcp_connection	conns[MAX_TCPCONN];


extern int 	movesens_ignore_retrig_periodms;
extern char     statusline[STLINE_LEN];

extern struct jlc_group_cfg     grpc[MAX_GROUPS+1];
extern struct jlc_group_val     grpv[MAX_GROUPS+1];

// Colour presets
extern struct jlc_colourpreset     clrp[MAX_COLOUR_PRESETS+1];





// byteswritten is cumulative total.
// This emulates a blocking write, sort off.
#define COUNTWRITE_TIMEOUTMS 5000
int countwrite(int fd, int byteswritten, char *p, int len)
{
	int w=0;
	int o=0;
	int bytestowrite=len;
	int chunksize=len;
	int stucktimems=0;

	//printf("countwrite() asked to write %d  ",len);
	if (byteswritten<0)
		return(byteswritten);							// last time was an error, preserve the error

	do
	{
		if (chunksize>2048)
			chunksize=2048;							// limit the write size to 1K or less
		w=write(fd, p+o, chunksize);
		if (w==-1 && errno!=EAGAIN)						// Resource temporarily unavailable
		{
			printf("countwrite() fd=%d errno=%d Err %s\n", fd, errno, strerror(errno)); fflush(stdout);
			usleep(1000);							// ensure we waste a little time before a retry
			return(-1);
		}
		if (w==-1 && errno==EAGAIN)
		{
			usleep(1000);
			stucktimems = stucktimems + 1;					// 1ms more stuck on the write
			if (stucktimems>COUNTWRITE_TIMEOUTMS)
			{
				printf("countwrite() timeout after %dms, closed socket fd=%d\n",COUNTWRITE_TIMEOUTMS, fd);
				fflush(stdout);
				close(fd);
				stucktimems=0;
				return(-1);
			}	
		}

		if (w>=1)								// wrote something ?
		{
			//n=n+w;
			//printf("(%d) ",w);
			o=o+w;								// move write pointer
			byteswritten = byteswritten + w;				// keep count of bytes out
			bytestowrite = bytestowrite - w;				// keep count of bytes left to write
			if (bytestowrite < chunksize)
				chunksize = bytestowrite;				// writing last bit now..
			stucktimems=0;							// wrote some bytes so not stuck
		}
	} while (bytestowrite>0);
	//printf("wrote %d\n",n); fflush(stdout);
	return(byteswritten);
}







// Send a binary message to TCP connected peer with file descriptor fd
// Returns negative error value if message send fails, number of bytes sent if ok
// Returns number of bytes sent, any negative value is an error
// system calls this as it updates state, keeps the front ends in sync
int bin_sendmsg(int fd, int msg_type, int idx)
{
	struct bin_header	hdr;
	int n=0;
	char buf[512];

	//printf("bin_sendmsg()\n"); fflush(stdout);
	memcpy(hdr.binmagic,"JLCB",4);
	hdr.msg_type = msg_type;
	bzero(hdr.spare,6);
	hdr.idx = idx;
	switch (msg_type)
	{
		case BIN_MSG_GROUP_CLEAR:
			hdr.msglen = 1;
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));		// send the header
			n = countwrite(fd, n, (char*)&buf[0], hdr.msglen);			// 1 byte payload just to keep sync
		break;

		case BIN_MSG_DEVN_CLEAR:
			hdr.msglen = 1;
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
			n = countwrite(fd, n, (char*)&buf[0], hdr.msglen);
		break;

		case BIN_MSG_DEVF_CLEAR:
			hdr.msglen = 1;
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
			n = countwrite(fd, n, (char*)&buf[0], hdr.msglen);
		break;

		case BIN_MSG_GROUP_CFG:
			hdr.msglen = sizeof(struct jlc_group_cfg);				// this many bytes follow the header
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));		// send the header
			n = countwrite(fd, n, (char*)&grpc[idx], hdr.msglen);			// possibly send the payload
		break;

		case BIN_MSG_GROUP_VAL:
			hdr.msglen = sizeof(struct jlc_group_val);
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
			n = countwrite(fd, n, (char*)&grpv[idx], hdr.msglen);
		break;

		case BIN_MSG_DEVF:
			hdr.msglen = sizeof(struct jlc_devf);
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
			n = countwrite(fd, n, (char*)&devf[idx], hdr.msglen);
		break;

		case BIN_MSG_DEVN:
			hdr.msglen = sizeof(struct jlc_devn);
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
			n = countwrite(fd, n, (char*)&devn[idx], hdr.msglen);
		break;

//JA new
		case BIN_MSG_CLRP:
			hdr.msglen = sizeof(struct jlc_colourpreset);
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
			n = countwrite(fd, n, (char*)&clrp[idx], hdr.msglen);
			//printf("sent colour preset\n"); fflush(stdout);
		break;

		case BIN_MSG_DATETIME:							// Sent once a second, help us detect socket drops
			bzero(&buf,sizeof(buf));
			sprintf(buf,"%s",datetimes(30));				// Send UTC ASCII string with added 0 terminator
			hdr.msglen = strlen(buf)+1;
			n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
			n = countwrite(fd, n, (char*)&buf, hdr.msglen);
		break;

		default:
			return(-1);
	}
	return(n);
}




int bin_sendmsg_generic(int fd, int msg_type, int idx, char* payload, int payload_len)
{
	struct bin_header	hdr;
	int n=0;

	memcpy(hdr.binmagic,"JLCB",4);
	hdr.msg_type = msg_type;
	bzero(hdr.spare,6);
	hdr.idx = idx;
	hdr.msglen = payload_len;	
	n = countwrite(fd, n, (char*)&hdr, sizeof(struct bin_header));
	n = countwrite(fd, n, payload, hdr.msglen);
	return(n);
}


// Send generic message to all peers currently receiving binary protocol
void bin_sendmsg_generics(int msg_type, int idx, char* payload, int payload_len)
{
	int i=0;

	for (i=0;i<MAX_TCPCONN-1;i++)							// for every possible active connection
	{
		if (conns[i].mode == CONN_MODE_BIN)
		{
			bin_sendmsg_generic(conns[i].fd, msg_type, idx, payload, payload_len);
		}
	}
}




// Looks through conns[] for peers in binary mode, send them an update
void bin_sendmsgs(int msg_type, int idx)
{
	int i=0;

	//printf("bin_sendmsgs()   msg_type=%d    idx=%d\n", msg_type, idx); fflush(stdout);
	for (i=0;i<MAX_TCPCONN-1;i++)							// for every possible active connection
	{
		if (conns[i].mode == CONN_MODE_BIN)
		{
			bin_sendmsg(conns[i].fd, msg_type, idx);
		}
	}
}





// Send the state of server to peers.
// The peer should have blank tables setup for devf[], devn[] grp[] and clrp[] 
// we send only the elements that are populated
void bin_send_state(int i)
{
	int s=0;
	//printf("bin_send_state() conns[i].t=%d  conns[i].idx=%d\n",conns[i].t, conns[i].idx);

	if (conns[i].mode == CONN_MODE_BIN_INITIAL)					// connected peer needs server state?
	{
		switch (conns[i].t)							// which table are currently sending ?
		{
			case BIN_MSG_GROUP_CFG:
				if (grpc[conns[i].idx].noc > 0)				// Not a blank entry ?
				{
					s=bin_sendmsg(conns[i].fd, BIN_MSG_GROUP_CFG, conns[i].idx);
					//printf("bin_sendmsg BIN_MSG_GROUP_CFG, conns[i].idx=%d\n", conns[i].idx); fflush(stdout);
					if (s<0)					// TCP send failed
					{
printf("s=%d\n",s); fflush(stdout);
						conn_disconnect(i,10);			// close socket, set conns[i] back to defaults
						return;
					}
				}
				conns[i].idx++;
				if (conns[i].idx >= MAX_GROUPS)				// finished this table ?
				{							// move on to next
					conns[i].t = BIN_MSG_GROUP_VAL;
					conns[i].idx = 0;
					return;
				}
			break;

			case BIN_MSG_GROUP_VAL:
				if (grpc[conns[i].idx].noc > 0)				// Not a blank entry ?
				{
					s=bin_sendmsg(conns[i].fd, BIN_MSG_GROUP_VAL, conns[i].idx);
					if (s<0)
					{
						conn_disconnect(i,11);
						return;
					}
				}
				conns[i].idx++;
				if (conns[i].idx >= MAX_GROUPS)
				{
					conns[i].t = BIN_MSG_DEVF;
					conns[i].idx = 0;
					return;
				}
			break;

			case BIN_MSG_DEVF:
				if (devf[conns[i].idx].dev_type != DEV_NONE)
				{
					s=bin_sendmsg(conns[i].fd, BIN_MSG_DEVF, conns[i].idx);
					if (s<0)
					{
						conn_disconnect(i,12);
						return;
					}
				}
				conns[i].idx++;
				if (conns[i].idx >= MAX_DEVF)
				{
					conns[i].t = BIN_MSG_DEVN;
					conns[i].idx = 0;
					return;
				}
			break;

			case BIN_MSG_DEVN:
				if (devn[conns[i].idx].dev_type != DEV_NONE)
				{
					s=bin_sendmsg(conns[i].fd, BIN_MSG_DEVN, conns[i].idx);
					if (s<0)
					{
						conn_disconnect(i,13);
						return;
					}
				}
				conns[i].idx++;
				if (conns[i].idx >= MAX_DEVN)
				{
					conns[i].t = BIN_MSG_CLRP;
					conns[i].idx = 0;
					return;
				}
			break;

//JA New
			case BIN_MSG_CLRP:
				if (clrp[conns[i].idx].active == TRUE)
				{
					s=bin_sendmsg(conns[i].fd, BIN_MSG_CLRP, conns[i].idx);
					if (s<0)
					{
						conn_disconnect(i,14);
						return;
					}
				}
				conns[i].idx++;
				if (conns[i].idx > MAX_COLOUR_PRESETS)
				{
					conns[i].t = 0;
					conns[i].idx = 0;
					conns[i].mode = CONN_MODE_BIN;			// Last entry is sent
				}
			break;
		}
		//if (s>0)
			//printf("wrote %d bytes\n",s);
	}
}



// Called once per second from jlc_timer.c
void bin_onehz_tick()
{
	int i=0;
	int s=0;

	for (i=0;i<MAX_TCPCONN-1;i++)							// for every possible active connection
	{
		if (conns[i].mode == CONN_MODE_BIN)
		{
			s=bin_sendmsg(conns[i].fd, BIN_MSG_DATETIME, 0);
			if (s<0)							// TCP send failed
			{
				conn_disconnect(i,15);					// close socket, set conns[i] back to defaults
				return;
			}
		}
	}

}



void print_header(struct bin_header* hdr)
{
	printf("binmagic\t[%c%c%c%c]\n", hdr->binmagic[0],  hdr->binmagic[1],  hdr->binmagic[2],  hdr->binmagic[3]);
	printf("msg_type=%d\n",hdr->msg_type);
	printf("idx=%d\n",hdr->idx);
	printf("msglen=%d (%d)\n",hdr->msglen, hdr->msglen+(int)sizeof(struct bin_header));
	fflush(stdout);
}





// Either returns 0 or -1 for cant read or returns having read the expected len
int readchunk(int i, char* buf, int len)
{
        int n=0;
        int x=0;

        n = recv(conns[i].fd, buf, len, MSG_DONTWAIT | MSG_PEEK);                               // Got enough data waiting yet ?
	if (n==len)
        {
                do
                {
                        n=read(conns[i].fd, buf+x, 1);
                        if (n==1)
                                x++;
                } while (x<len);
                return(x);
        }
        return(-1);
}




// Poll for a binary message
// We need two passes to read a full message, one for the header, one for the
// payload.  This is because the payload length is part of the header.
// Each call to bin_rxmsg either reads nothing, a header, or a payload
// return is 0 for no message, 1 for header, 2 for payload, negative for error
int bin_msg_poll(int i, char* payload)
{
	int n=0;
	struct bin_header	hdr;

	if (conns[i].gotheader==TRUE)								// Already read a valid header
	{
		n=readchunk(i, payload, conns[i].hdr.msglen);					// read the payload
		if ( (n==-1) || (n==0) )							// nothing read?
			return(0);								// not enough data yet
		if (n==conns[i].hdr.msglen)
		{
			conns[i].gotheader=FALSE;						// Start looking for next header
			return(2);								// complete, got header and payload now
		}
		if (n>0)
		{
			printf("!bin_msg_poll() Expected %d got %d  ,i=%d\n",conns[i].hdr.msglen,n,i);
			fflush(stdout);
		}
		if (n<0)									// real error
		{
			conn_disconnect(i,16);
			return(-1);
		}
	}

	conns[i].gotheader=FALSE;
	n=readchunk(i, (char*)&hdr, sizeof(struct bin_header));					// read the header
	if ( (n==-1) || (n==0) )								// nothing read?
		return(0);									// not enough data yet
	if (n<0)										// got a real error ?
	{
		conn_disconnect(i,17);
		return(-1);
	}

	// We read the header ok, check it
	if (strncmp(&hdr.binmagic[0],"JLCB",4)!=0)						// Bad magic ?
	{
		monitor_printf(MSK_ERR,"bin_rxmsg() got %d bytes but header magic was bad\n",n);
		dumphex(&hdr.binmagic[0], 4);
		return(-2);	
	}

	memcpy(&conns[i].hdr, &hdr, sizeof(struct bin_header));					// Got a valid header, take a copy
	conns[i].gotheader=TRUE;
	return(1);										// got header so far 
}


