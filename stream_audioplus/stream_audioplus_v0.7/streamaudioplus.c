//  Stream audio_plus data at a given host as well as playing audio locally.
//  Usage:
//		streamaudioplus_alsa <Destination IP for Audio> <mp3 filename>
//		./streamaudioplus_pulse 10.10.10.10 /disks/vol2/jnsmp3/lighting_tracks/This\ is\ Halloween.mp3 &>l.txt
//		cat l.txt |grep -v "A" >l1.txt		# remove lines starting with A
//
//	depends on md5sum and mplayer
//



#define PROGNAME "streamaudioplus"
#define VERSION	"0.06"
#define LASTCHANGED "27 Jun 2020"


#include <stdint.h>											// investigate this
#include "player_types.c"
#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>											// FIONREAD
#include <sys/resource.h>
#include <stdlib.h>
#include <math.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/wait.h>


// VU meter
#define DIGVU_MAX       128;                                                                                            // AGC initial value or fixed value if AGC off
int     digvu_la=0;
int     digvu_ra=0;                                                                                                     // Average damped values
int     digvu_lpeak=0;
int     digvu_rpeak=0;
int     digvu_max=DIGVU_MAX;
int     digvu_bgi=2;                                                                                                    // when plotting bargraph dimly light the meter scale with this value
int     digvu_flag_displayavg=TRUE;                                                                                     // display average (solid bar)
int     digvu_flag_displaypeak=TRUE;                                                                                    // when plotting bargraph include peak
int     digvu_flag_peakhold=FALSE;                                                                                      // if peak is on, then hold the peak rather than decaying it



//#include "lighting_common.c"
#define FIFO_AUDIO	"/tmp/fifo_audio"								// 44.1Khz 16 bit interleaved audio only on this pipe
#define FIFO_CONTROL	"/tmp/fifo_control"								// Commands to control this process (player - front end)
#define FIFO_PLAYER	"/tmp/fifo_player"								// Commands we send to mp3 player (decoder)


// ******************************************************************************
// Player can act as a slave, takes audio and display framebuffer from a remote 'master'
// in this mode the player is just a passive keybaord and buttons.
#define PLAYER_NORMAL	0
#define PLAYER_MASTER	1
#define PLAYER_SLAVE	2
char    player_mode=PLAYER_MASTER;
int flag_vu=FALSE;



// ******************************************************************************
// Mplayer re-samples the mp3 and always presents stereo data at 44.1Khz to this process
#define playback_sample_rate    44100                                                                   // Number of samples per second
#define NUM_SAMPLES  512                                                                                // The number of 44.1Khz 16 bit samples (for one channel)
// 1024 samples is 1024 * 2 (16 bits) = 2048 * 2 (stereo) = 4096 bytes
int fd_audioin = -1;
int fd_mplayercommands = -1;
int audio_chunks_this_track=0;                                                                          // If we play the WAV header mplayer spits out it clicks. Investiage mplayer options one day
int rtimeout;                                                                                           // count tries (looking at audio fifo)
int audioin_timeout=10;                                                                                 // number of spins around trying to get enough audio data
int audioin_surplus=0;                                                                                  // How many bytes are left in audio fifo after we have done a read
int audioin_surplus_chunks=-1;                                                                          // How many chunks of audio are left in named pipe (mode 'f' and 's') - cache fill in mode 'c'
unsigned char sfc=0;                                                                                    // short frame counter. Counts from 0 to roughly 85 for every audio frame transmitted via UDP
int VERBOSE=1;
int quit=FALSE;

int16 mplayersamples[NUM_SAMPLES*2];                                                                    // 44.1Khz stero inerleaved data received from mplayer
int16 mplayersamplesB[NUM_SAMPLES*2];                                                                   // 44.1Khz stero inerleaved data received from mplayer
int16 samplebuffer[NUM_SAMPLES];                                                                        // FFT sample buffer (mono audio), 512 elements of 16 bits each
int vu=0;
#define FRAMES_PER_SECOND       86                                                                      // with these sample sizes we process audio at the rate of N chunks per second
int32 dualsample;
int haveaudio=FALSE;                                                                                    // the main loop can still go round without any audio
int noaudiocount=0;
int noaudio=TRUE;                                                                                       // No audio for N consecutive loops then player is idle
int endofplaylist=FALSE;                                                                                // reached end of current play list
int player_loops_for_this_track=0;                                                                      // not same as chunks_for_this_track, loops includes paused time
int player_loops_for_this_track_peak=0;
char playbacksystem[1024];

