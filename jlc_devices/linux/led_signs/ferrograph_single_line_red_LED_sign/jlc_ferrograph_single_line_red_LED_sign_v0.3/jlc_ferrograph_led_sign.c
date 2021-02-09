// jlc_ferrograph_led_sign.c 

char PROGNAME[80]="jlc_ferrograph_led_sign.c";
char VERSIONT[20]="0.1";
char text_filename[2048];

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>
#include "jcp-client-linux.h"
#include "jcp_protocol_devs.h"
#include "jcp_client.h"
#include "jlc.h"

#include <sys/ioctl.h>
#include <termios.h>

#include "raspberry_pi_io.c"
#include "sign_common.c"

// No win, scanning mutiple frames before scrolling makes it smear, scanning a single frame then moving makes text distorted
#define GAURD_TIME_US	2
#define ON_TIME_US	1000	
#define OFF_MULT	10


// RPI pins selected not to conflict with SPI, allows an LCD if required
#define COUNTER_CLOCK	14					// GPIO  - 74HC191 Counter Clock
#define SR_DATA		15					// GPIO  - Shift Register DATA
#define SR_CLOCK	18					// GPIO  - Shift Register CLOCK, 74HC164 is CP L2H to latch data


#define PANELS		4							// Each panel is 6 5x7 LEDs wide = 30 LEDs wide
#define LED_DISPLAY_WIDTH		30 * PANELS				// Totoal size of display (width)
#define LED_DISPLAY_HEIGHT		7
unsigned char framebuf[LED_DISPLAY_HEIGHT+1][LED_DISPLAY_WIDTH+1];		// one byte per pixel
int scanline=0;
int scroll=TRUE;								// True for scrolling
int reloading=0;								// non zero if about to reload text
int scans=1;									// number of times to scan the frame

int flag_asciiart=FALSE;
unsigned char image_data[LED_DISPLAY_HEIGHT * LED_DISPLAY_WIDTH *2];
unsigned char *image_long=NULL;                                                 // Very wide buffer to render scroll text into
unsigned long int image_long_width=0;
unsigned long int image_long_size=0;


// Server is pushing state at us.
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
	//int v=0;
        if (idx==-100)                                                          // registered with server now
                return;
        printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\tds->valuebyteslen=%d\n",idx,ds->value1, ds->value2, ds->valuebyteslen);


	// Feature, reset to defaults, clear text
/*
	if (ds->value1==0 && ds->value2==0)
	{
		for (v=0;v<6;v++)
			bzero(&textline[v],sizeof(textline[v]));		// clear all text
		upper=FALSE;							// mixed case text, variable spacing
		prop=TRUE;
		render();
		return;
	}

	// Feature brightness
	if (ds->value1==65000)							// feature set brightness, value2 brightness %
	{
		if (ds->value2>=1 && ds->value2 <=100)				// percentage 1..100
		{
			v = ds->value2 * 10;					// 800 brightest 5 dimmest
			op = 1000 - v;
			printf("set brightness (percentage)  value=%d   op=%d\n",ds->value2, op);
			fflush(stdout);
		}
		return;
	}

	// Feature font proportional or fixed
	if (ds->value1==65001)
	{
		if (ds->value2<=0 && ds->value2>=1)
			prop=ds->value2;
	}


	// Select font (fake this one, 0=small font mixed case prop, 1=small font all uppercase, non prop
	if (ds->value1==65010)
	{
		if (ds->value2==0)
		{
			prop=FALSE;
			upper=TRUE;						// Big text, fixed spacing
		}
		if (ds->value2==1)
		{
			upper=FALSE;						// mixed case text, variable spacing
			prop=TRUE;
		}
	}
*/

	// scroll on or off,  0=default (on), 1=off
	if (ds->value1==65020)
	{
		printf("Turn scroll on or off %d  (0=default on)\n",ds->value2);
		fflush(stdout);
		if (ds->value2==0)
			scroll=TRUE;
		else	scroll=FALSE;	
	}


	// Scroll speed, 0=default, 1=slower, 2=slower still
	if (ds->value1==65022)
	{
		printf("Set scroll speed %d  (0=default fast, 1=slower, 2=slower still etc)\n",ds->value2);
		fflush(stdout);
		if (ds->value2<100)
		{
			scans=1+ds->value2;					// number of scans of the display before moving text
		}
	}




	//if (ds->value1>=1 && ds->value1<=6)					// value1 is lines 1 to 6 inclusive 
	//{
		//v=ds->value1-1;
		//printf("%s v=%d Set LINE %d to [%s]\n",PROGNAME, v, ds->value1, ds->valuebytes);
		//bzero(&textline[v],sizeof(textline[v]));			// clear all text
		//strcpy(textline[v],ds->valuebytes);
		//rerender(v);
	//}
}

