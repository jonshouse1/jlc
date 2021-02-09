/*
	jlp.c

	Jons lighting protocol / packet
	Send Lighting data packets to hosts
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlp.h"

extern struct    universe* uni[MAX_UNIVERSE+1];
extern uint16_t  session_id;
extern struct	 statss	 stats;
extern char      maskpath[MAX_PATHLEN];
extern int       jlp_udp_port;
extern int	 num_devf;
extern int	 num_devn;

// Two tables of device and device states
extern struct jlc_devf          devf[MAX_DEVF];                                         // Table of lighting devices
extern struct jlc_devn          devn[MAX_DEVN];                                         // Table of sensor devices




// Build and send a packet of lighting to a device (DEV_F)
// Copy number of channels (noc) worth of data from universe 'univ' first channel 'fchan' into correctly formatted
// packet and send it

// TODO:  Send large univ/chan combinations as fragmented messages.


void jlp_build_and_send_lighting_packet(unsigned char*uid, char *ipaddr, int jlp_port, int udr_hz, int noc, int univ, int fchan)
{
	static unsigned char	buf[sizeof(struct jlp_pkt)+MAX_CHANNELS+1];		// buffer for lighting data and packet
	struct jlp_pkt		*jlp=(struct jlp_pkt*)&buf;
	unsigned int ps=0;								// packet size
	int i=0;
	int p=0;									// UDP port;

	memcpy(&jlp->magic,JLP_MAGIC,sizeof(JLP_MAGIC)-1);
	memcpy(&jlp->uid,uid,UID_LEN);
	jlp->msg_type		= 2;
	jlp->update_rate_hz	= udr_hz;						// update rate server is going for
	jlp->host_time_ms	= current_timems();					// send time
	jlp->map_universe	= univ;							// we send univ/chan mapping, just for info
	jlp->map_fchan		= fchan;
	jlp->voffset		= 0;
	jlp->nov		= noc;							// number of values in message

	for (i=0;i<noc;i++)
	{
		if (fchan+i < uni[univ]->noc)						// Clip if we hit the edge of universe
		{
			jlp->values[i] = uni[univ]->ch[fchan+i].value;
		}
	}

	p=jlp_port;
	if (p<1024)
	{
		printf("jlp_build_and_send_lighting_packet() - jlp_udp_port must be >=1024\n"); fflush(stdout);
		exit(1);
	}
	ps=sizeof(struct jlp_pkt)+noc;							// header + bytes for lighting data
//printf("JLP() Sending %s port %d  %d bytes \n",ipaddr,p,ps); fflush(stdout);
	udp_generic_send((char*)&buf, ps, ipaddr, p, FALSE);

	stats.pps_jlp++;								// packets per second counter
	stats.bps_jlp = stats.bps_jlp + ps;						// bytes per second total
}





// Looks at the last send time for each fixture registered with the system, if the universe it maps to is active 
// and the time is ready then it sends data to the device
int jlp_light_sender()
{
	int i=0;
	int udrms=0;									// update rate (ms)

	if (num_devf==0)
		return(TRUE);								// table is empty

	for (i=0;i<MAX_DEVF;i++)
	{
		if (devf[i].dev_type != DEV_NONE)					// table entry contains a device ?
		{
			//printf("i=%d  devf[i].univ=%d   %08X\n",i,devf[i].univ, uni[i]); fflush(stdout);
			if (uni[devf[i].univ]!=NULL)
			{
				//printf("i=%d  devf[i].univ=%d  active=%d ",i,devf[i].univ, uni[devf[i].univ]->active ); fflush(stdout);
				if (uni[devf[i].univ]->active == TRUE)
				{
					if (devf[i].update_rate_hz<=0)
					{
						printf("jlp_light_sender()  devf[%d].update_rate_hz=%d, fixme!\n",i,devf[i].update_rate_hz);
						exit(1);
					}
					udrms=(1000/devf[i].update_rate_hz);
					if ((unsigned int)current_timems()-uni[devf[i].univ]->last_changed > 4000)	// was last changed more than Nms ago?
						udrms = 250;
					if (udrms > 500)
						udrms=500;					// minimum update rate is 500ms

 					if (devf[i].noc!=0 && devf[i].univ !=0 && devf[i].fchan!=0)	// device is mapped ?
					{								// device need data now ?
						if ( ((unsigned int)current_timems()-devf[i].last_send_time_ms) > udrms )
						{
							jlp_build_and_send_lighting_packet((unsigned char*)devf[i].uid, 
								   			devf[i].ipaddr, 
											devf[i].jlp_udp_port,
								   			devf[i].update_rate_hz, 
								   			devf[i].noc, 
								   			devf[i].univ, 
								   			devf[i].fchan);
							devf[i].last_send_time_ms = current_timems();
						}
					}
				}
			}
		}
	}
	return(TRUE);
}