// ******************************************************************************
int soundoutput_underruns=0;                                                                            // Failed to supply data fast enough to audio plyback system
int psoundoutput_underruns=0;
int soundoutput_overruns=0;                                                                             // pushed data at sound system too fast

char    audio_read_method='f';                                                                          // read method for audio data, "s"=simple "f"=full "c"=caching full

// ******************************************************************************
// relates to OUR player
#define PLAYER_PLAYING          2                                                                       // its playing now
#define PLAYER_INACTIVE         4                                                                       // mplayer is inactive. Maybe its just between tracks
#define PLAYER_STARTING         8                                                                       // We have only just asked player to start but have not yet heard audio
#define PLAYER_PAUSED           16
#define FASTPOLLCOUNTER         30                                                                      // when a player starts read this many lines from it in a single chunk
int player_playstate = PLAYER_INACTIVE;

// ***************************************************************************************************************************
//  int tm_sec;                   /* Seconds.     [0-60] (1 leap second) */
//  int tm_min;                   /* Minutes.     [0-59] */
//  int tm_hour;                  /* Hours.       [0-23] */
//  int tm_mday;                  /* Day.         [1-31] */
//  int tm_mon;                   /* Month.       [0-11] */
//  int tm_year;                  /* Year - 1900.  */
//  int tm_wday;                  /* Day of week. [0-6] */
//  int tm_yday;                  /* Days in year.[0-365] */
//  int tm_isdst;                 /* DST.         [-1/0/1]*/
//

void datetime(void *dt, int flag_short)
{
        struct tm *current;
        time_t now;
        //char datetimes[1024];

        time(&now);
        current = localtime(&now);

        if (flag_short==TRUE)
                //sprintf(dt,"%i:%i:%i", current->tm_hour, current->tm_min, current->tm_sec);
                sprintf(dt,"%02d:%02d:%02d", current->tm_hour, current->tm_min, current->tm_sec);
        //else    sprintf(dt,"%02d/%02d/%i %i:%i:%i", current->tm_mday, current->tm_mon, current->tm_year+1900, current->tm_hour, current->tm_min, current->tm_sec);
        else    sprintf(dt,"%02d/%02d/%i %02d:%02d:%02d", current->tm_mday, current->tm_mon, current->tm_year+1900, current->tm_hour, current->tm_min, current->tm_sec);
}



// ******************************************************************************
// ALSA does playback (most common)
#ifdef ALSA
        #include <alsa/asoundlib.h>
        snd_pcm_t *playback_handle = NULL;
        snd_pcm_hw_params_t *hw_params = NULL;
        static char *device = "default";                                                                // playback device
        int rc,rcb;                                                                                     // dont use err, its defined elsewhere
        #include "player_initialise_alsa.c"                                                             // function playsample()
        #include "player_playsample_alsa.c"                                                             // function playsample()
#endif


// ******************************************************************************
// Audio optionns, compiler preprocessor should pick one with -D
#ifdef PULSE
        #include <pulse/simple.h>
        #include <pulse/error.h>
        pa_simple *s = NULL;
        pa_buffer_attr paattr;
        #include "player_initialise_pulse.c"
        #include "player_playsample_pulse.c"                                                            // function playsample()
#endif



int fd_playerpipe=-1;
int fd_control_fifo=-1;
char filename[4096];
char cmdline[8192];
char destip[64];
char data[8192];
char selected_track_md5hash[1024];
int flag_paused=FALSE;
int flag_muted=FALSE;
int fps=0;
int broadcast=1;

// ******************************************************************************
char DEST_ADDR[64]= "255.255.255.255\0";
int  TXPORT = 4040;                                                                                     // default port for slave displays and led signs
int  RXPORT = 5050;
int  LOCAL_DISPLAY_PORT = 5051;                                                                         // UDP port for LOCAL display data


//*********************************************************************************************************************************
#include <sched.h>
void set_realtime(void)
{
        struct sched_param sparam;
        sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(0, SCHED_FIFO, &sparam);
        //sparam.sched_priority = sched_get_priority_max(SCHED_RR);
        //sched_setscheduler(0, SCHED_RR, &sparam);
}


// ***************************************************************************************************************************
int get_clockseconds()
{
        time_t          t;
        struct tm       *tm;

        time(&t);
        tm=localtime(&t);
        return(tm->tm_sec);
}




#include "streamaudioplus_masterslave.c"

