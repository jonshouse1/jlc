/*
	jlc_linuxgui_bin.c

	works with jlcd 0.110
*/


#define VERSION		"0.89"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>


#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#include "wavplay.h"
#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlp.h"
#include "jlc_group.h"

#define PROGNAME	"jlc_linuxgui_bin"


int quit=FALSE;

struct tcp_connection	conns[MAX_TCPCONN];
struct sockaddr_in      con_binmsg;
struct sockaddr_in      con_monitor;
struct sockaddr_in      con_terminal;
int                     fd_terminal=-1;                                         // tcp session for telnet style terminal
int                     fd_monitor=-1;                                          // tcp session with server device/group messages
int                     fd_binmsg=-1;                         


// Two tables of device and device states
struct jlc_devf		devf[MAX_DEVF];								// Table of lighting devices
struct jlc_devn		devn[MAX_DEVN];								// Table of sensor devices

// Groups
struct jlc_group_cfg	grpc[MAX_GROUPS+1];							// group configuration
struct jlc_group_val	grpv[MAX_GROUPS+1];							// group value

// Colour presets
struct jlc_colourpreset  clrp[MAX_COLOUR_PRESETS+1];

// Table for timed events
struct timed_events	te[MAX_TIMED_EVENTS];	

char			server_datetime[512];
char                    server_ipaddr[32];
char			sipaddr[32];
int                     server_tcpport=1111;
int			online=FALSE;								// connected to server? negative=no
int			app_width=800;
int			app_height=480;
char			windowtitle[512];
int			xserver_width=0;
int			xserver_height=0;
int			withborders=TRUE;

int			feature_fullscreen=FALSE;
int			feature_terminal=TRUE;
int			feature_caption=FALSE;							// Date/time top right
int			feature_manage_screen_blanking=FALSE;
int			feature_unblank_for_sounds=TRUE;					// turn screen blanker off when playing sound
int			feature_show_switches=FALSE;
int			feature_unblank_for_movement_sensor=FALSE;
char*			binpayload=NULL;


pid_t   mypid = 0;
void reap_all()
{
        pid_t wpid;
        int stat;

        while ( (wpid = waitpid(-1, &stat, WNOHANG)) > 0)
        {
                printf("%s: reap_all() - child %d terminated with exit code %d\n",PROGNAME, wpid, stat);
                if (wpid == mypid)
                        printf("%s: Cache child exited\n",PROGNAME);
                fflush(stdout);
        }
}


void dumphex(const void* data, size_t size)
{
        char ascii[17];
        size_t i, j;
        ascii[16] = '\0';
        for (i = 0; i < size; ++i) {
                printf("%02X ", ((unsigned char*)data)[i]);
                if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
                        ascii[i % 16] = ((unsigned char*)data)[i];
                } else {
                        ascii[i % 16] = '.';
                }
                if ((i+1) % 8 == 0 || i+1 == size) {
                        printf(" ");
                        if ((i+1) % 16 == 0) {
                                printf("|  %s \n", ascii);
                        } else if (i+1 == size) {
                                ascii[(i+1) % 16] = '\0';
                                if ((i+1) % 16 <= 8) {
                                        printf(" ");
                                }
                                for (j = (i+1) % 16; j < 16; ++j) {
                                        printf("   ");
                                }
                                printf("|  %s \n", ascii);
                        }
                }
        }
}


unsigned long long current_timems()
{
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        return milliseconds;
}

// Seconds
unsigned long current_times()
{
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        unsigned long long sec = te.tv_sec;
        return sec;
}


