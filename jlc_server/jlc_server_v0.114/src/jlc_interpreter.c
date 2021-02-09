/*
	jlc_interpreter.c

	Command interpreter, parse text and execute a command
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"
#include "jlp.h"
#include "jlc_group.h"

char origline[8192];
int  numwords=0;
char words[MAXWORDS][MAXWORDLEN];
char wordsuc[MAXWORDS][MAXWORDLEN];
int  offsets[MAXWORDS];							// offset into the text of the next word


// Console
extern uint32_t				console_m;
extern char				cprompt[MAX_PROMPTLEN];
extern char				version[];
extern int				cclr;
extern int				loophz;

extern struct tcp_connection            conns[MAX_TCPCONN];
extern struct statss			stats;
extern char  				statusline[STLINE_LEN];
extern unsigned int 			starttime;

extern struct jlc_group_cfg     grpc[MAX_GROUPS+1];
extern struct jlc_group_val     grpv[MAX_GROUPS+1];


extern struct timed_events     te[MAX_TIMED_EVENTS];

extern struct sequencescript           seqs[MAX_SEQ_SCRIPTS];
extern int	seqrun;



void removecrlf(char *textline, int textlength)
{
	int i;

	for (i=0;i<textlength;i++)
	{
		if (textline[i]==10 || textline[i]==13)
			textline[i]=0;
	}
}


// For a given file descriptor, is the terminal session set to colour, TRUE=yes
int iscolour(int textout)
{
	int i=0;
	if (textout==STDOUT_FILENO)								// using console
		return(cclr);
	i=conn_find_index(textout);
	return(conns[i].colour);
}


// Return the offset of the first character in a sting to occur after the word or -1 on error
int offset_after(char *sen, char *word)
{
	int x,y;
	int sl;
	int wl;
	int match;

	sl=strlen(sen);
	wl=strlen(word);
	if (sl==0 || wl==0)
		return(-1);

	for (x=0;x<sl;x++)
	{
		match=TRUE;
		for (y=0;y<wl;y++)
		{
			if (toupper(sen[x+y])!=toupper(word[y]))
				match=FALSE;
		}
		if (match==TRUE)
			return(x+y+1);
	}
	return(-1);                                                                             // word not found within string
}


void cmd_help(int textout, char *w)
{
	int all=FALSE;

	if (strlen(w)>0)
		strcpy(wordsuc[1],w);
	//printf("wordsuc[1]=[%s]\n",wordsuc[1]); fflush(stdout);
	if (strlen(wordsuc[1])<=0)
	{
		xprintf(textout,"HELP */<command>\n");
		xprintf(textout,"Valid commands are:");
		xprintf(textout,"HELP, COLOUR, LIST, RUN, MASK, MON, UNIV, GROUP, BLANK, GFX, FXCLEAR\n");
		xprintf(textout,"PLAYSOUND, BIN \n");
		xprintf(textout,"?\n");
	}
	if (wordsuc[1][0]=='*')
		all=TRUE;
	if ( (strncmp(wordsuc[1],"BIN",3)==0) || (all==TRUE) )
	{
		xprintf(textout," Command to allow front ends to enterigate server state efficiently\n");
		xprintf(textout," interface switches to binary mode\n");
	}
	if ( (strncmp(wordsuc[1],"GFX",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"GFX <group> <effect> [optional effect params]\n");
		xprintf(textout," GFX <group> NONE                               - Cancel any effects for group\n");
		xprintf(textout," GFX <group> PFADEUP <delay> [fade rate Hz]     - Wait duration, then fade up\n");
		xprintf(textout," GFX <group> PFADEDOWN <delay> [fade rate Hz]   - Wait duration, then fade down \n");
		xprintf(textout," GFX <group> STROBE <duration>                  - Strobe group \n");
	}
	if ( (strncmp(wordsuc[1],"FXCLEAR",4)==0) || (all==TRUE) )
	{
		xprintf(textout,"FXCLEAR <univ> <firstchan> <number of chan>     - Free these channels from any effects\n");
	}
	if ( (strncmp(wordsuc[1],"DUMP",4)==0) || (all==TRUE) )
	{
		xprintf(textout,"DUMP <univ>\n");
		xprintf(textout,"Shoow all light values for a universe\n");
	}
	if ( (strncmp(wordsuc[1],"BLANK",4)==0) || (all==TRUE) )
	{
		xprintf(textout,"BLANK <univ>\n");
		xprintf(textout,"Set all non X channels of universe to zero\n");
	}
	if ( (strncmp(wordsuc[1],"DATE",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"DATE [optional fmt number]\n");
		xprintf(textout,"Display the current date/time\n");
	}
	if ( (strncmp(wordsuc[1],"LIST",4)==0) || (all==TRUE) )
	{
		xprintf(textout,"LIST IP/DEV/STATE/GROUPS/UNIV/FX/SLINE/SUB/SEQ/COL\n");
		xprintf(textout," IP\t\t\tList IP addresses used by devices\n");
		xprintf(textout," DEV\t\t\tList all devices\n");
		xprintf(textout," DEV FULL\t\tList all devices in detail\n");
		xprintf(textout," DEV <name>\t\tList specific device\n");
		xprintf(textout," DEV <name> FULL\tList specific device in detail\n");
		xprintf(textout," DEV VER\t\tList model and firmware version for all devs\n");
		xprintf(textout," DEV STATE\t\tList device state\n");
		xprintf(textout," DEV MONITOR\t\tList device in the same format as 'monitor dev'\n");
		xprintf(textout," UNIV\t\t\tList currently loaded universes\n");
		xprintf(textout," GROUP\t\t\tList loaded groups\n");
		xprintf(textout," GROUP MONITOR\tList groups in same format as 'monitor grp'\n");
		xprintf(textout," STATE\t\t\tList current state of all devices\n");
		xprintf(textout," STATE <uid>\t\tList current state of a device\n");
		xprintf(textout," FX\t\t\tList effects\n");
		xprintf(textout," SLINE\t\t\tCurrent status line\n");
		xprintf(textout," SUB\t\t\tSubscriptions\n");
		xprintf(textout," SEQ\t\t\tLoaded sound to light sequences\n");
		xprintf(textout," COL\t\t\tColour Presets\n");
	}
	if ( (strncmp(wordsuc[1],"MON",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"MON ALL/GRP/ERR/INF/REG/JCP/DEV/NDE/SCR/WWW/SAMX\n");
		xprintf(textout,"Monitor events, [A]ll, [E]rrors, [G]roups, [R]egistrations, [J]ControlProtocol, [D]evice, [N]DE");
		xprintf(textout,"[SND]Sound, [SCR]ipts, [W]ww");
		xprintf(textout,"\nX-turn off monitor\n");
	}
	if ( (strncmp(wordsuc[1],"RUN",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"RUN <scriptname>\n");
	}
	if ( (strncmp(wordsuc[1],"MASK",4)==0) || (all==TRUE) )
	{
		xprintf(textout,"MASK CREATE/SHOW/KEY\n");
		xprintf(textout," CREATE <univ num> <num of channels> <pattern>\n");
		xprintf(textout," Create a mask file describing a universe, use UNIV <univ> LOAD to make it active\n");
		xprintf(textout,"MASK SHOW <univ num>\n");
		xprintf(textout," Show the contents of a mask file\n");
		xprintf(textout,"MASK KEY\n");
		xprintf(textout," Show the characters used in the mask and their meaning\n");
	}
	if ( (strncmp(wordsuc[1],"COLOUR",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"COLOUR, toggles between ANSI colour 24 bit or plain text\n");
	}
	if ( (strncmp(wordsuc[1],"UNIV",4)==0) || (all==TRUE) )
	{
		xprintf(textout,"UNIV <univ> LOAD            Load a universe (from file)\n");
		xprintf(textout,"UNIV <univ> SHOW            Show details about universe <univ>\n");
		xprintf(textout,"UNIV <univ> <value>         Set all channels of <univ> to <value>\n");
		xprintf(textout,"UNIV <univ> <chan> <value>  Set channel <chan> of universe <univ> to value\n");
		xprintf(textout,"UNIV <univ> COLOURS <value> Set channels to values based on colour mask\n");
		xprintf(textout,"Examples:\n");
		xprintf(textout,"  UNIV 2 R 0                Set all Red channels of universe 2 to 0\n");
		xprintf(textout,"  UNIV 2 RGB 255            Set all Red,Green and Blue channels to maximum\n");
		xprintf(textout,"\nA half brightness yellow for all of universe 2 would be these commands:\n");
		xprintf(textout,"  UNIV 2 B 0\n");
		xprintf(textout,"  UNIV 2 RG 128\n");
		xprintf(textout,"See MASK KEY for colour masks and MASK CREATE for universe creation\n");
	}
	if ( (strncmp(wordsuc[1],"MAP",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"MAP <uid> UNIV <univ> <firstchan> [numchannels]\n");
		xprintf(textout,"MAP <uid> GROUP <groupnum>\n");
		xprintf(textout," MAP <uid> UNIV maps a DEVF device onto a universe, <firstchan> is the\n");
		xprintf(textout," first channel of the universe to map onto.\n");
		xprintf(textout," For example:  a device with UID 5ccf7f117ebf01 has registered, it has 5 channels.\n");
		xprintf(textout," 'map 5ccf7f117ebf01 univ 1 10'\n"); 
		xprintf(textout," would map the device to universe 1 channel 10 would mean the devices channel 1\n");
		xprintf(textout," would be universe1 chan10 devices channel 2 would be universe 1 channel 11 etc\n");
		xprintf(textout,"\n");
		xprintf(textout," When DEVN (sliders, switches, relays) change state they use run a script, normally the\n");
		xprintf(textout," script modifies the value of a GROUP. \n");
		xprintf(textout," When group values change the device needs to be notified of the new group value\n");
		xprintf(textout," MAP <uid> group <groupnum> sets the system up to push notification of group\n");
		xprintf(textout," value changes to a device\n");
		xprintf(textout," For example a switch with UID 5ccf7f117ebf03 has registered.\n");
		xprintf(textout," use 'MAP 5ccf7f117ebf03 group 2' to ensure the switch is updated when group 2 changes\n");
		xprintf(textout," this can safely be added to the devices script file './scripts/5ccf7f117ebf03_reg'\n");
	}
	if ( (strncmp(wordsuc[1],"PRINT",5)==0) || (all==TRUE) )
	{
		xprintf(textout,"PRINT <message>\n");
		xprintf(textout,"PRINT <message $variable>\n");
		xprintf(textout,"See help var for variable names\n");
		xprintf(textout,"Displays a message\n");
	}
	if ( (strncmp(wordsuc[1],"PRINTCON",8)==0) || (all==TRUE) )
	{
		xprintf(textout,"PRINTCON <message>\n");
		xprintf(textout,"Outputs a message to the console\n");
	}
	if ( (strncmp(wordsuc[1],"GROUP",5)==0) || (all==TRUE) )
	{
//JA
		//xprintf(textout,"GROUP <groupnum/reload> <VALUE/show/?/on/off/I>\n");
		xprintf(textout,"Exmaples:\n");
		xprintf(textout,"  group reload\t\tReload group Description files group_1 etc\n");
		xprintf(textout,"  group 1 show\t\tLists universe/channels for group 1\n");
		xprintf(textout,"  group 1 ?\t\tDetails for group 1\n"); 
		xprintf(textout,"Group value change Simple form:\n");
		xprintf(textout,"  group 1 +on\t\tTurn on and set intensity to max\n");
		xprintf(textout,"  group 1 on\t\tTurn on lights\n");
		xprintf(textout,"  group 1 off\t\tTurn off lights\n");
		xprintf(textout,"  group 1 +off\t\tTurn off lights and set intensity to min\n");
		xprintf(textout,"  group 1 255\t\tSet intensity to max\n");
		xprintf(textout,"  group 1 +255\t\tOn and intensity to max\n");
		xprintf(textout,"Group value Colour Mask form:  R=RED, G=GREEN, B=BLUE, I=INTENSITY, +=ON\n");
		xprintf(textout,"  group 2 I100\t\tIntensity 100\n");
		xprintf(textout,"  group 2 +I100\t\tonoff=on Intensity 100\n");
		xprintf(textout,"  group 2 R50 G60 B70\n");
		xprintf(textout,"  group 2 R50 G60 B70 W200\n");
		xprintf(textout,"  group 2 R50 G60 B70 W200 I255\n");
	}
	if ( (strncmp(wordsuc[1],"DS",2)==0) || (all==TRUE) )
	{
		xprintf(textout,"DEV <UID> STATE <value1> [value2] [String]\n");
		xprintf(textout,"   Set device state, send value1, value2 and Valuebytes to device\n");
		xprintf(textout,"   The meaning of Value1, Value2 and [String] depends on the device, Table:\n");
		xprintf(textout,"MODEL          VALUE1  [VALUE2]  [STRING]  DESCRIPTION\n"); 
		xprintf(textout,"MAX7219CLOCK   1       0..15               Set display brightness\n");
		xprintf(textout,"MAX7219CLOCK   2       <duration> <string> Display <string> for duration seconds\n");
		xprintf(textout,"MAX7219CLOCK   3       0,1 or 2            : Blink rate, 0=none, 1=fast, 2=slow\n");
		xprintf(textout,"MAX7219CLOCK   4       <duration>          Flash display for duration seconds\n");
		xprintf(textout,"RELAY          0                           Turn relay off\n");
		xprintf(textout,"RELAY          1                           Turn relay on\n");	
		xprintf(textout,"RELAY          1       <duration>          Turn relay on, then off in value2 seconds\n");	
		xprintf(textout,"TEMPSENSOR     <duration>                  Report temperature every value1 seconds\n");
		xprintf(textout,"PLAYSOUNDS     LVolume% RVolume%% Samplenme Play sample at volume Left% Right% (1..100)\n");
	}
	if ( (strncmp(wordsuc[1],"DEV",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"DEV <UID> NAME <string>\n");
		xprintf(textout,"Set a device name\n");
		xprintf(textout,"DEV <UID> STATE <value1> [value2] [valuebytes as ASCII]\n");
		xprintf(textout,"Push state at a device, turn a relay or play a sound for example\n");
		xprintf(textout,"DEV <UID> HIDE\n");
		xprintf(textout,"Do not show this device except in 'full' reports\n");
		xprintf(textout,"DEV <UID> CONFIG <LOCATION> <NAME upto 32 chars>\n");
		xprintf(textout,"Give the device a name\n");
		//xprintf(textout,"DEV <UID> RETRIGGER <seconds>\n");
		//xprintf(textout,"Retrigger interval. Run the _1f script after the device has been idle for at least N seconds\n");
	}
	if ( (strncmp(wordsuc[1],"EVERY",4)==0) || (all==TRUE) )
	{
		xprintf(textout,"EVERY\n");
		xprintf(textout,"Show timed commands as list\n");
		xprintf(textout,"EVERY <interval> <command>\n");
		xprintf(textout,"Interval can be 1s 1m 1h etc \n");
		xprintf(textout,"EVERY DEL <list number>\n");
		xprintf(textout,"Delete entry from list\n");
	}
	if ( (strncmp(wordsuc[1],"IF",2)==0) || (all==TRUE) )
	{
		xprintf(textout,"IF\n");
		xprintf(textout,"IF DAYLIGHT STOP\n");
		xprintf(textout,"IF NO DEV <UID> STOP\n");
	}
	if ( (strncmp(wordsuc[1],"LOG",3)==0) || (all==TRUE) )
	{
		//log DOORBELL_A020A617D9D701 $dateu $\t A020A617D9D701 Doorbell pushed
		xprintf(textout,"LOG <filename> <string>\n");
		xprintf(textout,"example  log DOORBELL_A020A617D9D701 $dateu $\\t A020A617D9D701 Doorbell pushed\n");
	}
	if ( (strncmp(wordsuc[1],"PLAYSOUND",5)==0) || (all==TRUE) )
	{
		xprintf(textout,"PLAYSOUND <optional group, 0=ALL> <Volume percentage> <sample name>\n");
		xprintf(textout,"Devices subscribed to TOPIC_SAM or listening to monitor messages SAM\n");
		xprintf(textout,"will play the named sample\n");
		xprintf(textout,"Optional group number allows only a subset of sound players to play a sample\n");
		xprintf(textout,"example   playsound 0 100 doorbell3\n");
		xprintf(textout,"Everything will try and play the named sample, very loudly\n");
	}
	if ( (strncmp(wordsuc[1],"SEQ",3)==0) || (all==TRUE) )
	{
		xprintf(textout,"SEQ LOAD <filename>\n");
		xprintf(textout,"SEQ REMOVE <filename>\n");
		xprintf(textout,"SEQ LIST\n");
		xprintf(textout,"SEQ DEBUG <ON/OFF>\n");
		xprintf(textout,"SEQ RUN <sequence number>\n");
	}

	if ( (strncmp(wordsuc[1],"VAR",3)==0) )
	{
		variable_list(textout);
	}
}



// Monitor server activity, X cancels
void cmd_monitor(int textout, char*w)
{
	int i=0;
	uint32_t m=0;

	if (strlen(w)>0)
		strcpy(wordsuc[1],w);
	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout,"MON");
		return;
	}

	if (textout==STDOUT_FILENO)
	{
		m=console_m;
	}
	else
	{
		i=conn_find_index(textout);
		m=conns[i].m;
		if (i<0)
		{
			xprintf(textout,"Err: con_find_index(%d) returned %d\n",textout,i);
			return;
		}
	} 
	
	if (strncmp(wordsuc[1],"X",1)==0)
	{
		xprintf(textout,"Monitor cancelled\n");
		m = 0;
	}

	if (strncmp(wordsuc[1],"A",1)==0)
	{
		xprintf(textout,"Monitor All except JCP");
		//m = 0xFFFFFFFF;
		m = MSK_DEV |MSK_NDE | MSK_ERR | MSK_INF | MSK_REG | MSK_GRP | MSK_SCR | MSK_TCP | MSK_SND | MSK_WWW;
	}
	if (strncmp(wordsuc[1],"N",1)==0)
	{
		xprintf(textout,"Monitor device names");
		m |= MSK_NDE;
	}
	if (strncmp(wordsuc[1],"D",1)==0)
	{
		xprintf(textout,"Monitor devices");
		m |= MSK_DEV;
	}
	if (strncmp(wordsuc[1],"E",1)==0)
	{
		xprintf(textout,"Monitor Errors");
		m |= MSK_ERR;
	}
	if (strncmp(wordsuc[1],"I",1)==0)
	{
		xprintf(textout,"Monitor Info");
		m |= MSK_INF;
	}
	if (strncmp(wordsuc[1],"R",1)==0)
	{
		xprintf(textout,"Monitor registrations");
		m |= MSK_REG;
	}
	if (strncmp(wordsuc[1],"J",1)==0)
	{
		xprintf(textout,"Monitor JControlProtocol");
		m |= MSK_JCP;
	}
	if (strncmp(wordsuc[1],"G",1)==0)
	{
		xprintf(textout,"Monitor Groups");
		m |= MSK_GRP;
	}
	if (strncmp(wordsuc[1],"SCR",3)==0)
	{
		m |= MSK_SCR;
		xprintf(textout,"Monitor script execution");
	}

	if (strncmp(wordsuc[1],"WWW",1)==0)
	{
		m |= MSK_WWW;
		xprintf(textout,"Monitor HTTP requests");
	}

	if (strncmp(wordsuc[1],"TCP",1)==0)
	{
		m |= MSK_TCP;
		xprintf(textout,"Monitor TCP connect/disconnect");
	}

	if (strncmp(wordsuc[1],"SND",3)==0)
	{
		m |= MSK_SND;
		xprintf(textout,"Monitor sound play requests");
	}

	if (textout==STDOUT_FILENO)
		console_m=m;
	else	conns[i].m=m;
	if (m!=0)
		xprintf(textout,", Use MON X to cancel...\n");
}



void cmd_list(int textout, char*w)
{
	if (strlen(w)>0)
		strcpy(wordsuc[1],w);
	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout,"LIST");
		return;
	}

	if (strncmp(wordsuc[1],"SLINE",2)==0)
	{
		xprintf(textout,"%s\n",statusline);
		return;
	}

	if (strncmp(wordsuc[1],"SEQ",2)==0)
	{
		seq_list(textout);
		return;
	}

	if (strncmp(wordsuc[1],"SUB",2)==0)
	{
		list_subs(textout, wordsuc[2]);
		return;
	}

	if (strncmp(wordsuc[1],"IP",1)==0)
	{
		list_ip_details(textout);
		return;
	}

	if (strncmp(wordsuc[1],"GROUP",1)==0)
	{
		if (strlen(wordsuc[2])==0)
		{
			list_groups(textout);
			return;
		}
	}

	if (strncmp(wordsuc[1],"STATE",1)==0)
	{
		list_dev_state(textout, wordsuc[2]);
		return;
	}

	if (strncmp(wordsuc[1],"DEV",1)==0)
	{
		if (strncmp(wordsuc[2],"STATE",2)==0)
		{
			list_dev_state(textout, wordsuc[3]);
			return;
		}
		if (strncmp(wordsuc[2],"MON",2)==0)
		{
			list_dev_monitor(textout);
			return;
		}
//NEW
		if (strncmp(wordsuc[2],"NAME",1)==0)
		{
			list_dev_names(textout);
			return;
		}
//
		list_devices(textout, wordsuc[2], wordsuc[3]);
		return;
	}

	if (strncmp(wordsuc[1],"UNIV",1)==0)
	{
		list_universes(textout,0);				// 0=all
		return;
	}

	if (strncmp(wordsuc[1],"FX",1)==0)
	{
		list_fx(textout);
		return;
	}

	if (strncmp(wordsuc[1],"TEMP",1)==0)
	{
		list_temperatures(textout);
		return;
	}

	// Show only devices with mapping to a universe
	if (strncmp(wordsuc[1],"MAP",1)==0)
	{
		list_map(textout);
		return;
	}

	if (strncmp(wordsuc[1],"COL",1)==0)
	{
		colour_presets_list(textout);
		return;
	}
}



void cmd_run(int textout, char*w)
{
	static char cmd[1024];

	if (strlen(w)>0)
		strcpy(wordsuc[1],w);
	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout,"RUN");
		return;
	}

	strncpy(cmd,words[1],sizeof(cmd)-1);
	if (run_script(textout, words[1])==TRUE)
	{
		//xprintf(textout,"completed [%s]\n",cmd);
	}
}


		
// MASK CREATE <univ num> <num of channels> <pattern>
//  0     1        2              3            4
void cmd_mask(int textout, char*w)
{
	int noc=0;
	int u=0;

	if (strlen(w)>0)
		strcpy(wordsuc[1],w);
	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout,"MASK");
		return;
	}

	if (strcmp(wordsuc[1],"KEY")==0)
	{
		xprintf(textout,"KEY: %s\n",MASK_KEY);
		return;
	}

	if (strncmp(wordsuc[1],"CREATE",5)==0)
	{
		u=atoi(wordsuc[2]);
		if (universe_valid_range(textout, u, TRUE)!=TRUE)
			return;

		noc=atoi(wordsuc[3]);
		if (noc<=0)
		{
			xprintf(textout,"MASK CREATE Err, <num of channels> must be >0 <=%d\n",MAX_CHANNELS);
			return;
		}

		if (strlen(wordsuc[4])<=0)
		{
			xprintf(textout,"MASK CREATE Err, Missing pattern\n");
			return;
		}
		mask_create(textout,u,noc,(char*)&wordsuc[4]);
		xprintf(textout,"Mask created, use 'univ %d load' to make it active\n",u);
		return;
	}

	if (strncmp(wordsuc[1],"SHOW",4)==0)
	{
		u=atoi(wordsuc[2]);
		if (universe_valid_range(textout, u, TRUE)!=TRUE)
			return;
		mask_show(textout,u);
	}
}