#define PEAKHOLDTIME 36
int visslow=FRAMES_PER_SECOND/4;
int vissalt=1;
int peakholdtimel=PEAKHOLDTIME;
int peakholdtimer=PEAKHOLDTIME;
void process_visuals()
{
        int n,x,y;
        long int l,r;

        n=0; l=0; r=0;
        for (x=0;x<NUM_SAMPLES;x=x+2)
        {
                l=l+abs(audioplus_record.mplayersamples[x]);                                                            // Make all negative samples positive
                r=r+abs(audioplus_record.mplayersamples[x+1]);
        }

        l=l/NUM_SAMPLES;
        r=r/NUM_SAMPLES;                                                                                                // now its an average for all 512 samples
        l=l/10;
        r=r/10;


        if (l > digvu_la)                                                                                               // if current volume > previous then     
                digvu_la=l;                                                                                             // make it the new value
        if (r > digvu_ra)
                digvu_ra=r;

        if (digvu_la > 255)
                digvu_la=255;
        if (digvu_ra > 255)
                digvu_ra=255;

        if (digvu_lpeak < digvu_la)                                                                                     // is current peak less than average level
        {
                peakholdtimel=PEAKHOLDTIME;
                digvu_lpeak = digvu_la;                                                                                 // then move peak level up
        }
        if (digvu_rpeak < digvu_ra)                                                                                     // is current peak less than average level
        {
                peakholdtimer=PEAKHOLDTIME;
                digvu_rpeak = digvu_ra;                                                                                 // then move peak level up
        }


	digvu_la=digvu_la-10;
	digvu_ra=digvu_ra-10;

        if (digvu_flag_peakhold!=TRUE)                                                                                  // if flag is not set
        {
                digvu_lpeak--;                                                                                          // then let the peaks decay
                digvu_rpeak--;
        }
        else
        {
                peakholdtimel--;
                if (peakholdtimel<=0)                                                                                   // hold timer expired ?
                        digvu_lpeak=0;                                                                                  // remove held peak
                peakholdtimer--;
                if (peakholdtimer<=0)
                        digvu_rpeak=0;
        }


        if (digvu_rpeak <= 0)
                digvu_rpeak=0;
        if (digvu_lpeak <= 0)
                digvu_lpeak=0;
        if (digvu_la <= 0)
                digvu_la=0;
        if (digvu_ra <= 0)
                digvu_ra=0;
}



//**************************************************************************
// for a given file read it until its empty
void drain_fifo(int file_descriptor)
{
        int bytesavailable=0;
        unsigned char somedata[1024];
        int res=0;

        res=ioctl (file_descriptor,FIONREAD,&bytesavailable);                                                   // How many bytes available for read ?  
        if (bytesavailable<0)
                return;

        bytesavailable=0;
        do                                                                                                      // drain all data
        {
                bytesavailable=read(file_descriptor,&somedata,sizeof(somedata));                                // read and ignore some data
        } while (bytesavailable >0);                                                                            // more then no bytes and not an error
}


// ******************************************************************************
// read the exit status of any children we spawned, if we dont do this the
// new mplayer process will complete with an older zombie one
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


void player_ask(char *question)
{
        char cmd[2048];
        int numbytes=0;
        char st[1024];

        reap_all();
        strcpy(cmd,question);                                                                                   // we modify it possibly, so make a copy

        strcat(cmd,"\n");
        if (fd_mplayercommands < 0)
        {
                // This open seems to lock for upto 4 seconds, still does - but do it just when mplayer starts
                fd_mplayercommands=open(FIFO_PLAYER, O_WRONLY | O_NONBLOCK | O_NDELAY);                         // commands destined for mplayer instance

        }
        if (fd_mplayercommands < 0)
        {
                fprintf(stderr,"%s: Failed open open %s for write, but that is ok\n",PROGNAME,FIFO_PLAYER);
                fflush(stderr);
                //perror("Err");
                fd_mplayercommands=-1;
                if (strlen(cmd)>=1)
                        cmd[strlen(cmd)-1]=0;                                                                   // remove newline from message
                fprintf(stderr,"%s: player_ask() - MESSAGE [%s] Lost\n",PROGNAME,cmd);
                fflush(stderr);
                fd_mplayercommands=-1;
                return;
        }

        numbytes=write(fd_mplayercommands,&cmd,strlen(cmd));                                            // if not in error then send our text and bail
        if (numbytes<=0)
	{
		if (strlen(cmd)>=1)
                                cmd[strlen(cmd)-1]=0;                                                           // remove newline from message
                        printf("MESSAGE [%s] Lost\n",cmd);
                        fflush(stdout);
	}

        if (VERBOSE>0)
	{
                        fprintf(stderr,"%s: player_ask() - sent [%s]\n",PROGNAME,question);  fflush(stderr);
                        fflush(stderr);
	}
}


