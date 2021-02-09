// The player can act as a normal player (player_mode=PLAYER_NORMAL) or it can act as a MASTER or SLAVE
//
// In MASTER mode the player sends its display data via UDP broadcast on TXPORT
//
// In SLAVE mode the player receives framebuffer and audio data from a master and displays it locally.
// It also sends any keypresses it receives to the MASTER.
//
// Limitation:  It assumes the audio cache (playback delay) is the same both end, might be true if its the 
// 		same hardware.
//

#define AUDIOPLUS_PORT		25000  										// Not user selectable at the moment, keep things simple
//#define DISPLAYDATA_PORT	25001										// Send copy of display data for slave machines (Note: NOT led signs as not in time sync)


#include "player_masterslave.h"

struct audioplus		audioplus_record;								// in slave mode we receive this, in master we send it
int ms_sockfd=0;
int disp_sockfd=0;
int txdisp_sockfd=0;												// initial value matters

// these 3 could be in the local function but leave here for now
struct sockaddr_in ms_sendaddr;											// An IP address record structure
struct sockaddr_in ms_recvaddr;
struct sockaddr_in disp_recvaddr;
struct sockaddr_in txd_sendaddr;

char pslave_receive_originating_ip[64];	
char slave_receive_originating_ip[64];										// text version of IP address
char disp_receive_originating_ip[64];

char BROADCAST_DEST_ADDR[64]= "255.255.255.255\0";

// All these and only these commands to be sent to remote master when in slave mode
// Allow numbers, Up Down Left Right - block everything else or it may get confusing
#define NUM_PCCMDS	21
char proxy_commands[NUM_PCCMDS][32]={"U","D","L","R","m","0","1","2","3","4","5","6","7","8","9","C","M","BUTTON=Bottom","BUTTON=Top","BUTTON=Right","BUTTON=Left"};


// ******************************************************************************
// Send a control command, typically a key press. 
// unlike most of the socket code we open and close the socket for this one.
void send_command_udp(char *cmd, char *destination_ip, int destination_port)
{
	int cm_sockfd;
	struct sockaddr_in cm_sendaddr;										// An IP address record structure
	int numbytes=0;
	char command[512];
	int flags;

	bzero(command,sizeof(command));
	strcpy(command,cmd);
	if (strlen(command)<=0)
		return;

	//if((cm_sockfd = socket(PF_INET,SOCK_DGRAM | SOCK_NONBLOCK,0)) == -1)					// empeg cant do this
	if((cm_sockfd = socket(PF_INET,SOCK_DGRAM,0)) == -1)
        {
                fprintf(stderr,"%s: send_command_udp() -",PROGNAME);
                perror("cm_sockfd for udp master tx socket failed, not a show stopper but may be confusing as we lost something!");
		return;
        }
	flags = fcntl(cm_sockfd, F_GETFL);									// Get the sockets flags
	flags |= O_NONBLOCK;											// Set NONBLOCK flag
	if (fcntl(cm_sockfd, F_SETFL, flags) == -1)								// Write flags back
        {
		perror("send_command_udp() ,fcnctl failed - could not set socket to nonblocking");
                exit(1);
	}

        cm_sendaddr.sin_family = AF_INET;
        cm_sendaddr.sin_port = htons(destination_port);
        cm_sendaddr.sin_addr.s_addr = inet_addr(destination_ip);
        memset(cm_sendaddr.sin_zero,'\0',sizeof(cm_sendaddr.sin_zero));

//flags = fcntl(cm_sockfd, F_GETFL);                                                   
//flags |= O_NONBLOCK;                                                             
//if (fcntl(cm_sockfd, F_SETFL, flags) == -1)                           
        //{
		//perror("error,fcnctl failed - could not set socket to nonblocking");
                //exit(1);
	//}

        numbytes = sendto(cm_sockfd, &command, strlen(command), 0, (struct sockaddr *)&cm_sendaddr, sizeof(cm_sendaddr));
	if (numbytes<=0)
	{
		printf("%s: send_command_udp() - FAILED to send [%s] sendto numbytes=%d\n",PROGNAME,command,numbytes);
		return;
	}
	printf("%s: send_command_udp() - Sent [%s] to [%s] %d Bytes\n",PROGNAME,command,destination_ip,numbytes);
	close(cm_sockfd);
}




