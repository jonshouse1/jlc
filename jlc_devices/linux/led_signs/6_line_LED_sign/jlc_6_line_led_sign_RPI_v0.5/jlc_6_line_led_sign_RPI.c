// DAM I/O Version of 6 line LED sign code.
//
// This version uses a common RCK and SRCK for both panel sets, hardware changed
//
// The display is driven as two sets of 2 pannel, each of 3 lines of text
// At the moment an entire line is being updated before moving to the next line.
//
// A complete "frame" is all the dots scanned, but as only one of 3 lines is
// visible at any one point the true "per line" frame rate is always 1/3rd the fps
// -d shows frame rate as "Scans of the display (refresh rate per line of text)
// specify a huge value of -op to see scan pattern
//
// PIGPIO changed to 2Mhz DMA I/O Rather than 1Mhz 
//
// 	clkDivI = 50 * micros; /* 10      MHz - 1      MHz */
// To:  clkDivI = 25 * micros; /* 10      MHz - 1      MHz */
//


char PROGNAME[80]="jlc_6_line_led_sign_RPI";
char VERSIONT[20]="0.4";
char text_filename[2048];

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>
#include "pigpio.h"
#include "sign_common.c"

#include <errno.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <arpa/inet.h>


#include "jcp-client-linux.h"
#include "jcp_protocol_devs.h"
#include "jcp_client.h"
#include "jlc.h"

char textline[6][64];


// PIGPIO traps signals, for debugging sometimes it is better to build without it
//#define FAKEPIGPIO								// uncomment to fake it up
#ifdef FAKEPIGPIO
	int gpioSetMode(unsigned gpio, unsigned mode)  {  }
	int gpioWaveTxStop(void) { }
	void gpioTerminate(void) { }
	int gpioWrite_Bits_0_31_Set(uint32_t bits) { }
	int gpioWrite_Bits_0_31_Clear(uint32_t bits) { }
	int gpioInitialise(void) { }
#endif 


#define DMAIO_TABLESIZE 12000							// Max possible
gpioPulse_t pulse[DMAIO_TABLESIZE*100];						// A wave table

// Two panels are defined here, one side of the GPIO header per pair, top set S1, bot set S2
#define DATA_S1		18							// SM6B595 shift register data 
#define DATA_S2		9
#define SRCK		15							// clock, latch on rising edge
#define RCK		14
#define SWITCHSET_S1	24							// 74HC138 deocder -enable (LED ON)
#define SWITCHSET_S2	27							// 74HC138 deocder -enable (LED ON)
#define DECODER_A_S1	8							// 74HC138 input A
#define DECODER_A_S2	4							// 74HC138 input A
#define DECODER_B_S1	23
#define DECODER_B_S2	3
#define DECODER_C_S1	25
#define DECODER_C_S2	22
#define ENABLE		2

#define LED_ON_PERIOD_US	100
//#define LED_OFF_PERIOD_US	50


#define LED_LINES		6						// 6 lines total
#define LED_LINES_PER_BOARD	3
#define LED_DISPLAY_WIDTH	60 * 2					
#define LED_DISPLAY_HEIGHT	7						// of 7 leds high
unsigned char framebuf[LED_LINES][LED_DISPLAY_HEIGHT][LED_DISPLAY_WIDTH]; 	// one byte per pixel
int scanline=0;
char st[2048];	
int rotate=FALSE;								// Set true to use display the other way up
int op=500;									// LED Off period in us, 0=brightest, 1000=dimmest
int upper=FALSE;
int prop=TRUE;									// proportional TRUE or FALSE
int deb=FALSE;

int flag_asciiart=FALSE;
unsigned char image_data[LED_DISPLAY_HEIGHT * LED_DISPLAY_WIDTH *2];



// These 3 tables describe how the display is scanned, as viwed from LED side two 74HC183 decoders
// operate to select multiple columns of 7 pixels per column
// When the 3rd line is selected the SR data has some gaps in it
//NAME  SWITCHSET       decoder         Line(0,1 or2)   First Col lit  (1 to 5)
//A	0               0               0               1
//B	0               1               0               2
//C	0               2               0               3
//D	0               3               0               4
//E	0               4               0               5

//F  	0               5               1               1
//G     0               6               1               2
//H     0               7               1               3
//I     1               0               1               4
//J     1               1               1               5