// ******************************************************************************
// fails to close audio fifo , never mind
void mshutdown()
{
        int i;

        close(fd_control_fifo);
        fd_control_fifo=-1;
        close(fd_audioin);
        fd_audioin=-1;
        //fft_close (state);                                                                            // free fft memory
        printf("\n\nPlayer exited cleanly,  %d frames sent\n",player_loops_for_this_track_peak);
        exit(0);
}





#include "streamaudioplus_readsample.c"


int main(int argc, char **argv)
{
	FILE *f;
	int x;
	int p;
        int clockseconds=0;
        int pclockseconds=0;
        int loopcounter;
 	int readsize=0;
	int lc=0;
	int framesleft=0;


	if (argc<3)
	{
		printf("Usage: streamaudioplus <Destionation IP for Audio> 'Filename'\n");
		exit(0);
	} 

	
	strcpy(destip,argv[1]);
	strcpy(filename,argv[2]);
	set_realtime();

	// Collect the MD5 hash for the file we are about to stream
	sprintf(cmdline,"md5sum '%s'",filename);
  	f = popen(cmdline, "r");
  	if (f == NULL) 
	{
    		printf("Failed to run command\n" );
    		exit(1);
  	}
  	while (fgets(data, sizeof(data)-1, f) != NULL) 
	{
  	}
	pclose(f);


	// Trim out all but md5
	for (x=0;x<strlen(data);x++)
		if (data[x]==' ')
			data[x]=0;									// Terminate string at first space

	// Check we have an md5 now
	if (strlen(data)<1)
	{
		printf("Reading MD5 for [%s] failed \n",filename);
		exit(1);
	}
	strcpy(selected_track_md5hash,data);



        // ******************************************************************************
        // Ensure we have the named pipes, often these will exist so an error here is not fatel
        // fifo needs to end up with mode 777, simplest way to ensure user processes can write/read
        x=(mkfifo(FIFO_AUDIO, 0777));                                                                   // 0 prefix is octal, I use it so little, was 0666 now 0777
        if (x != 0)
                printf("%s: mkfifo %s returned %s \n",PROGNAME,FIFO_AUDIO,strerror(errno));
        chmod(FIFO_AUDIO,0777);                                                                         // for some reason mkfifo is not doing the business with mode ? FIXME
        x=(mkfifo(FIFO_CONTROL, 0777));
        if (x != 0)
                printf("%s: mkfifo %s returned %s\n",PROGNAME,FIFO_CONTROL,strerror(errno));
        chmod(FIFO_CONTROL,0777);
        x=(mkfifo(FIFO_PLAYER, 0777));
        if (x != 0)
                printf("%s: mkfifo %s returned %s\n",PROGNAME,FIFO_PLAYER,strerror(errno));
        chmod(FIFO_PLAYER,0777);
        fflush(stdout);

        fd_playerpipe=open(FIFO_PLAYER, O_RDWR | O_NONBLOCK);
        if (fd_playerpipe>0)
        {
                do
                {
                        x=read(fd_playerpipe,&mplayersamples,sizeof(mplayersamples));                   // Read and throw away any stale data heading for /fifo_player
                } while(x>0);                                                                           // this removes any processes blocking waiting to write
                close(fd_playerpipe);
        }




        // ******************************************************************************
        // Initialise lots of things
        initialise_sound_system(NUM_SAMPLES, 2, playback_sample_rate, TRUE);                            // configure sound sub system


#ifdef ALSA
        uid_t uid=getuid(), euid=geteuid();
        printf("%s: uid=%d  euid=%d\n",PROGNAME,uid,euid);
        if ( (uid==0) | (euid==0) )
        {
                printf("Running as root on INTEL with ALSA does not seem to work, run as non-root\n");
                exit(0);
        }
#endif



        // ******************************************************************************
        // Open audio fifo for read
        fd_audioin=open(FIFO_AUDIO, O_RDONLY | O_NONBLOCK);                                             // On the Pi tends to be file descriptor 3
        if (fd_audioin<=0)
        {
                printf("%s: Opening fifo %s for read failed (%s)\n",PROGNAME,FIFO_AUDIO,strerror(errno));
                exit(1);
        }




        // ******************************************************************************
        // Control fifo
        errno=0;
        fd_control_fifo=open(FIFO_CONTROL, O_RDWR | O_NONBLOCK);                                        // Open fifo for commands processes send to us
        if (fd_control_fifo<=0)
        {
                printf("%s: Error open %s for read/nonblock\n",PROGNAME,FIFO_CONTROL);
                exit(1);
        }

        drain_fifo(fd_audioin);                                                                         // empty mplayer audio fifo of any stale data
        drain_fifo(fd_control_fifo);                                                                    // empty control fifo of any stale commands


	printf("streamaudioplus Version %s\n",VERSION);
	printf("\tStreaming file [%s]\n",filename);
    	printf("\tWith MD5 hash [%s]\n", selected_track_md5hash);
	printf("\tTo IP [%s]\n",destip);

	setup_audioplus_transmitting_socket(destip);

	//sprintf(cmdline,"nice --20 mplayer -vo gl -slave -nolirc -input file=%s -af format=s16le,resample=44100,channels=2,volume=1 -ao pcm:fast:nowaveheader:file=%s \"%s\"  &",FIFO_PLAYER,FIFO_AUDIO,filename);
	sprintf(cmdline,"nice --20 mplayer -quiet -vo gl -slave -nolirc -input file=%s -af format=s16le,resample=44100,channels=2,volume=1 -ao pcm:fast:nowaveheader:file=%s \"%s\"  &",FIFO_PLAYER,FIFO_AUDIO,filename);
printf("%s\n",cmdline);
	system(cmdline);


        // Protect process memory from swapping
        mlockall(MCL_CURRENT | MCL_FUTURE);
	int external_player_startup_time=10;


	while (quit!=TRUE)
	{

		// ******************************************************************************
		// We take input from both a named pipe and listening UDP socket
		//udp_message_poll();
		//control_fifo_poll();									// Read and interpete any commands


		// fetch audio from external mp3 decoder
		// ******************************************************************************
		bzero(&mplayersamples,sizeof(mplayersamples));
		if (flag_paused!=TRUE)									// not paused ?
			readsize=read_audio(audio_read_method,&mplayersamples);				// fetch some audio to process
		else
		{
			player_playstate = PLAYER_PAUSED;
			readsize=sizeof(mplayersamples);						// paused - then pretend we read some audio
		}

                if ( readsize < (int)sizeof(mplayersamples) )						// Short read or error
		{
			printf("NO AUDIO, readsize=%d  noaudiocount=%d\n",readsize,noaudiocount); fflush(stdout);
			if (noaudiocount>200)
				quit=TRUE;

			haveaudio=FALSE;
			bzero(&mplayersamples,sizeof(mplayersamples));					// We timed out reading it so its probably crap
			if (noaudiocount > (FRAMES_PER_SECOND * external_player_startup_time) )		// timeout waiting for audio from externa mp3 decoder ?
			{
				noaudio=TRUE;								// N loops with nothing, then player is idle
				player_loops_for_this_track=0;						// How many times we have processed a given playing track
				bzero(&samplebuffer,sizeof(samplebuffer));				// Make sure visuals are chewing on empty audio buffer
			}
			//if (readsize==-2)								// Timeout reading audio 
			if (readsize<0)									// Timeout reading audio 
			{
				noaudiocount++;								// count timeouts, start a new track if we get many of them
                		playsample(&mplayersamples,sizeof(mplayersamples)); 	 		// Play the silence to keep timing consistent
                		master_txsamples(&mplayersamples,sizeof(mplayersamples),selected_track_md5hash,sfc,-1);	// plays silence when player is paused, blocking helps loop time
				sfc++;
				printf("T"); fflush(stdout);
				//player_poll();	
			}
		}
		else											// good news, we got a full chunk of audio
		{											// didnt read any sound data ?
			haveaudio=TRUE;
			if (readsize != sizeof(mplayersamples))						// Did we get enough audio ?
			{
				fprintf(stderr,"%s: bugger audioin short read, got %d, not %d or 0 bytes, ignoring it\n",PROGNAME,readsize,(int)sizeof(mplayersamples));
				fflush(stderr);
				bzero(&mplayersamples,sizeof(mplayersamples));				// what we got is partial or crap so discard it
			}
			noaudiocount=0;
			player_loops_for_this_track++;							// not the same as chunks_this_track=audio played
			process_visuals();
			if (audio_chunks_this_track >2 )  						// skip first N frames of audio (supress CLICK)
			{
				if (flag_muted==TRUE)							// mute on ?
				{
					bzero(mplayersamplesB,sizeof(mplayersamplesB));
                			playsample(&mplayersamplesB,sizeof(mplayersamplesB));		// Playback silence
                			master_txsamples(&mplayersamplesB,sizeof(mplayersamplesB),selected_track_md5hash,sfc,-1);
					sfc++;
				}
				else
				{
                			playsample(&mplayersamples,sizeof(mplayersamples));		// Playback audio
                			master_txsamples(&mplayersamples,sizeof(mplayersamples),selected_track_md5hash,sfc,audio_chunks_this_track);
					sfc++;
					//afn++;
				}
			}
			else
			{
				bzero(&mplayersamples,sizeof(mplayersamples));				// stop the visuals from display the wav header 
                		playsample(&mplayersamples,sizeof(mplayersamples));			// Playback audio
                		master_txsamples(&mplayersamples,sizeof(mplayersamples),selected_track_md5hash,sfc,audio_chunks_this_track);
				sfc++;
				printf("S");								// S = Skipped T=Timeout
				// got first samples from new player instance, time to do the first open of mplayer_control, may take several seconds - but we have not made a sound yet.
// JA 6 - problem !
// This is sending "get_audio_bitrate" 86 times a second.  Reading mplayer stdout much slower, we over ran the buffer.  Code a bit better now
// why is this even here ?
				player_ask("get_audio_bitrate");					// seems to be needed to move things along ?
				fflush(stdout);
			}
			if (flag_paused!=TRUE)
			{
				audio_chunks_this_track++;						// got good audio, inc counter
				//printf("audio_chunks_this_track=%d\n",audio_chunks_this_track); fflush(stdout);
			}
		}
		if (noaudio==TRUE)									// noaudio is a timeout on receiving audio
		{
			//printf("Exiting, no audio\n");
			//exit(0);
		}



		// ******************************************************************************
                pclockseconds=clockseconds;
                clockseconds=get_clockseconds();
		// Jobs done once a second here
                if (pclockseconds != clockseconds) 							// Clock just ticked over into a new second
                {
                        fps=loopcounter;
                        loopcounter=0;									// Reset the loop counter
			//printf("fps%d  player_loopsforthistrack=%d  fio surplus=%d chunks\n",fps,player_loopsforthistrack,audioin_surplus_chunks); fflush(stdout);
#ifndef EMPEG
			if (audio_read_method=='c')
			{
				if (samplecache->cachefill==0)
				{
					printf("%s: Oh dear, the cache is empty - cachefill=%d  fifo surplus chunks=%d\n",PROGNAME,samplecache->cachefill,samplecache->surplus_chunks);
					fflush(stdout);
				}
			}
#endif
			printf("fc=%d\n",audio_chunks_this_track);
			sfc=0;										// short frame counter is resit by the master (sender) once a second
                }


#ifdef ALSA
   			framesleft=snd_pcm_avail (playback_handle);					// larger = more cache left to be filled (ie needs more data hence less zzzz)
			framesleft=framesleft/10;							// scale value
			if (framesleft>127)
				framesleft=127;
#endif

		usleep(110);										// PC, wild speculative guess
usleep(10000);

                loopcounter++;
		lc++;											// loop counter, never reset

		if (player_loops_for_this_track_peak < player_loops_for_this_track)
			player_loops_for_this_track_peak=player_loops_for_this_track;


		//printf("fc=%d\t",audio_chunks_this_track);
		if (flag_vu==TRUE)
		{
			int l=0;
			for (l=0;l<NUM_SAMPLES;l++)
				vu=vu+abs(mplayersamples[l]);
			vu=vu/NUM_SAMPLES;								// average volume
			vu=vu/10;
			if (vu>p)
				p=vu;									// Take a peak value

			printf("%d\t",vu);								// Volume this frame
			printf("%d\t",(digvu_la+digvu_ra)/2);						// Damped volume this frame
			for (l=0;l<(vu/10);l++)
				printf("*");
			printf("\n");
		}
		fflush(stdout);
	}  //main loop 



	printf(" Done,  peak value=%d\n",p);
	// ******************************************************************************
	// shutdown code, lots of close and free - need finishing
	close(fd_audioin);
	if (fd_mplayercommands>0)
		close(fd_mplayercommands);
	mshutdown();
	return(0);
}



