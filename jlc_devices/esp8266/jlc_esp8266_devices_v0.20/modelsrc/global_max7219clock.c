// global_max7219clock.c
#pragma message ( "Building for MAX7219CLOCK" )


// Prototypes
char* ICACHE_FLASH_ATTR ipsta();


// LEDs
int wifi_led = 14;



// 4 controllers giving a display of 32x8 pixels
#define DISP_WIDTH	32
#define DISP_HEIGHT	8
__attribute__((aligned(32))) unsigned char disp[DISP_HEIGHT][DISP_WIDTH];
int suppress_leading_zero=FALSE;						// ISO_8601 says we should have the leading zero
int twelve_hour=FALSE;								// Default is 24Hr
int alignbot=TRUE;								// Move display down and blank the top line
int blink=0;									// 0=no flash, 1=fast flash, 2=slower.. server sets this	
int flashing=0;

int bright=0;
int pbright=-1;
char datetime[35];

int popupmsg=0;									// counter, counts down to 0
char popupmsg_string[32];

static const char *wday_name[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
int validtime=FALSE;
unsigned char lunarphase=0;							// To be arranged one day, maybe ...
unsigned char dow=0;								// Day of the week 0=Sun
unsigned char dom=0;
unsigned char mon=0;
int year=0;
unsigned char hh=12;
unsigned char mm=0;
unsigned char ss=0;
char message[1024];
int displaymessage=0;
int ndc=0;									// no data counter

static volatile os_timer_t fast_timer;
static volatile os_timer_t slow_timer;



// Server is sending us new state.  idx is the index used when regisering the device. 
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
	if (idx==-100)								// special message from jcp_client, registered
		return;
        os_printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);

	// Value1 is the command, value2 is the value
	// value1	value2 
	//  1		0 to 15		Set brightness
	//  2		1 to lots	Display pop up message for duration(seconds) value2
	//  3		0,1 or 2	Set blink :   0=NONE, 1=fast, 2=slow, all other values 0
	//  4		duration	flash display for value2 duration (seconds)
	if (ds->value1==0 && ds->value2==0)
		return;
	if (idx==0)
	{
		switch (ds->value1)
		{
			case 1:
				bright=ds->value2;
			break;
		
			case 2:
				strcpy(popupmsg_string,ds->valuebytes);
				popupmsg=ds->value2*2;
			break;

			case 3:
				if (ds->value2>=0 && ds->value2<=2)
					blink=ds->value2;
				else	blink=0;
			break;

			case 4:
				flashing=ds->value2;
			break;
		}
	}
	else os_printf("idx=%d ? Is this message meant for me ?\n",idx);
}


void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
	os_printf("got topic=%d len=%d\n", topic, tlen);
	if (topic == JCP_TOPIC_DATETIME_S_M)
	{
		os_printf("topic is JCP_TOPIC_DATETIME_S_M [%s]\n",tdata);
		if (tlen>0)
		{
			bzero(&datetime, sizeof(datetime));
			memcpy(datetime, tdata, tlen);
			validtime=TRUE;
			ndc=0;											// no data counter
		}
	}
}


void ICACHE_FLASH_ATTR connected()
{
	build_send_regiserdevs();
}


void ICACHE_FLASH_ATTR app_suspend_timers()
{
	os_printf("app_suspend_timers()\n");
	jcp_suspend_timer();
	os_timer_disarm(&slow_timer);
}




//*********************************************************************************************************************************
// Render small text into one byte per pixel framebuffer
// d        move everything right, 0=normal alignment
// *TEXT    string
// *destfb  Pointer to one byte per pixel frame buffer
// width    Width of framebuffer
// height   Height of framebuffer
// proportional   TRUE or FALSE
//	    if TRUE characters are moved left as rendered to occupy differing widths, an "l" for example takes less
//          pixels then an "m"
//
// returns the real number of pixels rendered for a line of text

//#include <strings.h>
//#include <string.h>

// On the Pi board this burns only a few % more CPU cycles
#define BUFFERTEST									// uncomment this if you get segfaults

