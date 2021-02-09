/*
	Jons Lighting Controller

	A server for jlp (jons lighting protocol) devices.
*/

#define VERSION		"0.114"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlp.h"
#include "jlc_group.h"


// Globals
int	  desired_loophz = 800;
char 	  version[16];
struct    universe*	 uni[MAX_UNIVERSE+1];							// Array of pointers, universes are 1 to N so +1
int	  c1		 = 1;
struct	  statss 	 stats;
int	  blankcount_top = 100;
char 	  c2		 = 2;
int       server_quit	 = FALSE;
uint16_t  session_id	 = 0;
char      script_path[MAX_PATHLEN];
char	  log_path[MAX_PATHLEN];
char	  mask_path[MAX_PATHLEN];
int	  flag_tracecode = FALSE;
int	  jlp_udp_port 	 = JLP_UDP_PORT;
int	  num_devn	 = 0;
int	  num_devf	 = 0;
int	  loopcount	 = 100;
int	  loophz	 = 0;
int	  dly		 = 1000;
char	  statusline[STLINE_LEN];
uint16_t  sl_counter	 = 0;
unsigned int starttime 	 = 0;

// Logging
int	  log_sensors	 = TRUE;								// Log movement sensor events
int	  log_sensors_e	 = FALSE;								// also write log line to stdout
int	  log_temps	 = TRUE;								// temperatures
int	  log_temps_e	 = FALSE;


// TCP connection
int     interpreter_tcp_port=1111;
char	cprompt[MAX_PROMPTLEN];									// for console
int     cclr=TRUE;
uint32_t console_m=0;


char	c3=3;
int	conn=-1;										// index for conns
struct	tcp_connection		conns[MAX_TCPCONN];



// Two tables of device and device states
struct jlc_devf		devf[MAX_DEVF];								// Table of lighting devices
struct jlc_devn		devn[MAX_DEVN];								// Table of sensor devices
char c4=4;

// Groups
struct jlc_group_cfg	grpc[MAX_GROUPS+1];							// group configuration
char c5=5;
struct jlc_group_val	grpv[MAX_GROUPS+1];							// group value
char c6=6;
struct efx		grpefx[MAX_GROUPS+1];							// group based effects

// Colour presets
struct jlc_colourpreset	clrp[MAX_COLOUR_PRESETS+1];						// colour presets 1 to N

// Table for timed events
struct timed_events	te[MAX_TIMED_EVENTS];	



// Audioplus data
struct 	audioplus        	audioplus_records[FBSMAX];
char	audioplus_md5[64];
char	audioplus_ipaddr[32];									// Audio data is from this source address
int	audioplus_sockfd	= -1;
int	audioplus_frame		= 0;								// current audioplus frame number
int	audioplus_framedelay 	= 10;								// Process data that is N frames behind current


// Audio sequencer (sound to light)
struct sequencescript		seqs[MAX_SEQ_SCRIPTS];
int	seqrun			= -1;								// -1 = off, 0 or more is index to seqs




// Visuals
int	vis_vu_max		= 128;								// the number of "lights" a VU meter should be
int	vis_peakholdtimel	= PEAKHOLDTIME;
int	vis_vu_left		= 0;								// Volume meter, left channel
int	vis_vu_right		= 0;
int	vis_vu_leftpeak		= 0;
int	vis_vu_rightpeak	= 0;
int	vis_flag_agc		= TRUE;
int	vis_flag_peakhold	= FALSE;
int     vis_flag_ansivu		= FALSE;


// Binary TCP protocol
char* binpayload = NULL;



int movesens_ignore_retrig_periodms = 1000 * 60 * 5;						// Ignore re-trigger events for movement for Nms

void canary_check()
{
	if (c1!=1)
	{
		printf("C1 corrupt %d\n",c1);
		fflush(stdout);
		exit(1);
	}
	if (c2!=2)
	{
		printf("C2 corrupt %d\n",c2);
		fflush(stdout);
		exit(1);
	}
	if (c3!=3)
	{
		printf("C3 corrupt %d\n",c3);
		fflush(stdout);
		exit(1);
	}
	if (c4!=4)
	{
		printf("C4 corrupt %d\n",c3);
		fflush(stdout);
		exit(1);
	}
	if (c5!=5)
	{
		printf("C5 corrupt %d\n",c3);
		fflush(stdout);
		exit(1);
	}
	if (c6!=6)
	{
		printf("C6 corrupt %d\n",c3);
		fflush(stdout);
		exit(1);
	}
}


void sig_handler(int signo)
{
	//printf("sighandler()  Got signo %d\n",signo);
	//fflush(stdout);
}



void init_paths()
{
	sprintf(script_path,"scripts");
	sprintf(log_path,"./log");
	sprintf(mask_path,"./masks");
}



void init_stats()
{
	stats.pps_jlp=0;
	stats.bps_jlp=0;
	stats.pps_jcp=0;
	stats.bps_jcp=0;
	stats.pps_audioplus=0;
	stats.ppps_audioplus=0;
}


int main(int argc, char **argv)
{
	int jcp_server_fd=-1;

	set_realtime();
	bzero(&statusline,sizeof(statusline));
	sprintf(version,"%s",VERSION);
	printf("jlcd Ver:%s\n",version);
	fflush(stdout);
	srand (time (0));
	session_id=rand();	
	printf("session_id=%d\n",session_id);


	// Initialise
	sprintf(cprompt,"> ");
	init_stats();
	init_paths();
	init_universe();
	init_conn();										// init TCP connections table
	init_group();
	init_effects();
	init_devf();
	init_devn();
	init_te();
	init_colour_presets();

	colour_presets_save(fileno(stdout));

	jcp_server_fd=jcp_init(JCP_SERVER_PORT,TRUE);						// Setup ready to poll for control protocol messages
	group_reload(STDOUT_FILENO);								// load all groups into ram	
	init_http(8888);

	// Binary protocol buffer
	binpayload=malloc(65538);								// uint16_t max plus a couple


	// Setup to receive audioplus data
	bzero(&audioplus_md5,sizeof(audioplus_md5));
	audioplus_clear_buffers();
	audioplus_sockfd=setup_audioplus_receiving_socket(AUDIOPLUS_PORT);
	if (audioplus_sockfd>0)
		printf("Listening for audioplus data on UDP port %d\n",AUDIOPLUS_PORT);
	else	printf("Failed to open UDP port %d for audioplus receive\n",AUDIOPLUS_PORT);
	fflush(stdout);

	colour_presets_load(fileno(stdout));

	// User startup commands
	run_script(STDOUT_FILENO, "autoexec.lsq");

	init_tcpconn(interpreter_tcp_port);							// init TCP connections
	if (cmd_colour(STDOUT_FILENO)!=TRUE)
		cmd_colour(STDOUT_FILENO);							// toggle until true
	init_console();
	signal(SIGPIPE, sig_handler);


	// Main loop
	starttime = current_timems();
	while (server_quit!=TRUE)
	{
		timer_fast();
		console_poll();
		conn_poll();									// poll for TCP session input
		jcp_message_poll(jcp_server_fd);						// Discovery and control and device state change msgs
		effects_tick();
		jlp_light_sender();
		http_poll();
		audioplus_poll();
		if (seqrun>=0)
			audioplus_fake(seqrun);							// pretend we have audio for this script


		// Always last
		usleep(dly);
		loopcount++;
		canary_check();
	}
	return(0);										// avoid pointless warning on older GCC
}