//  0     1      2        3     
// univ <univ> load
// univ <univ> show 
// univ <univ> <value>                   Every channel of univ <univ> to <value>
// univ <univ> <chan>  <value>           Set a single channel of univ <univ> to <value>
// univ <univ> COLOURS <value>           Set channels matching COLOURS to <value>
void cmd_univ(int textout)
{
	int u=0;
	int v=0;
	int c=0;

	u=atoi(wordsuc[1]);
	if (strlen(wordsuc[1])<=0 || u<=0)
	{
		cmd_help(textout,"UNIV");
		return;
	}

	// remember not to add L or S to colour mask...
	if (strncmp(wordsuc[2],"LOAD",1)==0)
	{
		universe_load(textout, u);
		return;
	}
	if (strncmp(wordsuc[2],"SHOW",1)==0)
	{
		universe_show(textout, u, iscolour(textout));
		return;
	}

	if (strlen(wordsuc[3])==0)						// No third argument ?	
	{
		v=atoi(wordsuc[2]);
		universe_set_allchan(textout, u, v);				// then univ <univ> <value>
		return;
	}

	if (wordsuc[2][0]>=48 && wordsuc[2][0]<=57)				// 0 to 9 ASCII ?
	{
		c=atoi(wordsuc[2]);
		v=atoi(wordsuc[3]);
		universe_channel_set(textout, u, c, v);
		return;
	}

	// Still here? then it should be univ <univ> COLOURS <value>
	v=atoi(wordsuc[3]);
	universe_set_colours(textout, u, (char*)&wordsuc[2], v);
}