#include "fontsmall.h"
unsigned long int ICACHE_FLASH_ATTR render_text_to_buffer_small(int d,char *TEXT,unsigned long int textlength,unsigned char *destfb, unsigned long int fbsize,unsigned long int width, int height, int proportional, int color)
{
	int			i,x;
        unsigned char  		thechar;
        unsigned char     	*ptr;
        unsigned int		offset;							// Added to pointer to access frame buffer (destfb)
	unsigned long int	c;
        unsigned long int	cwidth;
	long int 		scrollwidth;

	scrollwidth=0;
        ptr=destfb;  									// Set ptr to start of this scan line
        for (x=0;x<FONTSMALL_HEIGHT;x++)                                                // for every line
        {
		offset=width*x;								// start of next line
                for (c=0;c<textlength;c++)                                              // for every character accross the line
                {
			if (TEXT[c]<=FONTSMALL_LASTCHAR && TEXT[c]>=FONTSMALL_FIRSTCHAR)
                        	thechar=TEXT[c]-' ';					// remove any ASCII not in our font
			else	thechar=' ';
                        if (proportional == TRUE)
                                cwidth=atoi(fontsmall[thechar][0]);                     // Get the width of this character from font definition
                        else    cwidth=FONTSMALL_WIDTH-1;
			//printf("[%s]  TEXT[c]=%c(%d) cwidth=%d\n",fontsmall[thechar][0],TEXT[c],TEXT[c],cwidth); fflush(stdout);
			//if (cwidth==0)
				//exit(0);

                        for (i=0;i<cwidth;i++)                                          // for every pixel of the character
                        {
                                // [The character, 0=A] [9 lines] [of 9 pixels(chars)]
                                if (fontsmall[thechar][x+1][i]=='*')
                                        ptr[offset+d]=color;
                                else    ptr[offset+d]=0;
                                offset++;
				scrollwidth++;
                        }
                        offset++;       						// character spacing, one pixel
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



// Output the pixel buffer as plain text
void asciifb()
{
	int lines;
	int pixels;

	os_printf("\n");
	for (lines=0;lines<8;lines++)
	{
		for (pixels=0;pixels<32;pixels++)
		{
			if (disp[lines][pixels]==1)
				os_printf("*");
			else	os_printf(" ");
		}
		os_printf("\n");
	}
	os_printf("\n");
}


// Update entire display
void update_display()
{
	int nc=0;
	int lines=0;
	int pixels;
	int bytes=0;
	unsigned char b[4];
	int p=0;


	for (nc=0;nc<4;nc++)											// num of controllers
	{
		for (lines=0;lines<DISP_HEIGHT;lines++)								// for each line of leds
		{
			// Pack 32 pixels into 4 bytes
			for (bytes=0;bytes<4;bytes++)								// 4 bytes per line of pixels
			{
				b[bytes]=0;
				for (pixels=7;pixels>=0;pixels--)
				{
					p=8-(pixels+1); 
					b[bytes] |= disp[lines][p+(bytes*8)]<<pixels;				// turn bytes into bits
				}	
			}
			// Send one line of pixel data.  Address (1 to 8) followed by byte
//JA
#ifndef CLOCKB
			max_addto_seq(lines+1);	
			max_addto_seq(b[0]);
			max_addto_seq(lines+1);	
			max_addto_seq(b[1]);
			max_addto_seq(lines+1);
			max_addto_seq(b[2]);
			max_addto_seq(lines+1);
			max_addto_seq(b[3]);
#else
			max_addto_seq(lines+1);	
			max_addto_seq(b[3]);
			max_addto_seq(lines+1);	
			max_addto_seq(b[2]);
			max_addto_seq(lines+1);
			max_addto_seq(b[1]);
			max_addto_seq(lines+1);
			max_addto_seq(b[0]);
#endif

			max_send_seq();
		}
	}
}


void display_text(char *st, int moveright)
{
	int i;
	int l;

	bzero(&disp,sizeof(disp));
	render_text_to_buffer_small(moveright, st,strlen(st), (char*)&disp[0],sizeof(disp), 32, 8, TRUE, 1);

	if (alignbot==TRUE)								// Move display down one line and clear the bot line
	{
		for (l=7;l>=0;l--)
		{
			for (i=0;i<31;i++)
			{
				disp[l][i]=disp[l-1][i];
			}
		}
		for (i=0;i<32;i++)
			disp[0][i]=0;										// clear top line
	}
	update_display();
}



// sscanf seens to eat all our RAM, do it this way instead
int extractnumber(char *st, int stlen)
{
	char buf[16];
	int i=0;

	bzero(&buf,sizeof(buf));
	for (i=0;i<stlen;i++)
		buf[i]=st[i];
	return(atoi(buf));
}



// 500ms
void slow_timer_cb()
{
	char st[32];
	int i=0;
	static int onoff=0;
	static int alternate=0;
	static int alt=0;

	if (bright!=pbright)										// brightness changed ?
	{
		pbright=bright;
		max_setbright(bright);
	}

	alt=!alt;
	if (alt==1)											// once a second 
	{
		os_printf("IP:%s bright=%d ndc=%d\n",ipsta(),bright,ndc);
		if (ndc> 60 * 3)									// timeout in seconds
			validtime=FALSE;								// then displayed time is no longer valid
		else	ndc++;
	}

	if (popupmsg>0)
	{
		popupmsg--;
		display_text(popupmsg_string,0);
		return;
	}

	if (validtime!=TRUE)										// time we have is not current
		os_sprintf(st,"??:??",hh,mm);
	else	
	{
		dow=extractnumber(datetime, 2);
		hh=extractnumber(datetime+13, 2);
		mm=extractnumber(datetime+16, 2);
		i=hh;
		if (twelve_hour==TRUE)									// Default is 24Hr
		{
			if ( (i>=13) & (i<=23) )
				i=i-12;									// Adjust displayed value to 12hr clock
			suppress_leading_zero=TRUE;							// always squash leading 0 for 12hr clock
		}

		os_sprintf(st,"%02d:%02d",i,mm);
		if ( (suppress_leading_zero==TRUE) & (st[0]=='0') )
			st[0]=' ';
	}

	switch (blink)
	{
		case	0:
			onoff=1;									// No flash, just display :
		break;

		case 	1:										// 500 ms on/off
			onoff=!onoff;									// half second on/off
		break;

		case	2:										// one second on/off
			alternate++;
			if (alternate>1)
			{
				alternate=0;
				onoff=!onoff;	
			}
		break;	
	}
	if (onoff==0)
		st[2]=' ';

	// Overwrite clock display with a pop up message
	if (displaymessage>0)
	{
		displaymessage--;
		strcpy(st,message);
		display_text(st,0);
		return;
	}
	if ( (flashing>0) & (alt==1) )
	{
		flashing--;										// count down towards 0 (inactive)
		bzero(&st,sizeof(st));
	}
	display_text(st,2);										// normal clock display, move it right a bit
}