char* dev_type_name(int dev_type)
{
	static char name[64];
	char *namep=&name[0];

	name[0]=0;
	switch (dev_type)
	{
		case DEVF_DMX:                  sprintf(name,"DMX");                    break;
		case DEVF_PIXELS:               sprintf(name,"PIXELS");                 break;
		case DEVF_BITMAP:               sprintf(name,"BITMAP");                 break;
		case DEVF_LIGHT:                sprintf(name,"LIGHT");                  break;

		case DEVN_RELAY:                sprintf(name,"RELAY");                  break;
		case DEVN_SWITCH:               sprintf(name,"SWITCH");                 break;
		case DEVN_SWITCHPBT:            sprintf(name,"SWITCHPBT");              break;
		case DEVN_SWITCHPBM:            sprintf(name,"SWITCHPBM");              break;
		case DEVN_DOORBELL:             sprintf(name,"DOORBELL");               break;
		case DEVN_MOVEMENTSENSOR:       sprintf(name,"MOVESENSOR");             break;
		case DEVN_POT:                  sprintf(name,"POT");                    break;
		case DEVN_TEMPSENS:             sprintf(name,"TEMPSENSOR");             break;
		case DEVN_MCURRENTSENS:         sprintf(name,"MCURRENTSENS");           break;
		case DEVN_DCURRENTSENS:         sprintf(name,"DCURRENTSENS");           break;
		case DEVN_PLAYSOUNDM:           sprintf(name,"PLAYSOUNDM");             break;
		case DEVN_PLAYSOUNDS:           sprintf(name,"PLAYSOUNDS");             break;
		case DEVN_CLOCKCAL:             sprintf(name,"CLOCKCAL");               break;
		case DEVN_LEDSIGN:              sprintf(name,"LEDSIGN");                break;
		case DEVN_DUMMY:                sprintf(name,"DUMMY");                  break;
		default:                        sprintf(name,"Unknown!(%d)",dev_type);  break;
	}
	return(namep);
}


void screenblanker_runcmd(char *cmd)
{
        if (feature_manage_screen_blanking!=TRUE)
                return;
	if (strlen(cmd)>1)
	{
		system(cmd);
        	printf("screenblanker_runcmd[%s]\n",cmd);
        	fflush(stdout);
	}
}

// Screen saver and DPMS control (mostly for O2 joggler)
void init_screenblanker()
{
        screenblanker_runcmd("export DISPLAY=localhost:0;xset s reset;xset s blank;xset s 600;xset +dpms");
}

void screenblanker_screenon()
{
        screenblanker_runcmd("export DISPLAY=localhost:0;xset s reset;xset dpms force on");
}

void screenblanker_screenoff()
{
        screenblanker_runcmd("export DISPLAY=localhost:0;xset s activate;xset dpms force off");
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
        pid_t pid= fork();
        if (pid==0)
        {
                //set_volume(voll);
                snd_init();
                printf("%s:\tPlaying [%s]\n",PROGNAME,filename);
                fflush(stdout);
                wav_play(filename);
                printf("%s:\tsnd_end()\n",PROGNAME);
                fflush(stdout);
                snd_end();
                exit(0);
        }
	if (feature_unblank_for_sounds==TRUE)
                screenblanker_screenon();						// Turn display on	
}




char* gltype_to_text(int gltype)
{
        static char gsts[32];

        bzero(&gsts,sizeof(gsts));
        sprintf(gsts,"UNKNOWN");
        switch (gltype)
        {
                case GROUP_LIGHT_TYPE_BIT:      sprintf(gsts,"BIT");    break;
                case GROUP_LIGHT_TYPE_MONO:     sprintf(gsts,"MONO");   break;
                case GROUP_LIGHT_TYPE_RGB:      sprintf(gsts,"RGB");    break;
                case GROUP_LIGHT_TYPE_RGBW:     sprintf(gsts,"RGBW");   break;
        }
        return((char*)&gsts);
}



void hangup_server()
{
	online=FALSE;
	if (feature_terminal==TRUE)
	{
		if (fd_terminal>0)
			close(fd_terminal);
		fd_terminal=-1;
	}
	if (fd_binmsg>0)
		close(fd_binmsg);
	fd_binmsg=-1;
	if (fd_monitor>0)
		close(fd_monitor);
	fd_monitor=-1;
	conn_set_defaults(0);
}


void sig_handler(int signo)
{
        if (signo==SIGPIPE)
        {
		hangup_server();
        }
        else    exit(1);
}



char* datetimes(int fmt)
{
	struct tm *current;
	time_t now;
	static char dt[1024];

	time(&now);
        current = localtime(&now);

	switch (fmt)
        {
                case 30:
                        current = gmtime(&now);							// UTC time
                        sprintf(dt,"%s", asctime(current));
                        dt[strlen(dt)-1]=0;
                        strcat(dt," UTC");
                break;
	 }
        return((char*)&dt);
}




