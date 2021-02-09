/*
	jlc_doorbell.c

*/
#define PROGNAME	"jlc_doorbell"
#define VERS		"0.12"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
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

#include "jcp_protocol_devs.h"
#include "jcp_client.h"
#include "jlc.h"


#define DOORBELL_SAMPLE		"doorbell4"					// default sample for doorbell push
#ifdef RPI
	#include "raspberry_pi_io.c"
	#define LED_HB		23                                              // Heart beat LED
	#define DOORBELL	5                                               // Doorbell input, active low
	#define AMP_ENABLE	6						// Active high, enable audio amplifier, marked "SW" on PAM8403
	#define HB_TOP		400
#endif


// Prototypes
int wav_play(const char *fn);
char console_poll();
int snd_init(void);
int snd_end(void);

int lockout=FALSE;

#include <sched.h>
void set_realtime(void)
{
        struct sched_param sparam;
        sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(0, SCHED_FIFO, &sparam);
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


// Return TRUE if all the bytes are zero
int allzero(unsigned char *p, int len)
{
	int allzero=TRUE;
	int i=0;

	for (i=0;i<len;i++)
	{
		if (p[i]!=0)
			allzero=FALSE;
	}
	return(allzero);
}


#include <alsa/asoundlib.h>
void set_volume(long volume)
{
        long min, max, newvol;
        snd_mixer_t *handle = NULL;
        snd_mixer_selem_id_t *sid = NULL;
        const char *card = "default";
        //const char *selem_name = "Master";
        const char *selem_name = "PCM";
        float vpercent, fnewvol;

        if (snd_mixer_open(&handle, 0) < 0)
        {
                printf("%s: set_volume() RPI - snd_mixer_open failed\n",PROGNAME);
                fflush(stdout);
                perror("snd_mixer_open");
        }
        else
        {
                snd_mixer_attach(handle, card);
                snd_mixer_selem_register(handle, NULL, NULL);
                snd_mixer_load(handle);

                //snd_mixer_selem_id_alloca(&sid);
                snd_mixer_selem_id_malloc(&sid);
                snd_mixer_selem_id_set_index(sid, 0);
                snd_mixer_selem_id_set_name(sid, selem_name);
                snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);
                if (elem == NULL)
                {
                        printf("%s: set_volume() RPI - snd_mixer_find_selem failed()\n",PROGNAME);
                        fflush(stdout);
                }
                else
                {
                        snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

                        vpercent = (float)volume/100;
                        fnewvol = (float)min + vpercent * ( (float)max - (float)min);
                        newvol = (long)fnewvol;

                        //printf("volume=%ld  min=%ld   max=%ld   newvol=%ld\n",volume,min,max,newvol); fflush(stdout);
                        if ( snd_mixer_selem_set_playback_volume_all(elem,newvol) <0)
                        {
                                printf("%s: set_volume() RPI - snd_mixer_selem_set_playback_volume_all() failed\n",PROGNAME);
                                fflush(stdout);
                        }
                }
                snd_mixer_close(handle);
        }
}





void amp(int s)
{
	if (s==1)
	{
#ifdef RPI
		output_high(AMP_ENABLE);				// Audio AMP on
		usleep(1000*150);
#endif
		printf("%s:\tAmp On\n",PROGNAME);
		fflush(stdout);
	}
	else	
	{
#ifdef RPI
		output_low(AMP_ENABLE);
#endif
		printf("%s:\tAmp Off\n",PROGNAME);
	}
}


		
// Set volume (percentage left, right) and play sample
void playsample(int voll, int volr, char *name)
{
	char filename[2048];
	char lcname[1024];
	int i=0;

	bzero(&lcname,sizeof(lcname));
	if (strlen(name)<=0)
		 return;
	for (i=0;i<strlen(name);i++)
		lcname[i]=tolower(name[i]);
	sprintf(filename,"samples/%s.wav",lcname);
	fflush(stdout);
	amp(1);
#ifdef RPI
	printf("%s:\tSet volume %d\n",PROGNAME,voll); 
	fflush(stdout);
	set_volume(voll);
#endif
	snd_init();
	printf("%s:\tPlaying [%s]\n",PROGNAME,filename); 
	fflush(stdout);
	wav_play(filename);
	printf("%s:\tsnd_end()\n",PROGNAME);
	fflush(stdout);
	snd_end();
	amp(0);
}





// Server is pushing state at us. We can manually play samples when asked
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
	if (idx==-100)								// registered with server now
		return;
	printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);

	// Server asked us to play a sound sample
	if (ds->asciiorbinary == 1 && ds->valuebyteslen >=1)
	{
		//printf("valuebytes=[%s]  len=%d\n",ds->valuebytes, ds-> valuebyteslen);
		if (ds->valuebytes[ds->valuebyteslen-1]==' ')
			ds->valuebytes[ds->valuebyteslen-1]=0;			// remove trailing space
		playsample(ds->value1, ds->value2, ds->valuebytes);
	}
	fflush(stdout);
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
	char samplename[512];
	int v=0;
	int g=0;

	bzero(&samplename,sizeof(samplename));
	tdata[tlen]=0;
	printf("jcp_topic_cb() idx=%d  uid=%s  topic=%d  tlen=%d, tdata=[%s]\n",
		idx,printuid(uid),topic,tlen,tdata);


	if (topic==JCP_TOPIC_SND)						// sound playback topic message ?
	{
		sscanf((char*)tdata, "%d %d %64c",&g, &v, samplename);
		printf("group=%d\n",g);
		printf("volume=%d\n",v);
		printf("samplename=%s\n",samplename);
		if (g==0)
		{
			if (lockout!=TRUE)					// dont play doorbell sound this time..
				playsample(v, v, samplename);
			else	
			{
				printf("lockout was TRUE, skipping this request to play\n");
				lockout=FALSE;
			}
		}
		fflush(stdout);
	}
}