// Returns TRUE if now colour, FALSE if now mono
int cmd_colour(int textout)
{
	int i=0;

	if (textout==STDOUT_FILENO)								// using console
	{
		cclr = ! cclr;
		if (cclr==TRUE)
		{
			sprintf(cprompt,"%c[38;2;0;255;0m> %c[38;2;255;255;255m",27,27);
			return(TRUE);
		}
		else
		{
			sprintf(cprompt,"> ");
			return(FALSE);
		}
	}
	else
	{
		i=conn_find_index(textout);
		conns[i].colour = !conns[i].colour;						// Toggle
		if (conns[i].colour==TRUE)
		{
			sprintf(conns[i].prompt,"%c[38;2;0;255;0m> %c[38;2;255;255;255m",27,27);
			return(TRUE);
		}
		else	
		{
			sprintf(conns[i].prompt,"> ");
			return(FALSE);
		}
	}
}



void cmd_map(int textout, char *w)
{
	//int u=0;

	if (strlen(w)>0)
		strcpy(wordsuc[1],w);
	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout,"MAP");
		return;
	}
	// MAP

	// map <uid> group <groupnum>
	if (strncmp(wordsuc[2],"GR",2)==0)							// map <uid> GRoup
	{
		device_map_group(textout, wordsuc[1], atoi(wordsuc[3]));			// allow group=0 as well
		return;
	}

	// map <uid> <univ> <fist channel>
	device_map_uc(textout, wordsuc[1], atoi(wordsuc[2]), atoi(wordsuc[3]));
}