void conn_disconnect(int i, int caller)
{
	printf("conn_disconnect caller=%d\ti=%d\n",caller,i); fflush(stdout);
	close(conns[i].fd);
	conn_set_defaults(i);
	conns[i].fd=-1;
	hangup_server();
}


void monitor_printf(int e, const char *fmt, ...)
{
}



void devf_clear(int i)
{
        if (i<MAX_DEVF)
        {
                devf[i].uid[0]          = 0;
                devf[i].ipaddr[0]       = 0;
                devf[i].jcp_udp_port    = 0;
                devf[i].jlp_udp_port    = 0;
                devf[i].univ            = 0;
                devf[i].fchan           = 0;
                devf[i].dev_name[0]     = 0;
                devf[i].dev_type = DEV_NONE;							// mark entry as 'free'
        }
}



// Set a devn slot to its defaults
void devn_clear(int i)
{
        int t=0;

        if (i<MAX_DEVN)
        {
                devn[i].uid[0]          = 0;
                devn[i].ipaddr[0]       = 0;
                devn[i].jcp_udp_port    = 0;
                devn[i].jlp_udp_port    = 0;
                devn[i].group           = 0;
                devn[i].dev_name[0]     = 0;
                bzero(&devn[i].ds, sizeof(struct dev_state));
                devn[i].last_devstate_update_timems = 0;
                devn[i].hide            = FALSE;
                devn[i].dev_type        = DEV_NONE;
                devn[i].subscribed      = FALSE;
                for (t=0;t<MAX_TOPICS;t++)
                        devn[i].topics[t]='0';
        }
}


// If a group is active then free the memory, always zero the values
void group_clear(int g)
{
        int i=0;

	bzero(grpc[g].name,MAX_GROUPNAME_LEN);
	grpc[g].gltype  = GROUP_LIGHT_TYPE_MONO;                                        // default 8 bit unsigned value (DMX style)
	grpc[g].noc     = 0;                                                            // mark as inactive
	grpc[g].save_pending = FALSE;

	grpv[g].R       = 0;
	grpv[g].G       = 0;
	grpv[g].B       = 0;
	grpv[g].W       = 0;
	grpv[g].onoff   = 0;                                                            // off
	grpv[g].last_change_time_ms = 0;
	for (i=0;i<MAX_GROUPEL;i++)
	{
		grpc[g].univ[i]=0;
		grpc[g].chan[i]=0;
	}
}



void init_group()
{
        int i=0;

	//printf("init_group()\n"); fflush(stdout);
        for (i=0;i<MAX_GROUPS;i++)
                group_clear(i);
}


void init_devf()
{
	int i=0;

	//printf("init_devf()\n"); fflush(stdout);
	for (i=0;i<MAX_DEVF;i++)
		devf_clear(i);

}

void init_devn()
{
	int i=0;

	//printf("init_devn()\n"); fflush(stdout);
	for (i=0;i<MAX_DEVN;i++)
		devn_clear(i);
}


// We only use the first array index of conns[]
void conn_set_defaults(int i)
{
	conns[i].fd=fd_binmsg;
	conns[i].mode = CONN_MODE_BIN;
	conns[i].gotheader = FALSE;
	conns[i].idx=0;
	conns[i].t=0;
	bzero(&conns[0].hdr, sizeof(struct bin_header));
}



