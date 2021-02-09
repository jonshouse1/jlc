/*
	jlc_sound_sequencer.c
	Code to load light patterns and sequence them against audio
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlp.h"

#include "jlc_prototypes.h"


// Audioplus buffers
extern struct  audioplus               audioplus_records[FBSMAX];
extern char    audioplus_ipaddr[32];                                                                   // Audio data is from this source address
extern int     audioplus_sockfd;
extern int     audioplus_frame;
extern int     audioplus_framedelay;
extern char    audioplus_md5[64];


// Visuals
#define AUDIO_FRAMES_PER_SECOND		86
extern int     vis_vu_max;
extern int     vis_vu_left;
extern int     vis_vu_right;
extern int     vis_vu_leftpeak;
extern int     vis_vu_rightpeak;
extern int     vis_flag_agc;
extern int     vis_flag_peakhold;
extern int     vis_flag_ansivu;


extern struct statss                    stats;

extern struct sequencescript           seqs[MAX_SEQ_SCRIPTS];
extern int    seqrun;                



//#define MAX_SEQ_SCRIPTS                         32                              // Max number of scripts we load
//#define MAX_TIMEMARKS                           86 * 60 * 10                    // Hz * 1MIN * MINS
//#define MAX_SEQ_LINES                           MAX_TIMEMARKS * 20
// Basic information for each lighting sequence file (.lsq)
//struct sequencescript
//{
        //char    filename[MAX_FILENAMELEN];
        //char    md5hash[34];

        //char*   script;
        //int     scriptsize;                                                     // Size of script buffer in bytes
        //int     line_offsets[MAX_SEQ_LINES];                                    // offsets into script[] for the start of each line
        //int     line_length[MAX_SEQ_LINES];                                     // Length of line of text
        //int     timemark_line[MAX_TIMEMARKS];                                   // line number of lines  starting with an audio frame number [N]
        //int     timemark_lastline[MAX_TIMEMARKS];                               // the last line of the script before the next time mark
        //int     num_lines;
        //int     num_timemarks;
        //int     end_timemark;                                                   // time mark number for last time in script
        //int     debug;                                                          // TRUE to show code running, false for normal
        //int     framenumber;
//};





void seq_clear(int i)
{
	int x=0;

	bzero(seqs[i].filename, sizeof(seqs[i].filename));
	bzero(seqs[i].md5hash, sizeof(seqs[i].md5hash));
	seqs[i].script = NULL;
	for (x=0;x<MAX_SEQ_LINES;x++)
	{
		seqs[i].line_offsets[x]=-1;
		seqs[i].line_length[x]=0;
	}
	for (x=0;x<MAX_TIMEMARKS;x++)
	{
		seqs[i].timemark_line[x]=-1;
		seqs[i].timemark_lastline[x]=-1;
	}
	seqs[i].num_lines=0;
	seqs[i].num_timemarks=0;
	seqs[i].end_timemark=-1;
	if (seqs[i].script!=NULL)
	{
		free(seqs[i].script);
		seqs[i].script=NULL;
	}
	seqs[i].framenumber=-1;
	seqs[i].debug=FALSE;
}


void seq_init()
{
	int i=0;

	for (i=0;i<MAX_SEQ_SCRIPTS;i++)
		seq_clear(i);
}



// Returns the index of seqs[] with matching md5 or -1
int seq_match_md5(char *md5hash)
{
	int i=0;

	for (i=0;i<MAX_SEQ_SCRIPTS;i++)
	{
		if (seqs[i].num_lines>0)						// in use ?
		{
			if (memcmp(seqs[i].md5hash, md5hash, 32)==0)			// got a match ?
				return(i);
		}
	}
	return(-1);
}


// return the index of an unused entry in the seqs[] array or -1 if table is full
int seq_find_free_slot()
{
	int i=0;
	for (i=0;i<MAX_SEQ_SCRIPTS;i++)
	{
		if (seqs[i].num_lines==0)						// unused ?
			return(i);
	}
	return(-1);
}




// Run part of script, starting at timing mark N and finishing just before the next timing mark
void run_script_segment(int i, int framenumber)
{
	int startline=0;
	int endline=0;
	int line=0;
	int r=0;

	if (framenumber>MAX_TIMEMARKS)
		return;
	seqs[i].framenumber=framenumber;
	startline=seqs[i].timemark_line[framenumber];					// line number we start running code
	endline=seqs[i].timemark_lastline[framenumber];					// line number where this timemark section ends

	if (startline<0 || endline<0)
		return;
	if (startline==0 && endline==0)
		return;

	//printf("i=%d\tframenumber=%d\tstartline=%d\tendline=%d\n",i,framenumber,startline,endline);
	for (line=startline; line<=endline; line++)
	{
		//printf("line=%d length=%d [%s]\n",line,seqs[i].line_length[line],seqs[i].script+seqs[i].line_offsets[line]);	
		if (seqs[i].debug==TRUE)
		{
			printf("%02d:%06d [%s]\n",i,line,seqs[i].script+seqs[i].line_offsets[line]);
			fflush(stdout);
		}
		r=interpreter(-1, seqs[i].script+seqs[i].line_offsets[line], seqs[i].line_length[line], FALSE);
		if (r==0)
		{
		}
	}
	fflush(stdout);
}





// Audio is received at the rate of around 86 frames a second. Match the md5sum with a loaded sequence
// file and execute the code
void seq_process_audioframe(char *md5hash, int framenumber)
{
	int i=0;

	//printf("got audio frame %d\n",framenumber);
	i=seq_match_md5(md5hash);							// match a loaded script with audio
	if (i>=0)
	{										// got a match
		//printf("script %d matches md5, framenumber=%d\n",i,framenumber);
		if (seqs[i].framenumber < seqs[i].end_timemark)				// time mark is within range of script
			run_script_segment(i, framenumber);	
		else
		{
			seqs[i].framenumber=-1;						// script has finished playing
		}
	}
}




// look for MD5= string in file text (now in RAM).  Return true if found, false if not
int seq_script_find_md5(int i)
{
	int l=0;

	for (l=0;i<seqs[i].scriptsize-strlen("MD5=");l++)
	{
		if ( (seqs[i].script[l]=='M') && (seqs[i].script[l+1]=='D') && 
		     (seqs[i].script[l+2]=='5') && (seqs[i].script[l+3]=='=') )
		{
			memcpy(seqs[i].md5hash, seqs[i].script+l+4, 32);
			return(TRUE);
		}
	}
	return(FALSE);
}



// Mark the offsets of into the script buffer for each line. Return number of lines or negative for error
// splits buffer into a set of zero terminated strings
// If file is CR LF terminated then it will have two ASCII 0s between lines, terminated CR or LF alone then just one.
// Blank lines are removed by this process.
int seq_script_line_offsets(int i)
{
	int numlines=0;
	int c=0;
	int mark=FALSE;

	seqs[i].line_offsets[0]=0;						// Offset of first line is always 0
	for (c=0;c<seqs[i].scriptsize;c++)					// for every character of the script
	{
		if ( (seqs[i].script[c]==10) | (seqs[i].script[c]==13) )	// New line ?
		{
			seqs[i].script[c]=0;					// terminate line
			mark=TRUE;						// probably start of next line
		}
		else								// Not the end of a line?
		{
			if (mark==TRUE)						// Was recently the end a line?
			{
				mark=FALSE;					// cancel the condition
				if (numlines<MAX_SEQ_LINES)
					seqs[i].line_offsets[numlines++]=c;	// Record offset into buffer of start of a line
				else	return(-1);				// ran out of room to store index marks
			}
		}
	}
	if (numlines<=0)
		numlines=-1;							// no lines is an error
	return(numlines);
}


/*
// Populate an array of offsets with the offset of the start of each line of text.
// Returns the number of lines within the script
int seq_script_line_offsets(int i)
{
	int numlines=0;
	int c=0;
	int sl=0;								// start of line

	seqs[i].line_offsets[0]=0;						// Offset of first line is always 0
	sl=0;
	for (c=0;c<seqs[i].scriptsize;c++)					// for every character of the script
	{
		if ( (seqs[i].script[c]==10) | (seqs[i].script[c]==13) )	// New line ?
		{
			seqs[i].script[c]=0;					// terminate line
			c++;
			if ((seqs[i].script[c]==10) | (seqs[i].script[c]==13) )
			{
				seqs[i].script[c]=0;				// terminate line
				c++;
			}

			if (numlines<MAX_SEQ_LINES)
				seqs[i].line_offsets[numlines++]=sl;		// Record offset into buffer of start of a line
			else	return(-1);					// ran out of room to store index marks
			sl=c;
		}
	}
	if (numlines<=0)
		numlines=-1;							// no lines is an error
	return(numlines);
}
*/


