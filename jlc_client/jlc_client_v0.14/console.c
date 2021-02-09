/*
	Universe related functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "jlc.h"
#include <jcp_protocol_devs.h>
//#include "jlc_prototypes.h"


//extern char    cprompt[MAX_PROMPTLEN];


int kbhit()
{
    	static const int STDIN = 0;
    	struct termios term;
	static int initialised=FALSE;

    	if (initialised != TRUE)
    	{
        // Use termios to turn off line buffering
        	tcgetattr(STDIN, &term);
        	term.c_lflag &= ~ICANON;
        	tcsetattr(STDIN, TCSANOW, &term);
        	setbuf(stdin, NULL);
        	initialised = TRUE;
    	}

    	int bytesWaiting;
    	ioctl(STDIN, FIONREAD, &bytesWaiting);
    	return bytesWaiting;
}



char console_cmdline[MAX_CMDLEN];
void console_init()
{
	//bzero(&console_cmdline,sizeof(console_cmdline));
	//printf("%s",cprompt);
	fflush(stdout);
}



// Returns 0 or a character typed
char console_poll()
{
	char st[2];
        int bytesavailable;
	int br=0;

        bytesavailable=kbhit();
        if (bytesavailable>0)
        {
                br=read(0,&st,1);                                                                                // read 1 byte if available
		if (br==1)
		{
			return(st[0]);
		}
        }
	return(0);
}


