//*********************************************************************************************************************************
// Render small text into one byte per pixel framebuffer
// *TEXT    string
// *destfb  Pointer to one byte per pixel frame buffer
// width    Width of framebuffer
// height   Height of framebuffer
// proportional   TRUE or FALSE
//	    if TRUE characters are moved left as rendered to occupy differing widths, an "l" for example takes less
//          pixels then an "m"
//
// returns the real number of pixels rendered for a line of text

#include <strings.h>
#include <string.h>
#define TRUE 1
#define FALSE 0

// On the Pi board this burns only a few % more CPU cycles
#define BUFFERTEST									// uncomment this if you get segfaults

#include "fontsmall.h"
unsigned long int render_text_to_buffer_small(char *TEXT,unsigned long int textlength,unsigned char *destfb, unsigned long int fbsize,unsigned long int width, int height, int proportional, int color)
{
	int			i,x;
        unsigned char  		thechar;
        unsigned char     	*ptr;
        unsigned long int	offset;							// Added to pointer to access frame buffer (destfb)
	unsigned long int	c;
        unsigned long int	cwidth;
	long int 		scrollwidth;

	scrollwidth=0;
        ptr=destfb;  									// Set ptr to start of this scan line
        for (x=0;x<FONTSMALL_HEIGHT;x++)                                                // for every line
        {
		offset=width*x;								// start of next line
#ifdef BUFFSERTEST
		if (offset>fbsize)
		{
			printf("Offset [%u] > buffer size [%u]\n",offset,fbsize);
			exit(1);
		}
#endif
                for (c=0;c<textlength;c++)                                              // for every character accross the line
                {
			if (TEXT[c]<=FONTSMALL_LASTCHAR && TEXT[c]>=FONTSMALL_FIRSTCHAR)
                        	thechar=TEXT[c]-' ';					// remove any ASCII not in our font
			else	thechar=' ';
                        if (proportional == TRUE)
                                cwidth=atoi((const char *)fontsmall[thechar][0]);                     // Get the width of this character from font definition
                        else    cwidth=FONTSMALL_WIDTH-1;
			//printf("[%s]  TEXT[c]=%c(%d) cwidth=%d\n",fontsmall[thechar][0],TEXT[c],TEXT[c],cwidth); fflush(stdout);
			//if (cwidth==0)
				//exit(0);

                        for (i=0;i<cwidth;i++)                                          // for every pixel of the character
                        {
                                // [The character, 0=A] [9 lines] [of 9 pixels(chars)]
                                if (fontsmall[thechar][x+1][i]=='*')
                                        ptr[offset]=color;
                                else    ptr[offset]=0;
                                offset++;
#ifdef BUFFSERTEST
				if (offset>fbsize)
				{
					printf("Offset [%u] > buffer size [%u]\n",offset,fbsize);
					exit(1);
				}
#endif
				scrollwidth++;
                        }
                        offset++;       						// character spacing, one pixel
#ifdef BUFFSERTEST
			if (offset>fbsize)
			{
				printf("Offset [%u] > buffer size [%u]\n",offset,fbsize);
				exit(1);
			}
#endif
			//scrollwidth++;
			// comment this ++ out to test font widths
                }
        }
	scrollwidth=scrollwidth/FONTSMALL_HEIGHT;
	return(scrollwidth);
}