// Remove leading spaces or tabs  and leading time marks from line
void seq_script_trimline(int i)
{
	int line=0;
	int offset=0;
	char c;

	for (line=0;line<seqs[i].num_lines;line++)				// for every line of the script
	{
		offset=seqs[i].line_offsets[line];
		c = seqs[i].script[offset];					// first character of line
		if (c!='#')
		{
			while (c<=57)
			{
				c = seqs[i].script[offset];
				//printf("%c",c); fflush(stdout);
				offset++;
			}
			seqs[i].line_offsets[line] = offset-1;
		}
		seqs[i].line_length[line]=strlen(&seqs[i].script[seqs[i].line_offsets[line]]);		// record the line length
	}
}



// Record the position of each timing mark in the script, returns the number of timing marks found
int seq_script_timemark_lines(int i)
{
	int line=0;
	int nummarks=0;
	int tmark=-1;
	int ptmark=0;

	nummarks=0;
	for (line=0;line<seqs[i].num_lines;line++)					// for every line of the script
	{
		//printf("line=%d [%s]\n",line,seqs[i].script+seqs[i].line_offsets[line]);	
		tmark=-1;
		if (seqs[i].script[seqs[i].line_offsets[line]]!='#')			// not a comment line ?
		{
			sscanf(seqs[i].script+seqs[i].line_offsets[line],"%d ",&tmark);	
			if (tmark>0 && tmark<MAX_TIMEMARKS)
			{
				seqs[i].timemark_lastline[ptmark]=line-1;		// line before this ends last section of script
				ptmark=tmark;
				seqs[i].timemark_line[tmark]=line;			// this line number starts with a timing mark
				//printf("%d[%s]\n",tmark,seqs[i].script+seqs[i].line_offsets[line]);	
				nummarks++;

				if (seqs[i].end_timemark < tmark)			// mark is script larger than current value
					seqs[i].end_timemark=tmark;			// this is now the largest time mark
			}
		}
	}
	return(nummarks);
}