// ******************************************************************************
// Setup UDP socket for master (transmitting)
void setup_audioplus_transmitting_socket(char *ipaddr)
{
	int flags;
        //if((ms_sockfd = socket(PF_INET,SOCK_DGRAM | SOCK_NONBLOCK,0)) == -1)
        if((ms_sockfd = socket(PF_INET,SOCK_DGRAM, 0)) == -1)
        {
                fprintf(stderr,"%s: ",PROGNAME);
                perror("ms_sockfd for audioplus tx socket failed");
                exit(1);
        }

	flags = fcntl(ms_sockfd, F_GETFL);									// Get the sockets flags
	flags |= O_NONBLOCK;											// Set NONBLOCK flag
	if (fcntl(ms_sockfd, F_SETFL, flags) == -1)								// Write flags back
        {
		perror("error,fcnctl failed audioplus transmitting socket- could not set socket to nonblocking");
                exit(1);
	}


	if (strcmp(ipaddr,"255.255.255.255")==0)
	{
		if((setsockopt(ms_sockfd,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof broadcast)) == -1)
 		{
			fprintf(stderr,"%s: setup_audioplus_transmitting_socket()",PROGNAME);
			perror("setsockopt - SO_SOCKET for udp tx socket failed");
			exit(1);
        	}
	}
        ms_sendaddr.sin_family = AF_INET;
        ms_sendaddr.sin_port = htons(AUDIOPLUS_PORT);
        ms_sendaddr.sin_addr.s_addr = inet_addr(ipaddr);
        memset(ms_sendaddr.sin_zero,'\0',sizeof(ms_sendaddr.sin_zero));

        printf("%s: Sending master audio+ data to IP: %s  PORT: %d\n",PROGNAME,ipaddr,AUDIOPLUS_PORT);
        fflush(stdout);
}





// ******************************************************************************
// setup UDP socket for slave mode (listening)
int setup_audioplus_receiving_socket(int silent, int fatel)
{
	int flags;

        if((ms_sockfd = socket(PF_INET,SOCK_DGRAM, 0)) == -1)
        {
                perror("slave_setup_socket() - ms_sockfd ");
		if (fatel==TRUE)
                	exit(1);
		else	return(FALSE);
        }
        // Setuop socket, specify listening interfaces etc
        ms_recvaddr.sin_family = AF_INET;
        ms_recvaddr.sin_port = htons(AUDIOPLUS_PORT);
        ms_recvaddr.sin_addr.s_addr = INADDR_ANY;
        memset(ms_recvaddr.sin_zero,'\0',sizeof (ms_recvaddr.sin_zero));

        // Put socket in non blcoking mode
        flags = fcntl(ms_sockfd, F_GETFL);
        flags |= O_NONBLOCK;
        if (fcntl(ms_sockfd, F_SETFL, flags) == -1)
        {
                perror("setup_audioplus_receiving_socket() - could not set ms_sockfd socket to nonblocking");
		if (fatel==TRUE)
                	exit(1);
		else	return(FALSE);
        }
        if(bind(ms_sockfd, (struct sockaddr*) &ms_recvaddr, sizeof ms_recvaddr) == -1)
        {
                perror("slave_setup_socket () - error cant bind");
		if (fatel==TRUE)
                	exit(1);
		else	return(FALSE);
        }

	if (silent!=TRUE)
        	printf("%s: slave_setup_socket() - Listening for audio+ data on  UDP port %d\n",PROGNAME,AUDIOPLUS_PORT);
        fflush(stdout);
	return(TRUE);												// tell caller all is ok
}