void cmd_univ_dump(int textout)
{
	int u=0;

	u=atoi(wordsuc[1]);
	if (universe_valid_range(textout, u, TRUE)!=TRUE)
		return;
	universe_dump(textout, u);
}



// Group effects, apply and effect to all univ/chan of a group
// GFX <group> <effect> [effect params]
void cmd_gfx(int textout)
{
	int g;

	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout, "GFX");
		return;
	}
	g=atoi(wordsuc[1]);
	if (g<=0 || g>=MAX_GROUPS)
	{
		cmd_help(textout,"GFX");
		return;
	}
	group_apply_fx(textout, g, wordsuc[2], wordsuc[3], wordsuc[4], wordsuc[5]);		// Apply effect to all in group	
}



// Make sure these channels of a universe have no effects set
void cmd_fxclear(int textout)
{
}




// Modify device params, only "NAME" at the moment
// DEV <UID> PLAYSOUND [optional filename]
// DEV <UID> NAME <string>
// DEV <UID> STATE <v1> <v2> <string>
// DEV <UID> HIDE
// DEV <UID> CONFIG <LOCATION> <NAME upto 32 chars>
// 0    1    2      3...
void cmd_dev(int textout)
{
	char dev_name[1024];
	char wrd[1024];
	int i=0;
	char text[8192];
	//int l=0;

	bzero(&dev_name, sizeof(dev_name));
	//if (strlen(wordsuc[1])<=0 || strlen(wordsuc[3])<=0 )
	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout,"DEV");
		return;
	}