// Returns index of sequence array or -1 if failed. 
int seq_load(int textout, char* fname)
{
	struct stat 	buf;
	char		filename[MAX_FILENAMELEN];
	int		fd=-1;
	int		i=-1;
	int		bytesread=0;

	bzero(&filename,sizeof(filename));
	sprintf(filename,"seq/%s",fname);

	i=seq_find_free_slot();
	if (i<0)
	{
		xprintf(textout,"Err, sequence table full\n");
		return(-1);
	}

	fd=open(filename,O_RDONLY);
	if (fd<0)
	{
		xprintf(textout,"Err, %s\n",strerror(errno));
		return(-1);
	}
	fstat(fd,&buf);
	seqs[i].scriptsize=buf.st_size;
	if (seqs[i].scriptsize<=0)
	{
		xprintf(textout,"Err, Empty file\n",strerror(errno));
		seq_clear(i);
		return(-1);
	}
	seqs[i].script=malloc(seqs[i].scriptsize);

	bytesread=read(fd,seqs[i].script,seqs[i].scriptsize);
	if (bytesread<=0)
	{
		xprintf(textout,"Err, %s\n",strerror(errno));
		seq_clear(i);
		return(-1);
	}

	strcpy(seqs[i].filename,fname);				// filename without path
	if (seq_script_find_md5(i)!=TRUE)
	{
		xprintf(textout,"Err, file %s does not contain an MD5= line\n",filename);
		seq_clear(i);
		return(-1);
	}
	seqs[i].num_lines=seq_script_line_offsets(i);
	if (seqs[i].num_lines<=0)
	{
		xprintf(textout,"Err, file %s, Failed to parse line start offsets\n",filename);
		seq_clear(i);
		return(-1);
	}
	seqs[i].num_timemarks=seq_script_timemark_lines(i);
	if (seqs[i].num_timemarks<=0)
	{
		xprintf(textout,"Err, file %s, Failed to parse timing marks\n",filename);
		seq_clear(i);
		return(-1);
	}
	seq_script_trimline(i);
	return(i);
}


// How many sequences are currently loaded
int seq_count()
{
	int i=0;
	int c=0;

	for (i=0;i<MAX_SEQ_SCRIPTS;i++)
	{
		if (seqs[i].num_lines!=0)
			c++;
	}
	return(c);
}


int seq_remove(int textout)
{
	return(FALSE);
}



int seq_test(int textout)
{
	return(FALSE);
}



int seq_step(int textout)
{
	return(FALSE);
}



void seq_list(int textout)
{
	int i=0;
	int c=0;

	c=seq_count();
	xprintf(textout,"%d sequences loaded\n",c);
	if (c<=0)
		return;

	for (i=0;i<MAX_SEQ_SCRIPTS;i++)
	{
		if (seqs[i].num_lines>0)
		{
			xprintf(textout,"%02d:\tnon blank lines:%d\ttimemarks:%d\tAudio MD5:%s\tfilename:%s\n",
				i, seqs[i].num_lines, seqs[i].num_timemarks, seqs[i].md5hash, seqs[i].filename);
		}
	}
}



void seq_debug(int textout, int torf)
{
	int i=0;

	for (i=0;i<MAX_SEQ_SCRIPTS;i++)
		seqs[i].debug = torf;
	if (torf==TRUE)
	{
		xprintf(textout,"Debug on for all currently loaded sequences\n");
		return;
	}
	xprintf(textout,"Debug off for all currently loaded sequences\n");
}


int seqrunsequence_run(int textout, int s)
{
	if (s>MAX_SEQ_SCRIPTS || s<0)
	{
		xprintf(textout,"Err, sequence number out of range\n");
		return(-1);
	}
	if (seqs[s].num_lines<=0)
	{
		xprintf(textout,"Seq %d not in use\n",s);
		return(-1);
	}
	xprintf(textout,"Running sequence %d\n",s);
	return(s);									// sequence number is ok
}