//*********************************************************************************************************************************
int slave_receive_audioplus()
{
	int numbytes;
        int addr_len;

        addr_len = sizeof(ms_recvaddr);
        numbytes = recvfrom (ms_sockfd, &audioplus_record, sizeof(audioplus_record), MSG_DONTWAIT, (struct sockaddr *) &ms_recvaddr, &addr_len);

	if (numbytes==sizeof(audioplus_record))
	{
        	sprintf(slave_receive_originating_ip,"%s",(char*)inet_ntoa(ms_recvaddr.sin_addr));
		if (VERBOSE>1)
		{
			fprintf(stderr,"%s: slave_receive_audioplus() - got %d bytes from IP %s\n",PROGNAME,numbytes,slave_receive_originating_ip);
			fflush(stderr);
		}
		if (strcmp(audioplus_record.leadin,"PLAYER")!=0)
		{
			fprintf(stderr,"%s: slave_receive_audioplus() - got packet %d size but leadin field was not 'PLAYER' \n",PROGNAME,numbytes);
			fflush(stderr);
			return(-1);
		}
	}
	return (numbytes);
}




//*********************************************************************************************************************************
// ******* We need to do this even if another process is listening, investigate SO_REUSEPORT

// To simplify and save confusion we force the rule that each LAN can have only one master and any number
// of slaves (as the slaves are not registered in any way with the master it has no idea what slaves are 
// present, if any at all)
//
// Returns FALSE if no master is present, TRUE if a master is present, or 3 if setup of socket failed
int listen_for_master()
{
	int timeout=30;
	int have_packet=FALSE;

	if (setup_audioplus_receiving_socket(TRUE,FALSE)!=TRUE)
		return(3);													// return we failed to setup socket
	while (timeout > 0)
	{
		if (slave_receive_audioplus()>0)
			have_packet=TRUE;											// got some valid UDP data on AUDIOPLUS_PORT
		usleep(1000);
		timeout--;
	}
	close (ms_sockfd);
	ms_sockfd=0;

	if (VERBOSE>1)
	{
		fprintf(stderr,"%s: listen_for_master() - returned ",PROGNAME);
		if (have_packet==TRUE)
			fprintf(stderr,"TRUE");
		else	fprintf(stderr,"FALSE");
		fflush(stderr);
	}
	return(have_packet);
}




//*********************************************************************************************************************************
// If master mode is on  then send a copy of our audio data plus some other stuff
// short frame counter counts from 0 to 85 to indicate 86 frames a second of audio data. It is used to sync up LEDs on 
// slave players.
//void master_txsamples(void *bufptr, int bufsize, int selected_playlist, int selected_track, unsigned char short_frame_counter, int loops_for_this_track)
void master_txsamples(void *bufptr, int bufsize, unsigned char md5hash[], unsigned char short_frame_counter, int loops_for_this_track)
{
	int numbytes;
	if (player_mode != PLAYER_MASTER)
		return;															// If feature not enabled then just bail

	// form a UDP packet
	audioplus_record.RXPORT = RXPORT;												// tell everyone what UDP port we listen for keypresses on
	audioplus_record.DISPLAYDATAPORT = TXPORT;											// tell everyone what UDP port we send display data on
	strcpy(audioplus_record.leadin,"PLAYER");
	//strcpy(audioplus_record.sender_ip ,players_ip_address);
	memcpy(audioplus_record.mplayersamples,bufptr,bufsize);										// copy audio data into udp packet
	audioplus_record.short_frame_counter = short_frame_counter;

//JA modified
	if (flag_paused==TRUE)
		audioplus_record.loops_for_this_track = -1;										// pause is on
	else	audioplus_record.loops_for_this_track = loops_for_this_track;
	memcpy(audioplus_record.md5hash,md5hash,sizeof(audioplus_record.md5hash));


	//audioplus_record.milliseconds=get_time_in_milliseconds();
        numbytes = sendto(ms_sockfd, &audioplus_record, sizeof(audioplus_record), 0, (struct sockaddr *)&ms_sendaddr, sizeof(ms_sendaddr));
	if (VERBOSE >2)
	{
		printf("%s: master_txsamples() - sending %d byte UDP frame on port %d\n",PROGNAME,(int)sizeof(audioplus_record),AUDIOPLUS_PORT);	
		fflush(stdout); 
	}
}