//K     1               2               2               1
//L     1               3               2               2
//M     1               4               2               3
//N     1               5               2               4
//O     1               6               2               5
//      1               7               OFF

#define SR_BITS			8 * 12 *2					// 12 shift registers per board, each of 8 bits = 192
#define SCANCOMBINATIONS	15
//              B C D E F G H I J K L M N O A  (Read the pattern top to bottom)
//int so_sws[]={0,0,0,0,0,0,0,1,1,1,1,1,1,1,0};					// switchset, 0=top decoder, 1=bot
//int so_dec[]={1,2,3,4,5,6,7,0,1,2,3,4,5,6,0};					// 3 bit values for 3to8 line decoder
//int so_col[]={4,3,2,1,5,4,3,2,1,5,4,3,2,1,5};
//int so_tl[] ={0,0,0,0,1,1,1,1,1,2,2,2,2,2,0};					// scanning order text line 0=top, 1=middle 2=bot


// New scan order, one column of each of line in turn IE col1 or line 1, col1 of line 2 etc
//            A F K B G L C H M D I N E J O
int so_sws[]={0,0,1,0,0,1,0,0,1,0,1,1,0,1,1};					// switchset, 0=top decoder, 1=bot
int so_dec[]={0,5,2,1,6,3,2,7,4,3,0,5,4,1,6};					// 3 bit values for 3to8 line decoder
int so_col[]={5,5,5,4,4,4,3,3,3,2,2,2,1,1,1};
int so_tl[] ={0,1,2,0,1,2,0,1,2,0,1,2,0,1,2};					// scanning order text line 0=top, 1=middle 2=bot



void delay_ms(int m)
{
	usleep(m*1000);
}

void gpio_output_high(int g)
{
	gpioWrite_Bits_0_31_Set( (1<<g) );
}

void gpio_output_low(int g)
{
	gpioWrite_Bits_0_31_Clear( (1<<g) );
}

void gpio_set_bit(int g, int b)
{
	if (b==1)
		gpio_output_high(g);
	else	gpio_output_low(g);
}

// Set the 3 to 8 line decoder
inline void gpio_set_decoders(int d)
{
        gpio_set_bit(DECODER_A_S1,((d & (1<<0)) >>0));
        gpio_set_bit(DECODER_A_S2,((d & (1<<0)) >>0));
        gpio_set_bit(DECODER_B_S1,((d & (1<<1)) >>1));
        gpio_set_bit(DECODER_B_S2,((d & (1<<1)) >>1));
        gpio_set_bit(DECODER_C_S1,((d & (1<<2)) >>2));
        gpio_set_bit(DECODER_C_S2,((d & (1<<2)) >>2));
}

void gpio_clock_sr()
{
        gpio_output_high(SRCK);
	usleep(1);
        gpio_output_low(SRCK);
}

void gpio_latch_sr()
{
        gpio_output_low(RCK);
	usleep(1);
        gpio_output_high(RCK);
	usleep(1);
        gpio_output_low(RCK);
}


inline void leds_off()
{
	int i;
        gpio_set_bit(SWITCHSET_S1,1);
        gpio_set_bit(SWITCHSET_S2,1);
        gpio_set_decoders(7);
	gpio_output_low(DATA_S1);
	gpio_output_low(DATA_S2);
	for (i=0;i<12*8*2;i++)
		gpio_clock_sr();
	gpio_latch_sr();
}


void exithandler()
{
	gpioWaveTxStop();								// Stop DMA I/O
        leds_off();
	gpio_output_high(ENABLE);							// disable 74HCT245s
   	gpioTerminate();
	printf("With luck LEDs are off\n"); fflush(stdout);
        exit(0);
}

void bitmap_to_ascii(unsigned char*pixels, int bmwidth, int bmheight, int displaywidth)
{
	int w,h;	

	for (h=0;h<bmheight;h++)
	{
		for (w=0;w<displaywidth;w++)
		{
			if (pixels [w+(h*bmwidth)] >0)
				printf("*");
			else	printf(" ");
			fflush(stdout);
		}
		printf("\n"); fflush(stdout);
	}
}