//TODO: use offsets and test
	strcpy(dev_name,words[3]);
	for (i=4;i<MAXWORDS-1;i++)
	{
		if (wordsuc[i][0]!=0)
		{
			sprintf(wrd," %s",words[i]);
			strcat(dev_name,wrd);
		}
	}
	//printf("dev_name=[%s]\n",dev_name); fflush(stdout);

	if (strncmp(wordsuc[2],"NAME",1)==0)
		dev_set_name(textout, wordsuc[1], (char*)&dev_name);
	if (strncmp(wordsuc[2],"STATE",1)==0 || strncmp(wordsuc[1],"STATE",1)==0)
	{
		if (strlen(wordsuc[3])==0)
			cmd_help(textout,"DS");
		else	
		{
			//i=offsets[4];				// Start of text after 5th word "DEV uid state N N"
			//strcpy(text, (char*)&origline[i]); 
			//dev_set_state(textout, wordsuc[1], wordsuc[3], wordsuc[4], (char*)&origline[i]);

			bzero(&text,sizeof(text));
			for (i=5;i<MAXWORDS-1;i++)
			{
				if (words[i][0]!=0)
				{
					if (strncmp(words[i],"$SP",3)==0)
					{
						strcat(text," ");
					}
					else
					{
						sprintf(wrd,"%s ",words[i]);
						strcat(text,wrd);
					}
				}
			}
			if (strlen(text)>0)
				text[strlen(text)-1]=0;
			dev_set_state(textout, wordsuc[1], wordsuc[3], wordsuc[4], (char*)&text);
		}
	}
	if (strncmp(wordsuc[2],"HIDE",1)==0)
		dev_hide(textout, wordsuc[1], wordsuc[2]);
	if (strncmp(wordsuc[2],"CONFIG",1)==0)
	{
		if (strlen(wordsuc[3])>0)
		{
			bzero(&text, sizeof(text));
			for (i=4;i<MAXWORDS-1;i++)
			{
				if (strlen(words[i])>0)
				{
					if (i!=4)
						strcat(text," ");
					strcat(text,words[i]);
				}
				text[sizeof(text)-1]=0;
			}
			dev_config(textout, wordsuc[1], wordsuc[3], (char*)&text);
		}
	}
}