void persistant_binmsg_poll()
{
	int n=0;
	int i=0;

	int g=0;
	int v=0;
	char samplename[1024];

	// return is 0 for no message, 1 for header, 2 for payload, negative for error
	do
	{
		n=bin_msg_poll(0, binpayload);
		//if (n!=0)
			//{ printf("persistant_binmsg_poll()  n=%d\n",n); fflush(stdout); }
		if (n==-2)
		{
			printf("Error, lost sync, this should never happen\n");
			exit(1);
		}
		if (n==2)									// got a complete message
		{
			//print_header(&conns[0].hdr);
			i=conns[0].hdr.idx;
			switch (conns[0].hdr.msg_type)
			{
				case BIN_MSG_GROUP_CLEAR:
					jlc_change(conns[0].hdr.msg_type, conns[0].hdr.idx);
					group_clear(i);
					return;
				break;

				case BIN_MSG_DEVF_CLEAR:
					devf_clear(i);
					jlc_change(conns[0].hdr.msg_type, conns[0].hdr.idx);
					return;
				break;
                
				case BIN_MSG_DEVN_CLEAR:
printf("clearing idx=%d  uid=%s\n",i,printuid(devn[i].uid)); fflush(stdout);
					devn_clear(i);
					jlc_change(conns[0].hdr.msg_type, conns[0].hdr.idx);
					return;
				break;

				case BIN_MSG_PLAYSOUND:
					bzero(&samplename,sizeof(samplename));					// clear it only when we use it
					printf("PLAYSOUND [%s]\n", binpayload);
					sscanf(binpayload,"%d %d %s",&g,&v,samplename);
					playsample(v, v, samplename);
					fflush(stdout);
				break;

				case BIN_MSG_GROUP_CFG:
					if (i<MAX_GROUPS)
						memcpy(&grpc[i], binpayload, sizeof(struct jlc_group_cfg));
				break;

				case BIN_MSG_GROUP_VAL:
					if (i<MAX_GROUPS)
						memcpy(&grpv[i], binpayload, sizeof(struct jlc_group_val));
				break;

				case BIN_MSG_DEVF:
					memcpy(&devf[i], binpayload, sizeof(struct jlc_devf));			// overwrite struct with new one
				break;

				case BIN_MSG_DEVN:
					memcpy(&devn[i], binpayload, sizeof(struct jlc_devn));
				break;

				case BIN_MSG_CLRP:
					memcpy(&clrp[i], binpayload, sizeof(struct jlc_colourpreset));
printf("Got colour preset %d\n",i); fflush(stdout);
				break;

				case BIN_MSG_DATETIME:
					strcpy((char*)&server_datetime, binpayload);
				break;
			}
			jlc_change(conns[0].hdr.msg_type, conns[0].hdr.idx);
		}
	} while(n>0);												// while more incoming data?
}



// Read from file until we hit charcater c
void fd_readfor(int fd, char ec)
{
	char c;
	int n=0;

	do
	{
		n=read(fd_binmsg,&c,1);
		if (n==1)
		{
			//printf("%02X %c\t",(unsigned char)c,c);
			printf("%c",(unsigned char)c);
			if (c==ec)
				return;
		}
	} while (c!=ec);
}



// Try and connect to server
void connect_to_server()
{
        // Connect to server
        bzero(&con_binmsg,sizeof(con_binmsg));
        bzero(&con_monitor,sizeof(con_monitor));
        bzero(&con_terminal,sizeof(con_terminal));
	bzero(&server_ipaddr,sizeof(server_ipaddr));

	if (sipaddr[0]!=0)							// command line specifies IP ?
		strcpy(server_ipaddr,sipaddr);					// then use it
	else	jcp_find_server((char*)&server_ipaddr, 5000);			// otherwise try and discover it 

	if (strlen(server_ipaddr)<6)
	{
		printf("connect_to_server() Currently can not find a server, no IP address\n");
		fflush(stdout);
		sleep(5);
		return;
	}

        printf("Trying to connect to server %s\n",server_ipaddr);
        fflush(stdout);
        online=TRUE;                                                            // assume this works
        fd_binmsg=sockinit(&con_binmsg, server_ipaddr, server_tcpport, FALSE);
        if (fd_binmsg<=0)
                online=FALSE;
        fd_monitor=sockinit(&con_monitor, server_ipaddr, server_tcpport, TRUE);
        if (fd_monitor<=0)
                online=FALSE;
        if (feature_terminal==TRUE)
        {
                fd_terminal=sockinit(&con_terminal, server_ipaddr, server_tcpport, TRUE);
                if (fd_terminal<=0)
                        online=FALSE;
        }

        if (online!=TRUE)                                                       // Not online, or partially connected
        {
                printf("Connection failed\n");
		hangup_server();
        }
        else                                                                    // Are online, ask for initial state
        {
		printf("Connected\n");
		xprintf(fd_monitor,"mon scr\r");				// monitor scripts

		fd_readfor(fd_binmsg, '>');
		xprintf(fd_binmsg,"bin\r");					// enter binary mode
		conns[0].fd = fd_binmsg;
		if (align_for_binary_mode(fd_binmsg)!=TRUE)
		{
			printf("Error trying to read upti JLCB header magic\n");
			exit(1);
		}
		printf("\n");
        }
	fflush(stdout);
}




