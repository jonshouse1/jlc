/*
	parse_commandline.c
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "jlc.h"		// for TRUE and FALSE

#include "parse_commandline.h"


// Parse command line arguments
int parse_commandlineargs(int argc,char **argv,char *findthis)
{
        int i=0;
	//int n;
        char tm[512];

        if (argc==0)
                return(FALSE);                                                          // If no command line args then don't bother

        for (i=0;i<argc;i++)                                                            // For every command line argument
        {
                if (strcmp(findthis,argv[i])==0)                                        // If this is the string we are looking for
                {
                        //printf("found %s\n",findthis);
                        //n=0;
                        tm[0]='\0';
                        if (argc-1>i)                                                   // If the next argument exists then that's the bit we need to
                        {                                                               // return to the caller.
                                strcpy(tm,argv[i+1]);
                                //printf("args=%s\n",tm);
                        }

                        if (strlen(tm)>0)                                               // If have another argument after our string
                                strcpy(findthis,tm);                                    // Then copy it over the callers "findthis" string
                        else    findthis[0]='\0';                                       // or ensure its blank

                        return(TRUE);
                }
        }
        return(FALSE);
}


// Which command line argument does this string occur in
// Returns -1 if not found or the argument number if the string is found
int parse_findargument(int argc,char **argv,char *findthis)
{
        int i;
	//int n;
        char tm[512];

        if (argc==0)
                return(-1);   								// If no command line args then don't bother

        for (i=0;i<argc;i++)                                                            // For every command line argument
        {
                if (strcmp(findthis,argv[i])==0)                                        // If this is the string we are looking for
                {
                        //printf("found %s\n",findthis);
                        //n=0;
                        tm[0]='\0';
                        if (argc-1>i)                                                   // If the next argument exists then that's the bit we need to
                        {                                                               // return to the caller.
                                strcpy(tm,argv[i+1]);
                                //printf("args=%s\n",tm);
                        }

                        if (strlen(tm)>0)                                               // If have another argument after our string
                                strcpy(findthis,tm);                                    // Then copy it over the callers "findthis" string
                        else    findthis[0]='\0';                                       // or ensure its blank

                        return(i);
                }
        }
        return(-1);
}