void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}


// Server sends us config info, we can ignore this on linux
void jcp_dev_config(char *dt, int len)
{
}



void putpixel(unsigned char*pixels, int bmwidth, int bmheight, int row, int col, int p)
{
	pixels [col+(bmwidth*row)] = p;
}




// Load a file of ASCII text into RAM and render it into the long image buffer ready for scrolling
// returns TRUE if it worked, false if it failed
char *sometext = 0;
int sometext_length = 0;
int loadtextfile(char *filename)
{
        int textfd;
        unsigned long int sz;
        struct stat buf;

        if (sometext)
        {
                free(sometext);                                                                 // we have already malloced, so free first
                sometext=0;
        }

        if (file_exists(filename)!=TRUE)
        {
                sometext = malloc ( 4096 );                                                     // Ask for some ram
                printf("loadtextfile: File not found [%s]\n",text_filename);
                sprintf(sometext,"                        file [%s] not found                         ",text_filename);
                sometext_length=strlen(sometext);
                return(FALSE);
        }

        textfd=open(filename, O_RDONLY);
        if (textfd<=0)
                return(FALSE);

        //printf("loading %s\n",filename); fflush(stdout);
        fstat(textfd, &buf);                                                                    // get directory entry structure
        sz = buf.st_size;
        if ( sz <= 2)                                                                           // could be zero, malloc(0) gives no pointer
        {
                close(textfd);
                sometext = malloc ( 4096 );                                                     // Ask for some ram
                sprintf(sometext,"no text");
                sometext_length=strlen(sometext);
                return(FALSE);                                                                  // not enough bytes to display
        }

        sometext = malloc ( sz + 1 );                                                           // Ask for a nice big chunk of ram
        sometext_length=read(textfd,sometext,sz);
        close (textfd);
        printf("read %d bytes from file [%s]\n",sometext_length,filename);
	sometext[sometext_length]=0;                                                            // terminate string in bufferi
	return(TRUE);
}




// Clock one bit into shift register chain
inline void clocksr()
{
	output_high(SR_CLOCK);
	output_high(SR_CLOCK);
	output_low(SR_CLOCK);
	output_low(SR_CLOCK);
}

void exithandler()
{
	output_low(COUNTER_CLOCK);						// Turn LEDs off
	exit(0);
}


inline void clock_bit(int b)
{
	if (b==1)								// is bit set ?
	{
		//printf("1"); fflush(stdout);
		output_high(SR_DATA);
	}
	else	
	{
		//printf("0"); fflush(stdout);
		output_low(SR_DATA);
	}
	clocksr();
}



// Clock a single byte into shift register
void clock_byte(unsigned char b)
{
	int i;
	for (i=0;i<7;i++)							// for each bit
	{
		if ( (b>>i) & 1)						// is bit set ?
			clock_bit(1);
		else	clock_bit(0);
	}
	clocksr();
}




