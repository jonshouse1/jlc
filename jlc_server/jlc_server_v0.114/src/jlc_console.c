/*
	jlc_console.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"


extern char    cprompt[MAX_PROMPTLEN];


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
void init_console()
{
	bzero(&console_cmdline,sizeof(console_cmdline));
	printf("%s",cprompt);
	fflush(stdout);
}



// Read a single character from stdin if available and add to command line
void console_poll()
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
			st[1]=0;
			//printf("(%d)",st[0]); fflush(stdout);
			if (st[0]==13 || st[0]==10)
			{
				interpreter(STDOUT_FILENO,(char*)&console_cmdline, sizeof(console_cmdline)-1,FALSE);	
				printf("%s",cprompt);
				fflush(stdout);
				console_cmdline[0]=0;
			}
			else strcat(console_cmdline,st);
		}
        }
}


