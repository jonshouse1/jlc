/*

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
#include <jlc.h>



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