//*********************************************************************************************************************************
// *textfb  -   Source framebuffer, normally long wide strip of memory containing text pixels  (one byte per pixel mono)
// *destfb  -   Destination framebuffer (one bytes per pixel mono)
// skip_lines - Move text down this many lines
// scrollpos  - The first column of pixels to copy from source framebuffer (offset from the left)
// Source and destination fb must have same width
void scrolltextcopy(unsigned char *textfb, int textfb_width,int textfb_height, unsigned char *destfb, int fb_width, int destfb_height, int skip_lines,int scrollpos)
{
        int linestart=0;
        int line,pixels;
        int offset;

        offset=fb_width*skip_lines;                                                             // start this many scan lines down the destination framebuffer
        for (line=0;line<destfb_height;line++)                                                  // Copy pixeks from textfb framebuffer into destination framebuffer
        {
                linestart=(textfb_width*line)+scrollpos;
                for (pixels=0;pixels<fb_width;pixels++)                                         // for each line of destination buffer
                {
                        if (offset+(line*fb_width)+pixels<textfb_height*fb_width)               // copy only if pixel in range, tidy this up
                                destfb[(offset+(line*fb_width))+pixels]=textfb[linestart+pixels];       // Copy pixel into destination frame buffer
                        //printf("%d = %d [%d]\t",(128*17)+pixels,linestart+pixels,textfb[linestart+pixels]);
                }
        }
}





#include <time.h>
int get_clockseconds()
{
        time_t  t;
        struct tm   *tm;

        time(&t);
        tm=localtime(&t);
        return(tm->tm_sec);
}



// This makes a huge difference on 3.x kernel and works reasonably well on 2.6 kernel
#include <sched.h>
void set_realtime(void)
{
  	struct sched_param sparam;
  	sparam.sched_priority = sched_get_priority_max(SCHED_RR);
  	sched_setscheduler(0, SCHED_FIFO, &sparam);
}




// Returns true if a file exists
int file_exists(char *fname)
{
        FILE *fp;

        if ((fp=fopen(fname,"r+"))==NULL)
                return(FALSE);

        fclose(fp);
        return(TRUE);
}



// ***************************************************************************************************************************
// Parse command line arguments
int parse_commandlineargs(int argc,char **argv,char *findthis)
{
        int i;
        char tm[512];

        if (argc==1)
                return(FALSE);                                                          // If no command line args then dont bother

        for (i=1;i<argc;i++)                                                            // For every command line argument
        {
                if (strcmp(findthis,argv[i])==0)                                        // If this is the string we are looking for
                {
                        //printf("found %s\n",findthis);
                        tm[0]='\0';
                        if (argc-1>i)                                                   // If the next argument exists then thats the bit we need to
                        {                                                               // return to the caller.
                                strcpy(tm,argv[i+1]);
                                //printf("args=%s\n",tm);
                        }

                        if (strlen(tm)>0)                                               // If have another agument after our string
                                strcpy(findthis,tm);                                    // Then copy it over the callers "findthis" string
                        else    findthis[0]='\0';                                       // or ensure its blank

                        return(TRUE);
                }
        }
        return(FALSE);
}


// ***************************************************************************************************************************
// Which command line argument does this string occur in
// Returns -1 if not found or the argument number if the string is found
int parse_findargument(int argc,char **argv,char *findthis)
{
        int i;
        char tm[512];

        if (argc==0)
                return(-1);   								// If no command line args then dont bother

        for (i=0;i<argc;i++)                                                            // For every command line argument
        {
                if (strcmp(findthis,argv[i])==0)                                        // If this is the string we are looking for
                {
                        //printf("found %s\n",findthis);
                        //n=0;
                        tm[0]='\0';
                        if (argc-1>i)                                                   // If the next argument exists then thats the bit we need to
                        {                                                               // return to the caller.
                                strcpy(tm,argv[i+1]);
                                //printf("args=%s\n",tm);
                        }

                        if (strlen(tm)>0)                                               // If have another agument after our string
                                strcpy(findthis,tm);                                    // Then copy it over the callers "findthis" string
                        else    findthis[0]='\0';                                       // or ensure its blank

                        return(i);
                }
        }
        return(-1);
}




// ***************************************************************************************************************************
// Check if file modification date has changed.  We can "watch" for file changes this way
time_t get_filemodtime(const char *path) 
{
    struct stat file_stat;
    int err = stat(path, &file_stat);
    if (err != 0) 
    {
        perror("get_filemodtime: ");
        //exit(errno);
	return(100);
    }
    return file_stat.st_mtime;
}