void clock_fbline(unsigned char *ptr, int scanline, int numpixels)
{
	int l;
	unsigned int i=0;
	//unsigned char b;
	int c=0;
	int p,x;
	int color=0;

	int offset=scanline*(LED_DISPLAY_WIDTH+1);

	output_low(COUNTER_CLOCK);						// LEDs off
	usleep(GAURD_TIME_US);

	x=0;
	for (p=0;p<PANELS;p++)
	{
		c=0;
		for (l=0;l<30;l++)
		{
			color=*(ptr+offset);
			if (color==1)
				clock_bit(1);
			else	clock_bit(0);
			x++;
			offset++;
		}
		i=scanline;
		i=~i;
		clock_byte(i);
	}

	output_high(COUNTER_CLOCK);						// LEDs on
}





int main(int argc, char **argv)
{
	//char st[1024];	
        //int clockseconds,pclockseconds;
	uint16_t sid;
        int i;
	//int l;
	int s=0;
	int row,col;
        //int scans=1;
        int sloweventcount;
        unsigned long int scrollpos=0;
        unsigned long int scrollposmax=3000;
        time_t   filemodtime;
        time_t   pfilemodtime;
	int p=-1;
	unsigned char mymac[7];
	char vers[64];

        srand(time(NULL));
        sid=rand();

        // ****************************************************************************************
        char cmstring[1024];
        // Parse command line options and set flags first
        int op_help=FALSE;
        strcpy(cmstring,"-h");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        strcpy(cmstring,"--h");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        strcpy(cmstring,"-help");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        strcpy(cmstring,"--help");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
                op_help=TRUE;
        if (op_help==TRUE)
        {
                printf("%s <Options>\n",PROGNAME);
                printf("-q\t\tTurn LED sign off and exit\n");
                printf("-f\t\tSpecify filename for text file, if omitted /tmp/message.txt is used\n");
                exit(0);
        }



        strcpy(cmstring,"-f");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
        {
                int argnum;
                argnum=parse_findargument(argc,argv,cmstring);                                  /// find the -f string in argument list
                if ((argc-1>=argnum) & (argnum>0))                                              // if argument list isnt short
                {
                        strcpy(text_filename,argv[argnum]);                                     // then its our new workging value
                        if (strlen(text_filename)<1)
                        {
                                printf("-f <full path/filename>\n");
                                exit(1);
                        }
                }
                else    strcpy(text_filename,"/tmp/message.txt");
        }
        else
                strcpy(text_filename,"/tmp/message.txt");




        // ****************************************************************************************
	// Setup GPIO and exit handler
        setup_io();
	set_realtime();
	signal(SIGINT,exithandler);
	signal(SIGTERM,exithandler);
	set_as_output(COUNTER_CLOCK);
	set_as_output(SR_DATA);
	set_as_output(SR_CLOCK);
	output_low(COUNTER_CLOCK);								// LEDs off
        strcpy(cmstring,"-q");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)					// Turns leds off and exit
	{
		exit(0);
	}

        // ****************************************************************************************
	// jlc registration
        getmac("eth0", (char*)&mymac);
        printf("%s:\tMAC %s\n",PROGNAME,printuid((unsigned char*)&mymac));

	sprintf(vers,"%s",VERSIONT);
	p=jcp_linux_udplisten();								// opens listening socket
	jcp_init_b((char*)&mymac, sid, p, JLP_SERVER_PORT, (1000/JCP_TIMER_FREQ_HZ), "LXDB", (char*)&vers );
	jcp_register_dev(0, DEVN_LEDSIGN ,0, "0000000000000000");				// text based LED sign



        // ****************************************************************************************
	scroll=TRUE;
	printf("Ferrograph single line 120x7 LED Sign Ver:%s\n",VERSIONT); fflush(stdout);
        // load text file and render it to long buffer or load error text
        if (loadtextfile(text_filename)!=TRUE)
		scrollpos=24*5;									// initially skip leading spaces
        image_long_width=sometext_length*(FONTSMALL_WIDTH+1);
        image_long_size=image_long_width*(FONTSMALL_HEIGHT+1);
        image_long=malloc(image_long_size*2);	// Add some extra
        if (image_long==0)
        {
                printf("Cant malloc image_long\n");
                exit(1);
        }

        printf("Rendering very very long bitmap with text in it\n");
        bzero(image_long,image_long_size);
        scrollposmax=render_text_to_buffer_small(sometext, sometext_length, image_long, image_long_size, image_long_width , 7, TRUE, 1);
        printf("Done, bitmap is %d pixels wide\n",(int)scrollposmax);