// If the display is rotated then the lines are in the opposite order
// gln returns a line number adjusted for rotation
unsigned int gln(int lineno)
{
	int ln=0;
	if (rotate==TRUE)
		ln=5-lineno;
	else	ln=lineno;
	return(ln);
}


unsigned char getpixel(unsigned char*pixels, int bmwidth, int bmheight, int row, int col)
{
	if (rotate!=TRUE)
	{
		if (pixels [col+(bmwidth*row)] >0)
			return(1);
		else	return(0);
	}
	if (pixels [(bmheight*bmwidth)-1-(col+(bmwidth*row))] >0)
		return(1);
	else	return(0);
}





// For DMAIO only
// Add a low output to DMA I/O table
inline void output_low(int *idx, int gpio, int duration_us)
{
	pulse[*idx].gpioOn = 0;
	pulse[*idx].gpioOff = (1<<gpio);
	pulse[*idx].usDelay = duration_us;
	*idx=*idx+1;
}

inline void output_high(int *idx, int gpio, int duration_us)
{
	pulse[*idx].gpioOn = (1<<gpio);
	pulse[*idx].gpioOff = 0;
	pulse[*idx].usDelay = duration_us;
	*idx=*idx+1;
}

// Set GPIO line g to state b
inline void set_bit(int *idx, int g, int b, int t)
{
	if (b==1)
		output_high(idx,g,t);
	else	output_low(idx,g,t);
}


// Clocks shift registers, both sets
inline void clock_sr(int *idx,int t)
{
        output_high(idx,SRCK,t);
        output_low(idx,SRCK,t);
}

inline void latch_sr(int *idx, int t )
{
        output_high(idx,RCK,t);
        output_low(idx,RCK,t);
}

inline void set_decoders(int *idx, unsigned char d, int t)
{
	//printf("d=%d  %d %d %d\n",d,((d & (1<<0)) >>0),((d & (1<<1)) >>1),((d & (1<<2)) >>2)); fflush(stdout);
        set_bit(idx,DECODER_A_S1,((d & (1<<0)) >>0),0);
        set_bit(idx,DECODER_B_S1,((d & (1<<1)) >>1),0);
        set_bit(idx,DECODER_C_S1,((d & (1<<2)) >>2),0);

        set_bit(idx,DECODER_A_S2,((d & (1<<0)) >>0),0);
        set_bit(idx,DECODER_B_S2,((d & (1<<1)) >>1),0);
        set_bit(idx,DECODER_C_S2,((d & (1<<2)) >>2),0);	
        set_bit(idx,DECODER_C_S2,((d & (1<<2)) >>2),t);					// duplicate output, hack for dwell time
}


// Set DATA_S1 and DATA_S2
inline void setpixels(int *idx, int p2, int p1)
{
	pulse[*idx].usDelay = 0;
	pulse[*idx].gpioOn = 0;
	if (p1==1)
		pulse[*idx].gpioOn = pulse[*idx].gpioOn | (1<<DATA_S1);
	if (p2==1)
		pulse[*idx].gpioOn = pulse[*idx].gpioOn | (1<<DATA_S2);

	pulse[*idx].gpioOff = 0;
	if (p1==0)
		pulse[*idx].gpioOff = pulse[*idx].gpioOff | (1<<DATA_S1);
	if (p2==0)
		pulse[*idx].gpioOff = pulse[*idx].gpioOff | (1<<DATA_S2);
	*idx=*idx+1;
}



void rerender(int line)
{
	char st[1024];
	int i=0;

	bzero(&st,sizeof(st));
	strcpy(st,textline[line]);
	if (upper==TRUE)
	{
		for (i=0;i<strlen(st);i++)
			st[i]=toupper(st[i]);
	}
	bzero(&framebuf[line],sizeof(framebuf[line]));
	render_text_to_buffer_small(st,strlen(st),(unsigned char*)&framebuf[line],sizeof(framebuf[line]), LED_DISPLAY_WIDTH, LED_DISPLAY_HEIGHT, prop, 1);
}


void render()
{
	int i=0;

	for (i=0;i<6;i++)
		rerender(i);
}


