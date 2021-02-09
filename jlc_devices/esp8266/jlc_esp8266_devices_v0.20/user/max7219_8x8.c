/*
	Driver for max7219_8x8 LED driver(s)

	To use call max_setgpio() to speicify the GPIO pins.
		call max_init() for each cascaded display 
		call max_send_sequence() to send initialisation data to modules
*/

#define MDELAY	1000

#include <os_type.h>
#include <osapi.h>
#include <gpio.h>

// CCS Style GPIO
#define output_low(g)   GPIO_OUTPUT_SET(g,0);
#define output_high(g)  GPIO_OUTPUT_SET(g,1);
#define output_bit(g,n)	GPIO_OUTPUT_SET(g,n);

unsigned char sq[1024];

static int mpin_data=-1; 
static int mpin_clock=-1; 
static int mpin_chipsel=-1;
static int mbanks=4;
static int mseq=0;
static int mnc=4;						// number of controllers (ICs)

// Send 8 bits to SPI
// Data is latched on rising edge of clock for now... maybe add option for rising or falling edge
// Output MSB first
void max_write(unsigned char byte)
{
	int i;
	int b;

	for (i=7;i>=0;i--)					// Most sig first
	{
		output_low(mpin_clock);
		if (byte & (1<<i) )
		{
			output_high(mpin_data);
		}
		else	
		{	
			output_low(mpin_data);			// output the bit
		}
		output_high(mpin_clock);
	}
}


void max_new_seq()
{
	mseq=0;
}


void max_addto_seq(unsigned char b)
{
	sq[mseq++]=b;						// add next byte to sequence
}



// Diasy chained modules need CS asserted, then the enstire chain of data loaded before CS is de-asserted
void max_send_seq()
{
	int i;

	//os_printf("max_send_sequence - sending %d bytes\n",mseq);
	output_low(mpin_chipsel);
	os_delay_us(MDELAY);
	for (i=0;i<mseq;i++)
	{
		max_write(sq[i]);
		//os_printf("%02X ",sq[i]);
	}
	os_delay_us(MDELAY);
//Moved this side of delay
	output_high(mpin_chipsel);
	mseq=0;							// clear sequence, ready to build new one
}


// Setup MAX7219 for 8x8 matrix driving
void max_setgpio(int pin_data, int pin_clock, int pin_chipsel)
{
	mpin_data=pin_data;
	mpin_clock=pin_clock;
	mpin_chipsel=pin_chipsel;
//NEW
output_high(pin_chipsel);				// max7219 not selected
	os_printf("max_setgpio  PIN_DATA=%d\tPIN_CLOCK=%d\tPIN_CHIPSEL=%d\n",mpin_data,mpin_clock,mpin_chipsel);
	mseq=0;							
}



// Initialise N controllers
void max_init(int nc)
{
	int i;

	mnc=nc;
	mseq=0;
	for (i=0;i<=nc;i++)				// one more then really exists
	{
		max_addto_seq(0x09);
		max_addto_seq(0x00);			// Decode Mode: 0 = no decode
	}
	max_send_seq();

	for (i=0;i<nc;i++)
	{
		max_addto_seq(0x08);
		max_addto_seq(0x08);			// Set limit 8 (8 rows of 8 pixels)
	}
	max_send_seq();

	for (i=0;i<nc;i++)
	{
		max_addto_seq(0x0a);
		max_addto_seq(0x00);			// Brightness: 0 = dim, f = brightest
	}
	max_send_seq();

	for (i=0;i<nc;i++)
	{
		max_addto_seq(0x0b);
		max_addto_seq(7);			// Number of Digits: 3 = 4 Digits, 7 = 8 Digits  *fixme one day
	}
	max_send_seq();

	for (i=0;i<nc;i++)
	{
		max_addto_seq(0x0f);
		max_addto_seq(0x00);			// Test Register: 0 = Normal, 1 = Test Operation
	}
	max_send_seq();

	for (i=0;i<nc;i++)
	{
		max_addto_seq(0x0c);
		max_addto_seq(0x01);			// Shutdown: 0 = Shutdown, 1 = Normal Operation
	}
	max_send_seq();
}



// Set all MAX7219 ICs to brightness b,  values 0(dim) to 15(brightest)
void max_setbright(int b)
{
	int i;

	mseq=0;
	if (b<0)
		b=0;
	if (b>15)
		b=15;
	for (i=0;i<mnc;i++)
	{
		max_addto_seq(0x0a);
		max_addto_seq(b);
	}
	max_send_seq();
}





// Each call toggles pin high or low.  0=data, 1=clock, 2=chip_select
void max_blinkpin(int p)
{
	static int pf=0;

	pf=!pf;
	switch (p)
	{
		case 0:		output_bit(mpin_data,pf);	break;
		case 1:		output_bit(mpin_clock,pf);	break;
		case 2:		output_bit(mpin_chipsel,pf);	break;
	}
}