// Server sends us config info, we can ignore this on linux
void jcp_dev_config(char *dt, int len)
{
}



unsigned long long current_timems()
{
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        return milliseconds;
}





// For a given interface return the MAC address
int getmac(char *iface, char *mac)
{
	struct ifreq s;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	int i;

	printf("%s: Get MAC for interface [%s]\n",PROGNAME,iface); fflush(stdout);
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
                //printf("Got %d bytes from %s\n",numbytes,ipaddr); fflush(stdout);
                if (numbytes>=sizeof(struct jcp_header))
                {
                        jcp_parse_and_reply((char*)&rbuffer, numbytes, (char*)&ipaddr);
                }
        }
}



#ifdef RPI
void do_rpi()
{
	printf("RPI specific initialise do_rpi()\n");	fflush(stdout);
	setup_io();							// Setup ARM SOC memory mapped GPIO controller
	set_as_output(LED_HB);
	set_as_output(AMP_ENABLE);					// high=Amp on
	output_low(AMP_ENABLE);
}
#endif


#ifdef RPI
// Poll the doorbell button, returns 1 if button is pushed
int doorbell_poll()
{
	static int led_hbc=0;
	int db=0;

//printf("P"); fflush(stdout);
	led_hbc++;
	db=input(DOORBELL);								// Read GPIO
	if (db==0)									// button push ?
		return(1);

        if (led_hbc>HB_TOP)
                led_hbc=0;
        if (led_hbc>(HB_TOP/2))
                output_low(LED_HB);
        else    output_high(LED_HB);
	if (led_hbc>HB_TOP)
		led_hbc=0;
	return(0);
}

#else
// Fake function for testing on generic linux
int doorbell_poll()
{
	return(0);
}
#endif




int main(int argc, char **argv)
{
	uint16_t sid;
	unsigned char mymac[7];
	time_t t;
	int p=JLP_SERVER_PORT+1;
	int quit=FALSE;
	int sockfd=-1;
	int c=0;
	unsigned int ct=0;								// current time
	unsigned int pt=0;								// previous time
	char vers[64];

	int  pushed=0;									// 1 as doorbell is closed
	int  ppushed=-1;								// previous state

	srand((unsigned) time(&t));
	sid=rand();

	set_realtime();
#ifdef RPI
	do_rpi();
#endif

	//if (snd_init() < 0)
	//{
		//printf("snd_init() failed\n");
		//exit(1);
	//}

#ifdef RPI
	getmac("wlan0", (char*)&mymac);
	set_volume(100);
#else
	getmac("eno1", (char*)&mymac);
#endif
	printf("%s:\tMAC %s\n",PROGNAME,printuid((unsigned char*)&mymac));


        // Find a port to listen on, start with the server listing port+1 and work up. Do not specify
        // SO_REUSEPORT or SO_REUSEADDR as we want to be the only listener on the port
	p=JCP_SERVER_PORT+10;
	c=0;
	do
	{
		sockfd=jcp_udp_listen(p);						// Try and open a port for listening
		if (sockfd<0)								// open failed ?
			p++;								// try next port
		c++;
		if (c>40)
		{
			printf("Tried 30 UDP ports, giving up\n");
			exit(1);
		}
	} while (sockfd<0);
	printf("Listening JCP messages on UDP port %d\n",p);
	fflush(stdout);

	sprintf(vers,"%s %s",PROGNAME,VERS);
	jcp_init_b((char*)&mymac, sid, p, JLP_SERVER_PORT, (1000/JCP_TIMER_FREQ_HZ), "LXDB", (char*)&vers ); 
	jcp_register_dev(0, DEVN_DOORBELL   ,0, "0000000000000000"); 			// we play a noise and send a state when button pushed
	jcp_register_dev(1, DEVN_PLAYSOUNDS ,0, "0001000000000000");			// we can play sound samples on request 

	if ( munlockall() == -1 )							// never swap process out
		perror("munlockall:1");

	while (quit!=TRUE)
	{
		jcp_message_poll(sockfd);						// call into jcp_client
		ct=current_timems();
		//printf("%u\n",(ct-pt)); fflush(stdout);
		if ( (ct-pt) > (1000/JCP_TIMER_FREQ_HZ) )				// call into jcl_client on timer
		{
			jcp_timertick_handler();
			pt=ct;
		}

		pushed=doorbell_poll();							// 1=button held in
		if (pushed!=ppushed)
		{
			//jcp_send_device_state(int idx, uint16_t value1, uint16_t value2, int asciiorbinary, char* valuebytes, int valuebyteslen)
			jcp_send_device_state(0, pushed, 0, 0, "", 0);			// send doorbell 
			if (pushed==1)
			{
				lockout=TRUE;						// ensure we dont play it twice
				playsample(100,100,DOORBELL_SAMPLE);
			}
			ppushed=pushed;
		}	
		usleep(2000);
	}
	exit(0);
}