// Scan column on two panels top and in parallel two panels bot
int scan(int i)
{
	//int p=0;
	int idx=0;
	int sr=0;
	int row=0;
	int x;
	int p1,p2;
	int col=0;
	int textline=0;

	set_bit(&idx,DATA_S1,0,0);
	set_bit(&idx,DATA_S2,0,0);						// Turn LEDs off
	for (x=0;x<24*8;x++)
		clock_sr(&idx,0);
	latch_sr(&idx,0);							// All shift register bits to off
	set_bit(&idx,DATA_S2,0,op);						// unimportant bit set but gives us the delay (op)


	col=120-so_col[i];
	for (sr=23;sr>=0;sr--)							// two boards worth of shift registers, 12 per board
	{
		for (row=7;row>=0;row--)					// pixels per shift reg, row 0 is missing (7 LEDs only)
		{
			if (row!=7)
			{
                                textline=so_tl[i];
                                p1=getpixel((unsigned char *)&framebuf[gln(textline)], LED_DISPLAY_WIDTH, LED_DISPLAY_HEIGHT, row, col);
                                p2=getpixel((unsigned char *)&framebuf[gln(textline+3)], LED_DISPLAY_WIDTH, LED_DISPLAY_HEIGHT, row, col);
				setpixels(&idx,p1,p2);
			}
			clock_sr(&idx,0);					// Clock in each shift register bit
		}
		col=col-5;
	}

	// These values are 'future' values suitable for when we next re-enter scan()
	set_bit(&idx,SWITCHSET_S1,so_sws[i],0);					// Which display half is active
	set_bit(&idx,SWITCHSET_S2,so_sws[i],0);
	set_decoders(&idx,so_dec[i],0);						// which col is active, 74HC168 is slow
	latch_sr(&idx,0);							// Now we have moved the columns turn new pixels on
	return(idx);								// LEDs are now on, new pattern holds for the usleep in main()
}





// Server is pushing state at us.
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
	int v=0;
        if (idx==-100)                                                          // registered with server now
                return;
        printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\tds->valuebyteslen=%d\n",idx,ds->value1, ds->value2, ds->valuebyteslen);


	// Feature, reset to defaults, clear text
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


	if (ds->value1>=1 && ds->value1<=6)					// value1 is lines 1 to 6 inclusive 
	{
		v=ds->value1-1;
		printf("%s v=%d Set LINE %d to [%s]\n",PROGNAME, v, ds->value1, ds->valuebytes);
		bzero(&textline[v],sizeof(textline[v]));			// clear all text
		strcpy(textline[v],ds->valuebytes);
		rerender(v);
	}
}

void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}


// Server sends us config info, we can ignore this on linux
void jcp_dev_config(char *dt, int len)
{
}


