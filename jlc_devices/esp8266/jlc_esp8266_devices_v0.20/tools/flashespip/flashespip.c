/*
	ipflashesp.c
	Online flash tool for ESP8266
*/


#define	 TRUE	1
#define  FALSE  0

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
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "jcp_protocol_devs.h"


uint16_t session_id=111;
int listenport=JCP_SERVER_PORT+100;
static int message_id=0;
int jcp_server_port=-1;
int count=11;


unsigned int current_timems()
{
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
        return milliseconds;
}

/* test that dir exists (1 success, -1 does not exist, -2 not dir) */
int xis_dir (char *d)
{
	DIR *dirptr;
	if (access ( d, F_OK ) != -1 ) 
	{													// file exists
        	if ((dirptr = opendir (d)) != NULL) 
		{
            		closedir (dirptr);
		} else 
		{
			return -2;										// d exists, but not dir
		}
	} 
	else 
	{
		return -1;											// d does not exist
	}
	return 1;
}



// Send UDP packet 
int udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast)
{
	struct	sockaddr_in cm_sendaddr;                                                                         // An IP address record structure
	static int cm_sockfd=-1;
	int numbytes=0;
	int flags;

	
	if (cm_sockfd<0)
		cm_sockfd = socket(PF_INET,SOCK_DGRAM,0);
	if (cm_sockfd<0)
	{
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return (-1);
	}

	flags = fcntl(cm_sockfd, F_GETFL);                                                                      // Get the sockets flags
	flags |= O_NONBLOCK;                                                                                    // Set NONBLOCK flag
	if (fcntl(cm_sockfd, F_SETFL, flags) == -1)                                                             // Write flags back
	{
		perror("send_command_udp() ,fcnctl failed - could not set socket to nonblocking");
		exit(1);
	}

	if (broadcast==TRUE)
	{
		if((setsockopt(cm_sockfd,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof broadcast)) == -1)
		{
			perror("setsockopt - SO_SOCKET for udp tx socket failed");
			exit(1);
		}
	}
	cm_sendaddr.sin_family = AF_INET;
	cm_sendaddr.sin_port = htons(destination_port);
	cm_sendaddr.sin_addr.s_addr = inet_addr(destination_ip);
	memset(cm_sendaddr.sin_zero,'\0',sizeof(cm_sendaddr.sin_zero));

	numbytes = sendto(cm_sockfd, d, len, 0, (struct sockaddr *)&cm_sendaddr, sizeof(cm_sendaddr));
	return(numbytes);
}


int jcp_init(int port, int fl_reuseport)
{
	int dsockfd=-1;
	static struct sockaddr_in recvaddr;

	jcp_server_port=port;
	dsockfd=socket(PF_INET, SOCK_DGRAM, 0);	
	if (dsockfd<0)
	{
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		fflush(stderr);
		return(-1);
	}

	int flags = fcntl(dsockfd, F_GETFL);
	flags |= O_NONBLOCK;
	if (fcntl(dsockfd, F_SETFL, flags) == -1)
	{
		fprintf(stderr,"error,fcnctl failed - could not set socket to nonblocking");
		fflush(stderr);
		return(-1);
	}

	int optval=1;
	if (fl_reuseport==TRUE)
	{
		if (setsockopt(dsockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval))<0)
		{
        		fprintf(stderr,"SO_REUSEPORT failed, not available on this kernel ?\n");
		}
	}
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_port = htons(port);
	recvaddr.sin_addr.s_addr = INADDR_ANY;
	memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);


	if(bind(dsockfd, (struct sockaddr*) &recvaddr, sizeof recvaddr) == -1)
	{
        	fprintf(stderr,"port%d bind() failed %s\n",port, strerror(errno));
		return(-1);
	}
	//printf("Listening on UDP port %d\n",port);
	return(dsockfd);
}



/*
#define FLASHER_CMD_REQ_FWVER           8
#define FLASHER_CMD_PREPARE             10
#define FLASHER_CMD_FLASHCHUNK          11
#define FLASHER_CMD_RESTART             100
struct __attribute__ ((__packed__)) flasher_esp
{
        char                    magic[8];                                       // FLASH\0\0
        uint16_t                cmd;
        uint16_t                count;
        uint16_t                destport;                                       // reply to flash util on this port
        uint32_t                addr;
        uint16_t                chunk;                                          // 0,1,2 and 3
        char                    flashchunk[1024];                               // 4k per flash sector, so 4 of these for each flash write
};
*/



// Send a message and wait for reply for that message, return 0 if ok, negative on error or timeout
int send_then_wait_for_reply(int sockfd, struct flasher_esp* fe, char* destip, int destport, int felen, int cmd)
{
	struct flasher_esp	fer;
	static int count=112;
	int  tout=0;
	int  s=0;
	int  numbytes=0;
	static struct sockaddr_in recvaddr;
        unsigned int addr_len=sizeof(recvaddr);
	char ipaddr[32];
	int i=0;

	memcpy(fe->magic,"FLASH",5);
	fe->cmd		= cmd;
	fe->count	= ++count;
	fe->destport	= listenport;

	for (i=0;i<5;i++)										// 5 goes N ms spaced
	{
		printf("%s %d len=%d cmd=%d\n",destip,destport,felen,cmd); fflush(stdout);
		printf("."); fflush(stdout);
		tout=current_timems()+(800);								// 200ms timeout
		udp_generic_send((char*)fe, felen, destip, destport, FALSE);
		do
		{
			numbytes = recvfrom (sockfd, &fer, sizeof(struct flasher_esp)-1, 0, (struct sockaddr *)&recvaddr, &addr_len);
        		if (numbytes>0)
        		{
				sprintf(ipaddr,"%s",(char*)inet_ntoa(recvaddr.sin_addr));
				printf("Got packet from %s, size=%d\n",ipaddr,numbytes); fflush(stdout);
				if (fer.count == fe->count)						// reply for our message ?
				{
					//if (cmd==FLASHER_CMD_REQ_FWVER)
						printf("%s replied [%s]\n",ipaddr,fer.flashchunk);
					fflush(stdout);
					return(0);
				}
			}
		} while (current_timems()<tout && numbytes<=0);
		if (fer.count == fe->count)
			break;										// break from loop if we got our reply
	}
	printf("Failed to get response for count=%d\n",fe->count);
	return(-1);
}