//bzero(&framebuf,sizeof(framebuf));
//sprintf(st,"0123456789ABCDEFGHIJK");
//render_text_to_buffer_small(st,strlen(st),(unsigned char*)&framebuf,sizeof(framebuf), LED_DISPLAY_WIDTH+1 , 7, TRUE, 1);


        pfilemodtime=get_filemodtime(text_filename);                                            // first time we call this is dummy, next time is for real
        filemodtime=pfilemodtime;


        printf("Scrolling now\n"); fflush(stdout);
        sloweventcount=0;
        while (1)
        {
                scrolltextcopy(image_long,image_long_width,LED_DISPLAY_HEIGHT,image_data,LED_DISPLAY_WIDTH+1,7,0,scrollpos);

		if (reloading >0)
		{
			printf("reloading=%d\n",reloading); fflush(stdout);
			for (row=0;row<LED_DISPLAY_HEIGHT;row++)
			{
				for (col=reloading;col<LED_DISPLAY_WIDTH;col++)
					putpixel(image_data, LED_DISPLAY_WIDTH+1, LED_DISPLAY_HEIGHT+1, row, col, 0);
			}

			reloading--;
			if (reloading==0)							// counter just expired
			{
				output_low(COUNTER_CLOCK);					// LEDs off
                                loadtextfile(text_filename);
                                image_long_width=sometext_length*(FONTSMALL_WIDTH+1);
                                image_long_size=image_long_width*(FONTSMALL_HEIGHT+1);
                                free(image_long);
                                image_long=malloc(image_long_size);
                                if (image_long==0)
                                {
                                        printf("Cant malloc image_long\n");
                                        exit(1);
				}
                                bzero(image_long,image_long_size);
        			scrollposmax=render_text_to_buffer_small(sometext, sometext_length, image_long, image_long_size, image_long_width , 7, TRUE, 1);
                                printf("Done, bitmap is %u pixels wide\n",(unsigned int)scrollposmax);
				scroll=TRUE;
				scrollpos=0;
			}
		}



		for (s=0;s<scans;s++)								// number of time to scan frame
		{
			for (i=0;i<LED_DISPLAY_HEIGHT;i++)					// clock entire display at least once
			{
				clock_fbline((unsigned char*)image_data,i,LED_DISPLAY_WIDTH);
				usleep(ON_TIME_US);						// Keeps LEDs on for a bit
			}
	        	output_low(COUNTER_CLOCK);						// Turn LEDs off
			usleep(ON_TIME_US*LED_DISPLAY_HEIGHT);					// Keeps LEDs off for same durartion as on
		}

		if (scroll!=FALSE)
                	scrollpos++;
                if (scrollpos>scrollposmax)
                        scrollpos=0;


                sloweventcount++;
                if (sloweventcount>65)
                {
                        sloweventcount=0;

                        filemodtime=get_filemodtime(text_filename);
                        if (filemodtime!=pfilemodtime)
			{
                               	pfilemodtime=filemodtime;
                                printf("file [%s] changed, reloading it soon\n",text_filename); fflush(stdout);
				reloading=LED_DISPLAY_WIDTH;					// start counter
				// Blank text to the right of where we are
				// need to test if close to buffer edge
				//for (row=0;row<LED_DISPLAY_HEIGHT;row++)
					//memset(image_long+scrollpos+(image_long_width*row)+LED_DISPLAY_WIDTH,0,LED_DISPLAY_WIDTH);
			}
                }
	jcp_poll();
	}
}