int main(int argc, char **argv)
{
	int fps=0;
	int cursec=0;
	int pcursec=0;
	int loops=0;
        int i;
	int x=0;
	char vers[64];

	uint16_t sid;
	unsigned char mymac[7];
	int p=JLP_SERVER_PORT+1;

	int wave_id;
	int tableelements=0;
	int argnum=0;


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
                printf("\t-q\tTurn LED sign off and exit\n");
		printf("\t-r\tRotate the sign\n");
		printf("\t-op\tLED Off period (us), 0=brightest, 500=dimmer, values impact frame rate\n");
		printf("\t-d\tDebug, use bottom line for debug info\n");
                exit(0);
        }

        strcpy(cmstring,"-d");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		deb=TRUE;

        strcpy(cmstring,"-r");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
		rotate=TRUE;


	strcpy(cmstring,"-op");
	if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
	{
		argnum=parse_findargument(argc,argv,cmstring);					// find the -f string in argument list
		if (argnum>0)									// if argument list isnt short
		{
			strcpy(st,argv[argnum]);
			if (strlen(st)<1)
			{
				printf("-op <value>\n");
				exit(1);
			}
			else	op=atoi(st);
		}
	}

        strcpy(cmstring,"-f");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)
        {
		argnum=parse_findargument(argc,argv,cmstring);					// find the -f string in argument list
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
        else	strcpy(text_filename,"/tmp/message.txt");


	for (i=0;i<6;i++)
		bzero(textline[i],sizeof(textline[i]));


        // ****************************************************************************************
	// Setup GPIO and exit handler
   	if (gpioInitialise()<0) return -1;
	gpioSetMode(DATA_S1,PI_OUTPUT);
	gpioSetMode(DATA_S2,PI_OUTPUT);
	gpioSetMode(SRCK,PI_OUTPUT);
	gpioSetMode(RCK,PI_OUTPUT);
	gpioSetMode(SWITCHSET_S1,PI_OUTPUT);
	gpioSetMode(SWITCHSET_S2,PI_OUTPUT);
	gpioSetMode(DECODER_A_S1,PI_OUTPUT);
	gpioSetMode(DECODER_A_S2,PI_OUTPUT);
	gpioSetMode(DECODER_B_S1,PI_OUTPUT);
	gpioSetMode(DECODER_B_S2,PI_OUTPUT);
	gpioSetMode(DECODER_C_S1,PI_OUTPUT);
	gpioSetMode(DECODER_C_S2,PI_OUTPUT);
	gpioSetMode(ENABLE,PI_OUTPUT);

	set_realtime();
	signal(SIGINT,exithandler);
	signal(SIGTERM,exithandler);


        strcpy(cmstring,"-q");
        if (parse_commandlineargs(argc,argv,cmstring)==TRUE)					// Turns leds off and exit
	{
		leds_off();
		exit(0);
	}
        //pfilemodtime=get_filemodtime(text_filename);                                            // first time we call this is dummy, next time is for real
        //filemodtime=pfilemodtime;



	printf("6 line LED Sign Ver:%s\n",VERSIONT); fflush(stdout);
        // ****************************************************************************************
	// jlc registration
        getmac("eth0", (char*)&mymac);
        printf("%s:\tMAC %s\n",PROGNAME,printuid((unsigned char*)&mymac));

	sprintf(vers,"%s",VERSIONT);
	p=jcp_linux_udplisten();								// opens listening socket
	jcp_init_b((char*)&mymac, sid, p, JLP_SERVER_PORT, (1000/JCP_TIMER_FREQ_HZ), "LXDB", (char*)&vers );
	jcp_register_dev(0, DEVN_LEDSIGN ,0, "0000000000000000");				// text based LED sign



        // ****************************************************************************************
	bzero(&framebuf,sizeof(framebuf));

	sprintf(textline[0],"6lnsign %s",VERSIONT);
	textline[1][0]=0;
	textline[2][0]=0;
	textline[3][0]=0;
	textline[4][0]=0;
	textline[5][0]=0;
	//sprintf(textline[0],"The pigpio library lets me");
	//sprintf(textline[1],"drive all these dots without");
	//sprintf(textline[2],"glitching.");
	//sprintf(textline[3],"The true update rate in Hz");
	//sprintf(textline[4],"is shown in brackets below");
	//sprintf(textline[5],"F This is the sixth line. ");
	render();

	//bitmap_to_ascii((char*)&framebuf[0][0][0], LED_DISPLAY_WIDTH, LED_DISPLAY_HEIGHT, 60);
	int row,col;
        for (row=0;row<7;row++)
        {
                for (col=0;col<120;col++)
                {
                        if (getpixel((unsigned char *)&framebuf[5], LED_DISPLAY_WIDTH, LED_DISPLAY_HEIGHT, row, col) >0 )
                                printf("*");
                        else    printf(" ");
                }
                printf("\n");
        }
        fflush(stdout);



	gpio_output_low(ENABLE);									// enable 74HCT245s
	while(1)
	{
		for (i=0;i<SCANCOMBINATIONS;i++)							// scan entire display
		{
			tableelements=scan(i);
			if (gpioWaveAddGeneric(tableelements,pulse)==PI_TOO_MANY_PULSES)
			{
				printf("PI_TOO_MANY_PULSES %d\n",loops);
				exithandler();
			}
			wave_id=gpioWaveCreate();
			if (wave_id >= 0)
        			x=gpioWaveTxSend(wave_id, PI_WAVE_MODE_ONE_SHOT);			// Send this bit pattern once via DMA IO
			x=x;										// stop compiler warning..
			while (gpioWaveTxBusy()==1) { usleep(1); }					// wait for DMA I/O to complete
			gpioWaveDelete(wave_id);							// unhook previous DMA I/O buffer
			usleep(5);									// Must sleep here or machine in unreliable
		}
		loops++;

		cursec=get_clockseconds();
		if (cursec!=pcursec)									// Once a second
		{
			pcursec=cursec;
			fps=loops;
			loops=0;
		}

		jcp_poll();
	}
}