//int seq_load(int textout);
//int seq_loaded();
//int seq_remove(int textout);
//void seq_list(int textout);
//int seq_test(int textout);
//int seq_step(int textout);
int cmd_seq(int textout)
{
	int i=-1;

	if (strlen(wordsuc[1])<=0)
	{
		cmd_help(textout,"SEQ");
		return(TRUE);
	}

	if (strncmp(wordsuc[1],"LOAD",2)==0)
	{
		if (strlen(words[2])>0)
		{
			i=seq_load(textout, words[2]);
			if (i>=0)
			{
				xprintf(textout,"Loaded script %s, %d non blank lines, %d timing marks\n",seqs[i].filename,
					seqs[i].num_lines,seqs[i].num_timemarks);
				return(TRUE);
			}
			else	return(FALSE);					// load failed
		}
		else
		{
			cmd_help(textout,"SEQ");
			return(TRUE);
		}
	}

	if (strncmp(wordsuc[1],"REMOVE",2)==0)
	{
		return(TRUE);
	}

	if (strncmp(wordsuc[1],"LIST",1)==0)
	{
		seq_list(textout);
		return(TRUE);
	}

	if (strncmp(wordsuc[1],"DEBUG",1)==0)					// SEQ DEBUG ON/OFF
	{
		if (wordsuc[2][1]=='N')						// "ON"
			seq_debug(textout, TRUE);
		else	seq_debug(textout, FALSE);
		return(TRUE);
	}


	if (strncmp(wordsuc[1],"RUN",1)==0)					// SEQ DEBUG ON/OFF
	{
		if (strlen(wordsuc[2])<=0)
		{
			cmd_help(textout,"SEQ");
			return(TRUE);
		}

		seqrun=atoi(wordsuc[2]);					// start clocking this script (run without audio)
		seqrunsequence_run(textout, seqrun);				// returns -1 if their is a problem
	}
	return(FALSE);
}



// Returns TRUE if parsed with no errors. Errors may not be important though
int colour_text_parse(int textout, int firstword, int *R, int *G, int *B, int *W, int *I, int *O)
{
	int w=firstword;
	int v=0;

	*R=-1;
	*G=-1;
	*B=-1;
	*W=-1;
	*I=-1;
	*O=-1;
	do
	{
		//printf("parse %s\n",wordsuc[w]); fflush(stdout);
		if (wordsuc[w][0]>='0' && wordsuc[w][0]<='9')
		{								// Number on its own?
			//printf("Number on its own\n");
			*I=atoi((char*)&wordsuc[w][0]);				// then number is intensity
		}
		else
		{
			if (wordsuc[w][0]=='+')					// Number prefixed with +
			{
				*O=1;						// onoff = on
				*I=atoi((char*)&wordsuc[w][1]);			// + is always a prefix for intensity
			}
			else
			{
				if (wordsuc[w][1]==':')				// Example R:200
					v=atoi((char*)&wordsuc[w][2]);
				else	v=atoi((char*)&wordsuc[w][1]); 		// R200
				switch (wordsuc[w][0])
				{
					case 'R':  *R=v;	break;
					case 'G':  *G=v;	break;
					case 'B':  *B=v;	break;
					case 'W':  *W=v;	break;
					case 'I':  *I=v;	break;
					case 'O':  *O=v;	break;
				}
			}
		}
		w++;
	} while (wordsuc[w][0]!=0);
	return(TRUE);
}



/*
Exmaples:
  group reload          Reload group Description files group_1 etc
  group 1 show          Lists universe/channels for group 1
  group 1 ?             How many channels in group 1
Group value change Simple form:
  group 1 +on           Turn on and set intensity to max
  goupp 1 on            Set onoff flag to 1 (Turn on lights)
  group 1 off           Set onoff flag to 0 (Turn off lights)
  group 1 +off          Turn off lights and set intensity to min
  group 1 255           Set intensity to max
  group 1 +255          Set onoff to on and intensity to max
Group value Colour Mask form:  R=RED, G=GREEN, B=BLUE, I=INTENSITY, +=ON
  group 2 I100          Intensity 100
  group 2 +I100         onoff=on Intensity 100
  group 2 R50 G60 B70
  group 2 R50 G60 B70 W200
  group 2 R50 G60 B70 W200 I255
*/

//JA
int cmd_group(int textout)
{
	int g=0;
	int R=-1;
	int G=-1;
	int B=-1;
	int W=-1;
	int I=-1;
	int O=-1;

	if (strlen(wordsuc[1])==0)						// No other arguments?
	{
		cmd_help(textout, "GROUP");
		return(TRUE);
	}

	if (strncmp(wordsuc[1],"RELOAD",1)==0)					// Single argument "R"eload ?
	{
		group_reload(textout);
		return(TRUE);
	}

	g=atoi(wordsuc[1]);
	if (wordsuc[2][0]=='?')							// group <num> ?
	{
		list_group(textout, g);
		return(TRUE);
	}
	if (strncmp(wordsuc[2],"L",1)==0 || strncmp(wordsuc[2],"S",1)==0)	// group <num> List or  Show
	{
		list_group_channels(textout, g);
		return(TRUE);
	}


	if (strncmp(wordsuc[2],"ON",2)==0)					// group <num> on
	{
		group_set_value(textout, g, -1, -1, -1, -1, -1, 1);		// set onoff only
		return(TRUE);
	}

	if (strncmp(wordsuc[2],"+ON",3)==0)					// group <num> +on
	{
		group_set_value(textout, g, -1, -1, -1, -1, 255, 1);		// set onoff only
		return(TRUE);
	}
	if (strncmp(wordsuc[2],"OFF",3)==0)					// group <num> off
	{
		group_set_value(textout, g, -1, -1, -1, -1, -1, 0);
		if (g!=-10)
			group_apply_fx(textout, g, "NONE", "", "", "");		// Cancel all effects for this group
		return(TRUE);
	}
	if (strncmp(wordsuc[2],"+OFF",3)==0)					// group <num> off
	{
		group_set_value(textout, g, -1, -1, -1, -1, 0, 0);
		if (g!=-10)
			group_apply_fx(textout, g, "NONE", "", "", "");		// Cancel all effects for this group
		return(TRUE);
	}

	colour_text_parse(textout,2,&R,&G,&B,&W,&I,&O);
	group_set_value(textout, g, R, G, B, W, I, O);
	//printf("R=%d\tG=%d\tB=%d\tW=%d\tI=%d\tO=%d\n",R,G,B,W,I,O);
	return(TRUE);
}