// flash a file starting at address addr
int flashfile(int sockfd, char* destip, int destport, char* filename, uint32_t baseaddr)
{
	struct flasher_esp	fe;
	int fd=-1;
	int sz=0;

	printf("Flashing file %s\n",filename); fflush(stdout);

	bzero(&fe,sizeof(struct flasher_esp));
	fe.addr=baseaddr;
	fd = open(filename, O_RDONLY);

	do
	{
		sz = read(fd, (char*)&fe.flashchunk, sizeof(fe.flashchunk));				// Read 1k bytes from file 
//printf("sz=%d\n",sz); fflush(stdout);
		if (sz>0)
		{
			printf("sending block, addr=%08X\n",(unsigned char)fe.addr);
			if (send_then_wait_for_reply(sockfd, &fe, destip, destport, sizeof(struct flasher_esp), FLASHER_CMD_FLASHCHUNK)<0)	// Send block of data	
			{
				printf("Failed to get ACK for this block\nFLASHING FAILED\n\n");
				exit(1);
			}
			fe.addr = fe.addr + sz;								// Add another 1k to offset
			fe.chunk++;
			if (fe.chunk>3)
				fe.chunk=0;								// 0,1,2,3, 0,1,2,3 
		}
		usleep(1000*100);
	} while (sz>0);
	printf("\n\n");
	close(fd);
}


void usage()
{
	printf("flashespip <ipaddress>\n");
	printf("flashespip <ipaddress> <firmware directory>\n");
	exit(0);
}




int main(int argc, char **argv)
{
	struct flasher_esp	fe;
	int sockfd=-1;
	char destip[32];
	char rbuffer[4096+16];
	int destport=0;
	int i=0;

	char flashdir[8192];
	char filename[8192];

	bzero(&flashdir,sizeof(flashdir));
	bzero(&fe,sizeof(struct flasher_esp));
	destport = JCP_SERVER_PORT+1;

	if (argc<=1)
		usage;
	if (argc==2)									// One command line arg
	{
		strcpy(destip,argv[1]);							// cmd line arg is IP address
		for (i=0;i<10;i++)
		{
			sockfd=jcp_init(listenport+i,FALSE);
			if (sockfd>0)
				break;
		}
		if (send_then_wait_for_reply(sockfd, &fe, destip, destport, 32, FLASHER_CMD_REQ_FWVER)<0)
		{
			printf("Timeout or error getting version from %s\n",destip);
			exit(1);
		}
		exit(0);
	}

	if (argc==3)
	{
		strcpy(destip,argv[1]);							// cmd line arg is IP address
		for (i=0;i<10;i++)
		{
			sockfd=jcp_init(listenport+i,FALSE);
			if (sockfd>0)
				break;
		}
		strcpy(destip,argv[1]);							// cmd line arg is IP address
		strcpy(flashdir,argv[2]);						// second arg is flash file directory
		i=xis_dir((char*)&flashdir);	
		if (i!=1)								// not a directory
		{
			if (i==-2)
				printf("Exists, but not a directory [%s]\n\n",flashdir);
			if (i==-1)
				printf("Directory not found [%s]\n\n", flashdir);
			exit(1);
		}

		sprintf(filename,"%s/0x00000.bin", flashdir);
		if (access(filename, F_OK) == -1)
		{
			printf("Mandaroty file [%s] does not exist\n",filename);
			exit(1);
		}
		sprintf(filename,"%s/0x40000.bin", flashdir);
		if (access(filename, F_OK) == -1)
		{
			printf("Mandatory file [%s] does not exist\n",filename);
			exit(1);
		}

		if (send_then_wait_for_reply(sockfd, &fe, destip, destport, 32, FLASHER_CMD_REQ_FWVER)<0)
		{
			printf("Timeout or error getting version from %s\n",destip);
			exit(1);
		}

		if (send_then_wait_for_reply(sockfd, &fe, destip, destport, 32, FLASHER_CMD_PREPARE)<0)		// put target in flash mode
		{
			printf("Target did not acknowledge FLASHER_CMD_PREPARE?\n");
			exit(1);
		}
		//sprintf(filename,"%s/0x00000.bin", flashdir);
		//flashfile(sockfd, (char*)&destip, destport, (char*)&filename, 0x00000);					// flash first file
		sprintf(filename,"%s/0x40000.bin", flashdir);
		flashfile(sockfd, (char*)&destip, destport, (char*)&filename, 0x40000);					// flash first file
		sprintf(filename,"%s/page.mpfs", flashdir);
		if (access(filename, F_OK) != -1)					// if optional file exists
			flashfile(sockfd, (char*)&destip, destport, (char*)&filename, 0x80000);					// flash first file
		send_then_wait_for_reply(sockfd, &fe, (char*)&destip, destport, 32, FLASHER_CMD_RESTART);
		exit(0);
	}
	else usage();
}