void help()
{
	printf("%s:\n",PROGNAME);
	printf("-s <addr>\tConnect to server with hostname or IP address (required)\n");
	printf("-h\t\tHelp text\n");
	//printf("-hv\t\tHide all unused visual controls\n");
	printf("-ec\t\tEnable caption (date/time)\n");
	printf("-dc\t\tDisable caption (date/time)\n");
	printf("-dt\t\tDisable terminal\n");
	printf("-ss\t\tShow switches on Control tab\n");
	printf("-ds\t\tDo not show switches on Control tab\n");
	printf("-p <port>\tTCP port for server\n");
	printf("-aw <width>\tApp width\n");
	printf("-ah <height>\tApp height\n");
	printf("-sb\t\tManage screen blanking\n");
	printf("-ms\t\tUnblank screen for movement sensor\n");
	printf("-fs\t\tFullscreen\n");
}


void init_colour_presets()
{
	int i=0;

	for (i=1;i<=MAX_COLOUR_PRESETS;i++)
	{
		clrp[i].R=0;
		clrp[i].G=0;
		clrp[i].B=0;
		clrp[i].W=0;
		clrp[i].active=FALSE;
	}
}


int main(int argc, char **argv)
{
        char cmstring[1024];
        int argnum=0;

	init_group();
	init_devf();
	init_devn();
	init_colour_presets();
	conn_set_defaults(0);

        strcpy(cmstring,"-h");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE )
	{
		help();
		exit(0);
	}

        sipaddr[0]=0;
        strcpy(cmstring,"-s");
        if ( (parse_commandlineargs(argc,argv,cmstring)==TRUE || strlen(server_ipaddr)<7) )
        {
                argnum=parse_findargument(argc,argv,cmstring);
                if (argnum>0)
                        strcpy(sipaddr,argv[argnum]);
        }

        strcpy(cmstring,"-aw");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
        {
                argnum=parse_findargument(argc,argv,cmstring);
                if (argnum>0)
                        app_width=atoi(argv[argnum]);
        }

        strcpy(cmstring,"-ah");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
        {
                argnum=parse_findargument(argc,argv,cmstring);
                if (argnum>0)
                        app_height=atoi(argv[argnum]);
        }


        strcpy(cmstring,"-p");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
        {
                argnum=parse_findargument(argc,argv,cmstring);
                if (argnum>0)
                        server_tcpport=atoi(argv[argnum]);
        }
	

        strcpy(cmstring,"-ec");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_caption=TRUE;
        strcpy(cmstring,"-dc");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_caption=FALSE;


        strcpy(cmstring,"-dt");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_terminal=FALSE;
        strcpy(cmstring,"-et");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_terminal=TRUE;

        strcpy(cmstring,"-ss");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_show_switches=TRUE;
        strcpy(cmstring,"-ds");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_show_switches=FALSE;


        strcpy(cmstring,"-sb");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_manage_screen_blanking=TRUE;
        strcpy(cmstring,"-ms");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_unblank_for_movement_sensor=TRUE;

        strcpy(cmstring,"-fs");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		feature_fullscreen=TRUE;

	printf("%s Ver: %s\n",PROGNAME,VERSION); fflush(stdout);
	sprintf(windowtitle,"%s Ver: %s",PROGNAME,VERSION);
	binpayload=malloc(65538);							// uint16_t plus a couple
	if (jlc_gui_init()!=0)
		exit(1);
	if (jlc_gui_defaults()!=0)
		exit(1);

	signal(SIGPIPE, sig_handler);
	init_screenblanker();
	if (jlc_gui_run()!=0)
		exit(1);
	return(0);
}