char *concat_words(int idx, int lastword)
{
	int i=0;
	static char concat[8192];

	concat[0]=0;
	for (i=idx;i<numwords;i++)
	{
		if (i!=lastword)
		{
			strcat(concat,words[i]);
			strcat(concat," ");
		}
		else	strcat(concat,words[i]);
	}
	return((char*)&concat);
}




void bin_dump(int textout, unsigned char *data, int size)
{
	int i=0;

	xprintf(textout,"BYTES:%d\n",size);
	for (i=0;i<size;i++)
		xprintf(textout,"%c",data[i]);
}





// Returns TRUE if interpreted and valid, FALSE is invalid
int interpreter(int textout, char *textline, int textlength, int verbose)
{
	char *pch;
	int lastword=0;
	int l=0;
//int value;
	int g=0;
	int ll=0;										// line length
	int i=0;
	unsigned int offset=0;
	char text[4096];
	unsigned int tdiff=0;

	//printf("interpreter(%s)\n",textline); fflush(stdout);
	if (textlength<=0)
		return(FALSE);

	bzero(&words,sizeof(words));
	bzero(&wordsuc,sizeof(wordsuc));
	removecrlf((char*)textline, textlength);
	textlength=strlen(textline);
	strcpy(origline,textline);

	ll=strlen(textline);
	if (ll==0)
		return(TRUE);									// blank lines are fine
	if (textline[0]=='#')
		return(TRUE);									// skip comments

 	//printf ("Splitting string \"%s\" into tokens:\n",textline);
	numwords=0;
        pch = strtok (textline," ,\t");
        while (pch != NULL)
        {
                //printf ("%s\n",pch);
                strcpy(words[lastword],pch);
                for (l=0;l<strlen(words[lastword]);l++)
                        wordsuc[lastword][l]=toupper(words[lastword][l]);
                wordsuc[lastword][l++]=0;
                pch = strtok (NULL, " ,\t");
		offsets[lastword]=0;
		if (pch!=NULL)
		{
			offset=pch - textline;
			//printf("offset=%d\n",offset);
			offsets[lastword]=offset;
		}
		if (variable_subs((char*)wordsuc[lastword])==TRUE)				// A word was replaced with a variable
			strcpy(words[lastword],wordsuc[lastword]);				// make lower and upper case versions the value
		lastword++;
		numwords=lastword;
        }
        if (wordsuc[0][0]=='#')
                return(TRUE);


	if (strncmp(wordsuc[0],"UPTIME",4)==0)
	{
 		tdiff=current_timems() - starttime;
                xprintf(textout,"Uptime %s\n",ms_to_timetext(tdiff));
		return(TRUE);
	}


	//printf("numwords=%d\n",numwords); fflush(stdout);
	//printf("wordsuc[0]=[%s] wordsuc[1]=[%s]\n",wordsuc[0],wordsuc[1]); fflush(stdout);

	if (strncmp(wordsuc[0],"BIN",3)==0)
	{
		conn_set_binmode(textout);
		return(TRUE);
	}


	// Allow runscript to terminate early, in this case if it is daylight
	// "if daylight stop"
	if (strncmp(wordsuc[0],"IF",2)==0)
	{
		if (strncmp(wordsuc[1],"DAYLIGHT",3)==0)
		{
			l=isdaylight();
			if (strcmp(wordsuc[2],"STOP")==0)
			{
				if (l==TRUE)
					return(SCRIPT_STOP);					// tell caller interpreter wants code stopped
				else    return(TRUE);
			}
		}


		// "if no dev <uid> stop"
		if (strncmp(wordsuc[1],"NO",2)==0)
		{
			if (strncmp(wordsuc[2],"DEV",1)==0)
			{
				if (strncmp(wordsuc[4],"STOP",1)==0)
				{
					if (dev_lookup_idx_by_uid_string(DEV_N, (char*)wordsuc[3])<0)		// DEV not found ?
						return(SCRIPT_STOP);
					else    return(TRUE);
				}
			}
		}
	}


	if (strcmp(wordsuc[0],"?")==0)
	{
		xprintf(textout,"jlcd Ver:%s\n",version);
		xprintf(textout,"%d universes active, %d created\n",universe_count(UNIVERSE_ACTIVE),universe_count(UNIVERSE_CREATED));
		xprintf(textout,"%d tcp sessions active\n",conn_count());
		xprintf(textout,"%d F devices registered\n",dev_count(DEV_F));
		xprintf(textout,"%d N devices registered\n",dev_count(DEV_N));
		xprintf(textout,"%d sound to light scripts loaded\n",seq_count());
		xprintf(textout,"%d lighting groups loaded\n",group_count());
		xprintf(textout,"%d Hz main loop\n",loophz);

		if (stats.pbps_jlp!=0)
			xprintf(textout,"Sending JLC data %d packets per second (%d bytes per second)\n",stats.ppps_jlp, stats.pbps_jlp);
		if (stats.pbps_jcp!=0)
			xprintf(textout,"Sending JCP data %d packets per second (%d bytes per second)\n",stats.ppps_jcp, stats.pbps_jcp);
		if (stats.ppps_audioplus>0)
		{
			xprintf(textout,"Receiving audio plus data at %d packets per second (",stats.ppps_audioplus,sizeof(struct audioplus)*stats.ppps_audioplus);
			printbytes(textout, sizeof(struct audioplus)*stats.ppps_audioplus);
			xprintf(textout," per second)\n");
		}
	}


	if (strncmp(wordsuc[0],"CLEAR",5)==0)
		xprintf(textout,"%c[2J%c[1;1H",27,27);	

	if (strncmp(wordsuc[0],"EVERY",4)==0)
	{
		if (strlen(wordsuc[1])==0)
		{
			//xprintf(textout,"list every\n");
			te_list(textout);
			return(TRUE);
		}

		if (strncmp(wordsuc[1],"DEL",1)==0)				// delete entry
		{
			if (strncmp(wordsuc[2],"ALL",1)==0)			// every del all
			{
				for (l=0;l<MAX_TIMED_EVENTS;l++)
					te_clear(l);
				xprintf(textout,"Cleared all events\n");
				return(TRUE);
			}

			l=atoi(wordsuc[2]);
			if (l>=0 && l<MAX_TIMED_EVENTS)
			{
				if (te_clear(l)>=0)
					xprintf(textout,"Timed event %d removed\n",l);
				else	xprintf(textout,"No event %d\n",l);
				return(TRUE);
			}
		}

		// Extract cmd from line
		i=offsets[1];							// Start of text after 3rd word
		text[0]=0;
		if (i>0)
			strcpy(text, (char*)&origline[i]); 
//printf("TEXT=[%s]\n",text); fflush(stdout);


		// EVERY <interval> <cmd>
		i=timetext_to_ms(wordsuc[1]);
		if (i<0)
		{
			xprintf(textout,"Illegal time for command %d\n",i);
			return(FALSE);
		}

		if (strlen(text)<=0)
		{
			xprintf(textout,"Usage: Every <interval> <command>\n");
			return(FALSE);
		}



		l=te_find_idx_by_cmd(textout, text);				// try and find this command in list
		if (l>=00)							// found it
		{
			xprintf(textout,"refreshing event %d\n",l);
			te_set(textout, l, i, TRUE, (char*)&text); 
			return(TRUE);
		}

		g=te_add(textout, i, TRUE, (char*)&text);
		if (g>=0)
		{
			xprintf(textout,"Added event, number %d\n",g);
			return(TRUE);
		}
	}


	if (strncmp(wordsuc[0],"QUIT",4)==0)
	{
		xprintf(textout,"Bye..\n");
		if (textout==STDOUT_FILENO)
			exit(0);
		conn_disconnect(conn_find_index(textout), 20);
	}


// Remove me one day
	if (strncmp(wordsuc[0],"TEST",4)==0)
	{
		for (l=0;l<lastword;l++)
		{
			g=offsets[l];			// offset of first character of next word
			if (g>0)
			{
				printf("wordsuc[%d]=[%s] origline[%d]=%c\n",l,wordsuc[l],g,origline[g]);
				fflush(stdout);
			}
		}
	}


	if (strncmp(wordsuc[0],"LOG",3)==0)
	{
		if (strlen(wordsuc[1])>0)
			log_printf(wordsuc[1],"%s\n",concat_words(2,lastword));
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"HELP",4)==0)
	{
		cmd_help(textout,"");
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"LIST",1)==0)
	{
		cmd_list(textout,"");
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"RUN",3)==0)
	{
		cmd_run(textout,"");
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"MASK",4)==0)
	{
		cmd_mask(textout,"");
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"COLOUR",3)==0)
	{
		if (cmd_colour(textout)==TRUE)
			xprintf(textout,"Colour text\n");
		else	xprintf(textout,"Mono text\n");
	}

 	if (strncmp(wordsuc[0],"PRINTCON",7)==0)
        {
		printf("%s\n",concat_words(1,lastword));
		fflush(stdout);
                return(TRUE);
        }

 	if (strncmp(wordsuc[0],"PRINT",5)==0)
        {
		xprintf(textout,"%s\n",concat_words(1,lastword));
                return(TRUE);
        }

	if (strncmp(wordsuc[0],"MAP",3)==0)
	{
		cmd_map(textout,"");
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"GFX",3)==0)
	{
		cmd_gfx(textout);
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"DEV",3)==0)
	{
		cmd_dev(textout);
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"DATE",3)==0)
	{
		if (strlen(wordsuc[1])>0)
			l=atoi(wordsuc[1]);	
		else	l=4;
		xprintf(textout,"%s\n",datetimes(l));
		return(TRUE);
	}

