/*
	jlc_audioplus.c
	Audioplus packets contain CD quality audio. Process these packets into visuals
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlp.h"

#include "jlc_prototypes.h"



// these 3 could be in the local function but leave here for now
struct sockaddr_in ms_sendaddr;											// An IP address record structure
struct sockaddr_in ms_recvaddr;
struct sockaddr_in disp_recvaddr;
struct sockaddr_in txd_sendaddr;

char BROADCAST_DEST_ADDR[64]= "255.255.255.255\0";



// Audioplus buffers
extern struct  audioplus               audioplus_records[FBSMAX];
extern char    audioplus_ipaddr[32];                                                                   // Audio data is from this source address
extern int     audioplus_sockfd;
extern int     audioplus_frame;
extern int     audioplus_framedelay;
extern char    audioplus_md5[64];


// Visuals
#define AUDIO_FRAMES_PER_SECOND		86
extern int     vis_vu_max;
extern int     vis_vu_left;
extern int     vis_vu_right;
extern int     vis_vu_leftpeak;
extern int     vis_vu_rightpeak;
extern int     vis_flag_agc;
extern int     vis_flag_peakhold;
extern int     vis_flag_ansivu;


extern struct statss                    stats;

extern struct sequencescript           seqs[MAX_SEQ_SCRIPTS];
extern int    seqrun;



void audioplus_clear_buffers()
{
	int i=0;
	
	audioplus_md5[0]=0;
	audioplus_ipaddr[0]=0;
	for (i=0;i<FBSMAX;i++)
		bzero(&audioplus_records[i],sizeof(struct audioplus));
}



// given the current frame number find the buffer with the oldest frame number or the first unused slot
int audioplus_findoldestidx(int current_frame)
{
	int oldest_idx=0;
	int oldest_frame=0;
	int i=0;

        for (i=0;i<FBSMAX;i++)
	{
                if (audioplus_records[i].loops_for_this_track == 0)					// unused slot ?
			return(i);
		if (oldest_frame > audioplus_records[i].loops_for_this_track)				// found something older?
		{
			oldest_frame = audioplus_records[i].loops_for_this_track;
			oldest_idx   = i;
		}
	}
	return(oldest_idx);
}




// Find a record whois frame number is "frames behind" the current frame or failing that the lowest frame number
int audioplus_findidx_of_frame(int frame)
{
	int i=0;

        for (i=0;i<FBSMAX;i++)
	{
		if (audioplus_records[i].loops_for_this_track == frame)	
			return(i);
	}
	return(-1);
}





// setup UDP socket for slave mode (listening)
int setup_audioplus_receiving_socket(int port)
{
	int flags;
	int ms_sockfd=-1;

        if((ms_sockfd = socket(PF_INET,SOCK_DGRAM, 0)) == -1)
        {
                perror("slave_setup_socket() - ms_sockfd ");
                exit(1);
        }
        // Setuop socket, specify listening interfaces etc
        ms_recvaddr.sin_family = AF_INET;
        //ms_recvaddr.sin_port = htons(AUDIOPLUS_PORT);
        ms_recvaddr.sin_port = htons(port);
        ms_recvaddr.sin_addr.s_addr = INADDR_ANY;
        memset(ms_recvaddr.sin_zero,'\0',sizeof (ms_recvaddr.sin_zero));

        // Put socket in non blcoking mode
        flags = fcntl(ms_sockfd, F_GETFL);
        flags |= O_NONBLOCK;
        if (fcntl(ms_sockfd, F_SETFL, flags) == -1)
        {
                perror("setup_audioplus_receiving_socket() - could not set ms_sockfd socket to nonblocking");
                exit(1);
        }
        if(bind(ms_sockfd, (struct sockaddr*) &ms_recvaddr, sizeof ms_recvaddr) == -1)
        {
                perror("slave_setup_socket () - error cant bind");
                exit(1);
        }
	return(ms_sockfd);
}




int slave_receive_audioplus(int ms_sockfd, char* ipstr, struct audioplus *ap_record)
{
	int numbytes;
        unsigned int addr_len;

        addr_len = sizeof(ms_recvaddr);
        numbytes = recvfrom (ms_sockfd, (void*)ap_record, sizeof(struct audioplus), MSG_DONTWAIT, (struct sockaddr*)&ms_recvaddr, &addr_len);


	if (numbytes==sizeof(struct audioplus))
	{
        	sprintf(ipstr,"%s",(char*)inet_ntoa(ms_recvaddr.sin_addr));
		if (strncmp(ap_record->leadin,"PLAYER",sizeof(ap_record->leadin))!=0)
		{
			fprintf(stderr,"slave_receive_audioplus()  leadin not PLAYER? \n");
			fflush(stderr);
			return(-1);
		}
		return(ap_record->loops_for_this_track);
	}
	else
	{
		if (numbytes!=-1)
		{
			fprintf(stderr,"slave_receive_audioplus() Got %d bytes, expected %d bytes\n",numbytes,(int)sizeof(struct audioplus));
			fflush(stderr);
		}
	}
	return (-1);
}



void process_audiofx_fft(struct audioplus *ap_record)
{
	//printf("fft of frame %d\n",ap_record->loops_for_this_track); fflush(stdout);
}



int visslow=AUDIO_FRAMES_PER_SECOND/4;
int vissalt=1;
int peakholdtimel=PEAKHOLDTIME;
int peakholdtimer=PEAKHOLDTIME;
void process_audiofx_vu(struct audioplus *ap_record)
{
	//printf("vu meter of frame %d\n",ap_record->loops_for_this_track); fflush(stdout);
        int x;
        long int l,r;

        l=0; r=0;
        for (x=0;x<NUM_SAMPLES;x=x+2)
        {
                l=l+abs(ap_record->mplayersamples[x]);									// Make all negative samples positive
                r=r+abs(ap_record->mplayersamples[x+1]);
        }

        l=l/NUM_SAMPLES;
        r=r/NUM_SAMPLES;                                                                                                // now its an average for all 512 samples
        l=l/58;
        r=r/58;

        // Automatic gain control :-)
        if (vis_flag_agc==TRUE) 
        {
                visslow--;
                if (visslow<=0)
                {
                        vis_vu_max--;											// decrasing this value increases the meters gain so do it slowly
                        visslow=AUDIO_FRAMES_PER_SECOND/4;
                }
                if (l>vis_vu_max)
                {
                        vis_vu_max=l;
                        //printf("digvu_max=%d\n",digvu_max); fflush(stdout);
                }
        }

        if (l > vis_vu_left)												// if current volume > previous then     
                vis_vu_left=l;												// make it the new value
        if (r > vis_vu_right)
                vis_vu_right=r;

        if (vis_vu_left > 255)
                vis_vu_left=255;
        if (vis_vu_right > 255)
                vis_vu_right=255;

        if (vis_vu_leftpeak < vis_vu_left)										// is current peak less than average level
        {
                peakholdtimel=PEAKHOLDTIME;
                vis_vu_leftpeak = vis_vu_left;										// then move peak level up
        }
        if (vis_vu_rightpeak < vis_vu_right)										// is current peak less than average level
        {
                peakholdtimer=PEAKHOLDTIME;
                vis_vu_rightpeak = vis_vu_right;									// then move peak level up
        }

        vis_vu_left--;
        vis_vu_left--;
        vis_vu_left--;
        vis_vu_left--;
        vis_vu_left--;
	vis_vu_right--;
	vis_vu_right--;
	vis_vu_right--;
	vis_vu_right--;
	vis_vu_right--;

        if (vis_flag_peakhold!=TRUE)											// if flag is not set
        {
                vis_vu_leftpeak--;											// then let the peaks decay
                vis_vu_rightpeak--;
        }
        else
        {
                peakholdtimel--;
                if (peakholdtimel<=0)                                                                                   // hold timer expired ?
                        vis_vu_leftpeak=0;										// remove held peak
                peakholdtimer--;
                if (peakholdtimer<=0)
                        vis_vu_rightpeak=0;
        }

        if (vis_vu_rightpeak <= 0)
                vis_vu_rightpeak=0;
        if (vis_vu_leftpeak <= 0)
                vis_vu_leftpeak=0;
        if (vis_vu_left <= 0)
                vis_vu_left=0;
        if (vis_vu_right <= 0)
                vis_vu_right=0;
}



void ansi_vu(int frame)
{
	int i=0;
	char c=' ';

	printf("%c[s",27);  // save cursor position
	printf("%c[1;1H",27);										// first line
	printf("%c[2K",27);										// clear the line
	printf("%08d ",frame);
	//for (i=0;i<(vis_vu_left/2);i++)
	for (i=0;i<64;i++)
	{
		c=' ';
		if ( i <= (vis_vu_left/3) )
			c='*';
		if ( i == (vis_vu_leftpeak/3) )
   			c='P';
		printf("%c",c);
	}
	printf("%c[u",27);  // restore cursor position
	fflush(stdout);
}



void process_audioplus(struct audioplus *ap_record)
{
	//printf("processing frame %d\n",ap_record->loops_for_this_track); fflush(stdout);
	process_audiofx_fft (ap_record);
	process_audiofx_vu (ap_record);
	if (vis_flag_ansivu == TRUE)
		ansi_vu(ap_record->loops_for_this_track);
	seq_process_audioframe(ap_record->md5hash, ap_record->loops_for_this_track);
	ap_record->loops_for_this_track = 0;							// free up this buffer for re-use
}




// When receiving audioplus data we should get around 86 packets per second = around 180k bytes per second
void audioplus_poll()
{
	int		audioplus_idx=0;
	static int	audioplus_oldestidx=0;
	int		process_frame=0;

	audioplus_frame=slave_receive_audioplus(audioplus_sockfd, (char*)&audioplus_ipaddr, (struct audioplus*)&audioplus_records[audioplus_oldestidx]);
	if (audioplus_frame>0)
	{
		stats.pps_audioplus++;

		if (audioplus_md5[0]==0)
		{
			memcpy(audioplus_md5, audioplus_records[audioplus_oldestidx].md5hash, sizeof(audioplus_records[audioplus_oldestidx].md5hash));
			printf("Now receiving audio for track with md5sum %s, frame=%d\n",audioplus_records[audioplus_oldestidx].md5hash,audioplus_records[audioplus_oldestidx].loops_for_this_track);
			fflush(stdout);
		}

		process_frame = audioplus_frame - audioplus_framedelay;
		if (process_frame>0)
		{
			audioplus_idx = audioplus_findidx_of_frame(process_frame);			// Find the index of buffer containing this frame
			if (audioplus_idx>0)
			{
				if (memcmp(audioplus_records[audioplus_idx].md5hash, audioplus_md5, strlen(audioplus_md5))==0)
					process_audioplus((struct audioplus*)&audioplus_records[audioplus_idx]);// and process it
			}
			else
			{
				printf("Failed to find audiplus frame %d\n",process_frame);
			}
		}
		audioplus_oldestidx = audioplus_findoldestidx(audioplus_frame);				// find oldest of all, ready to overwrite
	}
}



// Fake up audio as if we are receiving it
void audioplus_fake(int s)
{
	static uint32_t		tnow;
	static uint32_t		tprev=0;

	if (seqs[s].num_lines<=0)										// nothing to process?
	{
		seqrun=-1;											// cancel fake audio
		seqs[s].framenumber=-1;
		return;
	}

	tnow=(uint32_t)current_timems();
	if ( (tnow-tprev) >11)											// more than Nms since last run ?
	{
		tprev=tnow;
		seqs[s].framenumber++;
printf("(seqs[s].framenumber=%d\tseqs[s].end_timemark=%d\n",seqs[s].framenumber,seqs[s].end_timemark); fflush(stdout);
		if (seqs[s].framenumber >= seqs[s].end_timemark)						// current audio framenumber off the end of the script?
		{
printf("fakeend\n");
			seqrun=-1;										// cancel fake audio
			seqs[s].framenumber=-1;
			return;
		}
		else	seq_process_audioframe(seqs[s].md5hash, seqs[s].framenumber);
	}
}