// Everything below here parses a single character as the first word, so these must go last
	if (strncmp(wordsuc[0],"MON",1)==0)
	{
		cmd_monitor(textout,"");
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"UNIV",1)==0)
	{
		cmd_univ(textout);
		return(TRUE);
	}

	if (strncmp(wordsuc[0],"DUMP",1)==0)			// everything starting with "D" 
	{
		cmd_univ_dump(textout);
		return(TRUE);
	}
	if (strncmp(wordsuc[0],"BLANK",1)==0)			// everything starting with "B"
	{
		if (strlen(wordsuc[1])==0)
		{
			cmd_help(textout,"BLANK");
			return(TRUE);
		}
		//universe_channels_set(textout, atoi(wordsuc[1]), 'A', 0);		// set all channels in univ to zero
		universe_set_allchan(textout, atoi(wordsuc[1]), 0);			// set all channels in univ to zero
		return(TRUE);
	}


	if (strncmp(wordsuc[0],"GROUP",1)==0)
		return(cmd_group(textout));


	if (strncmp(wordsuc[0],"PLAYSOUND",5)==0)
	{
		if (strlen(wordsuc[1])<=0)
		{
			cmd_help(textout,"PLAYSOUND");
			return(FALSE);
		}
		g=atoi(wordsuc[1]);							// sound group, 0 = All	
		l=atoi(wordsuc[2]);							// volume
		dev_playsound(g,l,wordsuc[3]);
		sprintf(text,"%d %d %s",g,l,wordsuc[3]);
		bin_sendmsg_generics(BIN_MSG_PLAYSOUND, 0, text, strlen(text)+1);
		return(TRUE);
	}


	if (strncmp(wordsuc[0],"SEQ",3)==0)
	{
		return(cmd_seq(textout));
	}

	return(FALSE);
}
