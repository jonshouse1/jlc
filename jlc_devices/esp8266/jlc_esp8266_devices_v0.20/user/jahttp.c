/*  jahttp.c - Memory file system based http server

    Version 0,92, May 2016 Jonathan Andrews (jon@jonshouse.co.uk )

    based on tcp_server.c by Tom Trebisky:  http://cholla.mmto.org/esp8266/OLD/sdk/tcp_server.c
    His original notes: http://cholla.mmto.org/esp8266/OLD/sdk/

    Flash memory file system by the genius that is Charles Lohr 
    Original link: https://github.com/cnlohr/esp8266ws2812i2s

    The CGI interpreter is very crude at the moment and has no relationship with the CGI specification!
    files name .cgi should be html files but also containing "$SOMEVARIABLE" references, these will be
    re-written with the variables value.  Note the "$SOMEVARIABLE" string must be padded to the length
    expected for the variable, for example "$SSID                         "
*/    


//#define DEBUG							// uncomment for lots of serial output
#define CGI_INSTEAD_OF_NOTFOUND					// uncomment this to run index.cgi rather than 404 message
#define INDEX_INSTEAD_OF_NOTFOUND				// uncomment to output index.html rather than a 404 message
#define MAX_CONNECTIONS			32	
//#define LED_ACTIVITY			4			// (GPIO) uncomment for an LED that lights when the http server is used



#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "mfs.h"
#include "mfs_filecount.h"
#include <gpio.h>
#include "common.h"


#define HT_STATE_IDLE			0
#define HT_STATE_CLOSE			1
#define HT_STATE_SENDING_FILE		3
#define HT_STATE_NOTFOUND		4

#define HT_CONTENT_TEXTHTML		0
#define HT_CONTENT_TEXTHTMLCGI		1
#define HT_CONTENT_JAVASCRIPT		2
#define HT_CONTENT_IMAGEJPEG		10
#define HT_CONTENT_IMAGEPNG		11
#define HT_CONTENT_IMAGEGIF		12
#define HT_CONTENT_UNKNOWN		13
#define HT_CONTENT_TEXTPLAIN		14
#define HT_CONTENT_PDF			15
#define HT_CONTENT_GZ			16

extern char jahttp_cgi_variables[][16];


int has_index_html=FALSE; 
int has_index_cgi=FALSE;
// We use an array of this to hold all the state for a connection
struct connection
{
	int	connected;									// 'socket' is active
	int 	remote_port;
	uint8 	remote_ip[4];
	int  	ht_content_type;			
	int  	ht_state;
	int  	ht_count;									// Which chunk of file or output
	char 	ht_filename[32];
	char 	ht_fh;										// File handle
	struct  MFSFileInfo mfs_fileinfo;
	int	bytes_sent;									// bytes sent for this file so far
};
struct connection connections[MAX_CONNECTIONS];							// Array of connections, index in the table acts as a handle


#ifdef LED_ACTIVITY
static volatile os_timer_t led_off_timer;
void ICACHE_FLASH_ATTR led_off_timer_cb( void )
{
    GPIO_OUTPUT_SET(LED_ACTIVITY,1);                                                            // LED off
}
#endif


// Prototypes
void ICACHE_FLASH_ATTR jahttp_cgi_variable_to_value(char *cgivar, int hasquote);
int ICACHE_FLASH_ATTR jahttp_cgi_post(int cgi_variables_idx, char *value, int len);
int percent_decode(char *in_buff, int in_len, char* out_buff, int out_len);




static bool is_hex_char(int c)
{
  return (isdigit(c) || (c <= 'f' && c >= 'a'));
}

static char hex_decode_char(uint8_t c)
{
  return isdigit(c) ? c - '0' : c - 'a' + 10;
}

static char hex_decode(uint8_t hi, uint8_t lo)
{
  return (hex_decode_char(hi) << 4) + hex_decode_char(lo);
}


// returns length used in the output buffer.
// returns -1 on error, either due to:
//  - malformed input
//  - unexpected end-of-buffer for input
//  - inexpected end-of-buffer for output 
int ICACHE_FLASH_ATTR percent_decode(char *in_buff, int in_len, char* out_buff, int out_len)
{
	int in_pos, out_pos;
	char c;
	char h_hi;
	char h_lo;
	in_pos = out_pos = 0;
	while(in_pos < in_len)
	{
		if(out_pos >= out_len)
			return -1;

		c = in_buff[in_pos++];
		if(c == '%')
		{		
			if(!(in_pos + 1 < in_len))
				return -1;
	  		h_hi = tolower(in_buff[in_pos++]);
	  		h_lo = tolower(in_buff[in_pos++]);
	  		if(!is_hex_char(h_hi) || !is_hex_char(h_lo))
	    			return -1;
	  		out_buff[out_pos++] = hex_decode(h_hi, h_lo);
		}
      		else
		{
	  	out_buff[out_pos++] = c;
		}
    	}
  	return out_pos;
}






void ICACHE_FLASH_ATTR jahttp_overwrite(char *cgivar, char *value, int hasquote, int len)
{
        int i,c;
        int s;

        //os_printf("overwrite %c%c%c%c (len=%d)\n",cgivar[0],cgivar[1],cgivar[2],cgivar[3],len);
        s=strlen(value);
	if (s>len) s=len;
        for (i=0;i<s;i++)
                cgivar[i]=value[i];								// overwrite $MYVAR with MYVAL

	if (hasquote==TRUE)
        	cgivar[i++]='\"';								// add end quote

        for (c=i;c<len;c++)									// pad with spaces, overwrite previous "
                cgivar[c]=' ';									// when testing change space to an 'x'
}


// Find "$SOMEVAR" and replace with "SOMEVALUE"
void ICACHE_FLASH_ATTR cgi_variable_substitute(uint8 *data, int len)
{
	int c,i,l,x;
	int sq=0;										// start quote position

	for (i=0;i<len;i++)
	{
		if (data[i]=='$')								// look for $
		{
			if (i>=1)
			{
				if (data[i-1]=='\"')						// Is it "$ ?
					jahttp_cgi_variable_to_value((char*)data+i,TRUE);	// then tell everyone it is in the form "$MYVAR"
				else	jahttp_cgi_variable_to_value((char*)data+i,FALSE);	// else it is in the form $MYVAR
			}
		}
	}
}



// ***** audit this and jahttp_cgi_post() to ensure we never go off the end of the buffer
// returns the index of the last cgi variable processed.
// BUG - Cuts off the last character of the value= field of the last variable posted, IE F
int ICACHE_FLASH_ATTR post_variable_process(char *data, int len)
{
	int i;
	int v;
	int x;
	int c=0;
	int l=0;										// length of VALUE part of variable=value
	int r=0;

	//os_printf("post_variable_process len=%d [%s]\n",len,data);
	r=-1;
	for (i=0;i<len;i++)									// move on byte at a time forward through post data
	{
		for (v=0;v<NO_jahttp_cgi_variables;v++)						// for each variable in the list
		{
			if (i <= (len-strlen(jahttp_cgi_variables[v])) )
			{
				//os_printf("looking for %s at %d\n",jahttp_cgi_variables[v],i);
				if (strncmp(data+i,jahttp_cgi_variables[v],strlen(jahttp_cgi_variables[v]))==0)
				{
					c=0;
					for (x=i+strlen(jahttp_cgi_variables[v]);x<len;x++)					// Look for the end of the =VALUE part
					{
						c++;
						if ( (data[x]=='&') || (data[x]==13) || (data[x]==10) )
						{
							data[x]=0;		// JA new
							break;
						}
					}

					//os_printf("Calling jahttp_cgi_post  %c%c%c \n",data[i+strlen(jahttp_cgi_variables[v])-1],data[i+strlen(jahttp_cgi_variables[v])],
										       //data[i+strlen(jahttp_cgi_variables[v])+1]);

					r=jahttp_cgi_post(v, data+i+strlen(jahttp_cgi_variables[v]),c--);
				}
			}
		}
	}
	return(r);
}




#ifdef DEBUG
static volatile os_timer_t sh_timer;
void ICACHE_FLASH_ATTR sh_timer_cb( void )
{
	int i;
	os_printf("%c[2J;%c[1;1H",27,27);
	
	for (i=0;i<MAX_CONNECTIONS;i++)
	{
		if (connections[i].connected==TRUE)
		{
			os_printf("%d\t%d%d%d%d\t%d\n",i,connections[i].remote_ip[0],
							 connections[i].remote_ip[1],
							 connections[i].remote_ip[2],
							 connections[i].remote_ip[3]);
		}
	}
}
#endif





// Due to a rather poor connection API we don't seem to have a unique handle for each connection made, we have to make a handle using the ip and port
// of the host.  We might as well store all the state we need in the handle table.
#define HANDLE_REMOVE		1
#define HANDLE_CREATE		2
#define HANDLE_LOOKUP		3
// Returns -1 on error
// For each connection the remote_port and its ip identify it as unique.  Turn these ip/port pairs into a handle
int ICACHE_FLASH_ATTR connectionhandle(void *arg, int action)
{
	struct espconn *conn = arg;
	int i;
	int f;


	// Did not exist in table, optionally try and add it
	if (action==HANDLE_CREATE)
	{
		for (i=0;i<MAX_CONNECTIONS;i++)
		{
			if (connections[i].connected!=TRUE)					// this line is not in use ?
			{
				connections[i].connected=TRUE;					// mark it as in use now
				connections[i].remote_port =conn->proto.tcp->remote_port;
				connections[i].remote_ip[0]=conn->proto.tcp->remote_ip[0];
				connections[i].remote_ip[1]=conn->proto.tcp->remote_ip[1];
				connections[i].remote_ip[2]=conn->proto.tcp->remote_ip[2];
				connections[i].remote_ip[3]=conn->proto.tcp->remote_ip[3];
				//os_printf("Create handle %d for %d.%d.%d.%d port %d\n",i,connections[i].remote_ip[0],
											 //connections[i].remote_ip[1],
											 //connections[i].remote_ip[2],
											 //connections[i].remote_ip[3],
											 //connections[i].remote_port);
//os_printf("test %d\n",conn->proto.tcp->local_ip[0]);
				return(i);							// tell caller the table index (handle)
			}
		}
	}


	// Look for an existing line in the table that matches the passed ip and port, update the table entry to reflect new state
	f=-1;
	for (i=0;i<MAX_CONNECTIONS;i++)
	{
		if ( (connections[i].connected == TRUE ) && 
		     (connections[i].remote_port  == conn->proto.tcp->remote_port) && 
		     (connections[i].remote_ip[0] == conn->proto.tcp->remote_ip[0] ) && 
		     (connections[i].remote_ip[1] == conn->proto.tcp->remote_ip[1] ) && 
		     (connections[i].remote_ip[2] == conn->proto.tcp->remote_ip[2] ) && 
		     (connections[i].remote_ip[3] == conn->proto.tcp->remote_ip[3] ) )
		{
			f=i;									// Found entry at this index
			connections[i].connected=TRUE;
			connections[i].remote_port =conn->proto.tcp->remote_port;
			connections[i].remote_ip[0]=conn->proto.tcp->remote_ip[0];
			connections[i].remote_ip[1]=conn->proto.tcp->remote_ip[1];
			connections[i].remote_ip[2]=conn->proto.tcp->remote_ip[2];
			connections[i].remote_ip[3]=conn->proto.tcp->remote_ip[3];
			if (action==HANDLE_LOOKUP)						// caller just wants the handle
				return(i);
			break;
		}
	}

	if (f<0)
		return (-1);									// Did not find table index from ip/port 

	if ( action==HANDLE_REMOVE )
	{
		connections[f].connected=FALSE;
		connections[f].ht_state=HT_STATE_IDLE;
		connections[f].ht_count=0;
		connections[f].bytes_sent=0;
		connections[f].ht_content_type=0;
		connections[f].ht_filename[0]=0;
		//os_printf("Releasing handle %d\n",f);
		return (f);									// return the handle just invalidated
	}
} 



char url[65];
//void show_ip ( void );

void ICACHE_FLASH_ATTR show_mac ( void )
{
    unsigned char mac[6];

    wifi_get_macaddr ( STATION_IF, mac );
    os_printf ( "MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
	mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
}

/* Could require up to 16 bytes */
char * ICACHE_FLASH_ATTR ip2str ( char *buf, unsigned char *p )
{
    os_sprintf ( buf, "%d.%d.%d.%d", p[0], p[1], p[2], p[3] );
    return buf;
}

#ifdef notdef
/* Could require up to 16 bytes */
char * ICACHE_FLASH_ATTR ip2str_i ( char *buf, unsigned int ip )
{
    int n1, n2, n3, n4;

    n1 = ip & 0xff;
    ip >>= 8;
    n2 = ip & 0xff;
    ip >>= 8;
    n3 = ip & 0xff;
    ip >>= 8;
    n4 = ip & 0xff;
    os_sprintf ( buf, "%d.%d.%d.%d", n1, n2, n3, n4 );
    return buf;
}
#endif

void ICACHE_FLASH_ATTR show_ip ( void )
{
    struct ip_info info;
    char buf[16];

    wifi_get_ip_info ( STATION_IF, &info );
    // os_printf ( "IP: %08x\n", info.ip );
    // os_printf ( "IP: %s\n", ip2str_i ( buf, info.ip.addr ) );
    os_printf ( "IP: %s\n", ip2str ( buf, (char *) &info.ip.addr ) );
}



void ICACHE_FLASH_ATTR send_header (void *arg, int h)
{
	struct espconn *conn = arg;
	char header[1024];
	char st[32];
	//os_printf("sending header: ht_content_type=%d, length=%d\n",connections[h].ht_content_type, connections[h].mfs_fileinfo.filesize );

	os_sprintf(header,"HTTP/1.1 200 OK\r\nServer: ESP8266\r\n");
	os_sprintf(st,"Content-Type: ");      
	strcat(header,st); 
	switch (connections[h].ht_content_type)
	{
		case HT_CONTENT_TEXTHTML:	os_sprintf(st,"text/html");	break;
		case HT_CONTENT_TEXTHTMLCGI:	os_sprintf(st,"text/html");	break;
		case HT_CONTENT_IMAGEJPEG:	os_sprintf(st,"image/jpeg\r\nPragma: no-cache");	break;
		case HT_CONTENT_IMAGEPNG:	os_sprintf(st,"image/png\r\nPragma: no-cache");	break;
		case HT_CONTENT_IMAGEGIF:	os_sprintf(st,"image/gif\r\nPragma: no-cache");	break;
		case HT_CONTENT_JAVASCRIPT:	os_sprintf(st,"application/javascript\r\nPragma: no-cache");	break;
		case HT_CONTENT_UNKNOWN:	os_sprintf(st,"unknown");	break;
		case HT_CONTENT_TEXTPLAIN:	os_sprintf(st,"text/plain");	break;
		case HT_CONTENT_PDF:		os_sprintf(st,"application/pdf"); break;
		case HT_CONTENT_GZ:		os_sprintf(st,"application/octet-stream"); break;	// downloading via browser
	}
	strcat(header,st); 
	strcat(header,"\r\n");
	if (connections[h].mfs_fileinfo.filesize>0)
	{
		os_sprintf(st,"Content-Length: %d\r\n",connections[h].mfs_fileinfo.filesize);	
		strcat(header,st); 
	}
	os_sprintf(st,"Connection: close\r\n\r\n");							// Also single blank line signify end of header      
	strcat(header,st); 
	espconn_send ( arg, (uint8 *)&header, strlen(header));
}


int ICACHE_FLASH_ATTR detect_content_type(char * ht_filename)
{
	int i;
	char lastthree[4];

	i=strlen(ht_filename);
	if (i>3)
	{
		lastthree[0]=tolower(ht_filename[i-3]);
		lastthree[1]=tolower(ht_filename[i-2]);
		lastthree[2]=tolower(ht_filename[i-1]);
		lastthree[3]=0;

		//os_printf("lastthree=[%s]\n",lastthree);
		if (strcmp(lastthree,"tml")==0)
			return HT_CONTENT_TEXTHTML;
		if (strcmp(lastthree,"htm")==0)
			return HT_CONTENT_TEXTHTML;
		if (strcmp(lastthree,".js")==0)
			return HT_CONTENT_JAVASCRIPT;
		if (strcmp(lastthree,"cgi")==0)
			return HT_CONTENT_TEXTHTMLCGI;
		if (strcmp(lastthree,"jpg")==0)
			return HT_CONTENT_IMAGEJPEG;
		if (strcmp(lastthree,"png")==0)
			return HT_CONTENT_IMAGEPNG;
		if (strcmp(lastthree,"gif")==0)
			return HT_CONTENT_IMAGEGIF;
		if ( (strcmp(lastthree+1,".c")==0) | (strcmp(lastthree+1,".h")==0) )
			return HT_CONTENT_TEXTPLAIN;
		if (strcmp(lastthree,"pdf")==0)
			return HT_CONTENT_PDF;
		if (lastthree[1]=='g' && lastthree[2]=='z')
			return HT_CONTENT_GZ;
	}
	return HT_CONTENT_TEXTHTML;		
}




// Send one chunk of a file
//struct MFSFileInfo      mfs_fileinfo;
void ICACHE_FLASH_ATTR send_file_chunk(void *arg)
{
    int h;
    char asector[MFS_SECTOR];
    int bytesleft=0;
    int cs=MFS_SECTOR;
    int r;

#ifdef LED_ACTIVITY
    GPIO_OUTPUT_SET(LED_ACTIVITY,0);                                                                    // LED on
#endif

    h=connectionhandle(arg, HANDLE_LOOKUP);								// Find our connection details
    if (h<0)
	return;

    bytesleft = MFSReadSector( &asector[0], &connections[h].mfs_fileinfo );				// It always reads MFS_SECTOR size chunks
    if (bytesleft<=0)
	connections[h].ht_state=HT_STATE_CLOSE;								// Hangup socket

//BUG/TODO:  At the moment we are only parsing within a 256 byte chunk, we need to parse across chunks somehow
    if (connections[h].ht_content_type==HT_CONTENT_TEXTHTMLCGI)						// if this file is cgi 
    	cgi_variable_substitute((uint8 *)&asector[0],MFS_SECTOR);					// replace "$SOMEVAR" with "somevalue"

    if (connections[h].mfs_fileinfo.filelen<=0)								// Last block to send
	cs=connections[h].mfs_fileinfo.filesize - connections[h].bytes_sent;

    espconn_send ( arg, (uint8 *)&asector[0], cs);							// send next chunk of tcp data
    connections[h].bytes_sent = connections[h].bytes_sent + cs;
}




// Send a file back to the caller, auto detect content type based on extension
void ICACHE_FLASH_ATTR sendafile(void *arg, int h, char *filename)
{
    strcpy(connections[h].ht_filename,filename);
    connections[h].ht_state=HT_STATE_SENDING_FILE;
    connections[h].ht_count=0;
    connections[h].bytes_sent=0;
    connections[h].ht_content_type=detect_content_type((char*)&connections[h].ht_filename);
    os_printf("jahttp sending [%s] type=%d\n",filename,connections[h].ht_content_type);

//NNN
    connections[h].ht_fh=MFSOpenFile(connections[h].ht_filename, &connections[h].mfs_fileinfo);
    if (connections[h].ht_fh<0)
    {
	connections[h].ht_state=HT_STATE_CLOSE;							// Hangup socket
	os_printf("send_file_chunk error\n");
	return;
    }
    send_header (arg, h);
}



// This is not the last word in code quality, it is possible a malformed URL could crash this although
// WDT would fire and it would reboot so life goes on !
void ICACHE_FLASH_ATTR tcp_receive_data ( void *arg, char *buf, unsigned short len )
{
    int h;
    int i;
    int u;
    int x;
    int getat=-1;
    int postat=-1;
    char st[65];
    int fsize;
    int n=0;
    int c=0;

    h=connectionhandle(arg, HANDLE_LOOKUP);							// Find our connection details
    if (h<0)
  	return;

    cgi_handler((char*)buf, len);								// let other code extract meaning it wants


    // The string we are after has the form "GET / HTTP/1.1"
    os_printf("tcp_receive_data got len %d [%s]\n",len,buf);
    for (i=0;i<len-4;i++)									// Find GET or POST in tcp message
    {
	if ( (buf[i]=='G') && (buf[i+1]=='E') && (buf[i+2]=='T') )
	{
		getat=i;	
		break;
	}
	if ( (buf[i]=='P') && (buf[i+1]=='O') && (buf[i+2]=='S') && (buf[i+3]=='T') )
	{
		postat=i;
		break;
	}
    }
    // Post may be split over multiple tcp data chunks. for now if the first 3 bytes are not GET then
    // we will assume its part of a post
    if ( (getat<0) && (buf[0]!='G') && (buf[1]!='E') && (buf[2]!='T') )
         postat=0;



    //os_printf("'GET' is at %d,  'POST' is at %d\n",getat,postat);
    bzero(&url,sizeof(url));
    if (postat>=0)										// Handle POST
    {
  	i=postat+6;										// Copy text after POST
	u=0;
    	while ( (i<len) && (buf[i]!=10) && (buf[i]!=13) && (buf[i]!='&') && (i<sizeof(url)) )
		url[u++]=buf[i++];
	while (url[u]!=' ' && u>0)
		u--;										// now walk backwards looking for a space
	url[u]=0;										// shorten string, remove HTTP/1.1
	if (post_variable_process(buf, len)==100)						// extract meaning from post variables
		strcpy(url,"reflash.html");							// system only has a few seconds left to send it though..

	sendafile(arg,h,url);									// Send web page back to browser
	return;
    }



    if (getat>=0)
    {
  	i=getat+5;										// Copy text after GET
	u=0;
    	while ( (i<len) && (buf[i]!=10) && (buf[i]!=13) && (buf[i]!='&') && (i<sizeof(url)) )
		url[u++]=buf[i++];
	while (url[u]!=' ' && u>0)
		u--;										// now walk backwards looking for a space
	url[u]=0;										// shorten string, remove HTTP/1.1
	//os_printf("GET URL=[%s]\n",url);

	// Match the URL with a file
	connections[h].ht_count=0;
	connections[h].bytes_sent=0;
        if ( (url[0]==0) || (url[0]=='?') )							// url line is blank or a ? on its own
	{
		if ( (has_index_cgi==TRUE) && (url[0]!='?') )					// if we have an index.html and we don't have a ? url 
		{
			sendafile(arg,h,"index.cgi");
			return;
		}
		if ( (has_index_html==TRUE) && (url[0]!='?') )					// if we have an index.html and we don't have a ? url 
		{
			sendafile(arg,h,"index.html");
			return;
		}
		sendafile(arg,h,"list");							// html file with a list of files in ./www
		return;
	}


	//os_printf("trying to match url\n");
	// Try and match URL with files stored in flash
	for (i=0;i<MFS_FILECOUNT;i++)								// For every file
	{
        	if ( MFSFileList(i+1, (char*)&st, &fsize )==0)
		{
			if (strcmp(st,url)==0)							// we have a match for a filename
			{
				//os_printf("Got match for file %s\n",url);
				sendafile(arg,h,url);
				return;
			}
		}
	}


	// If we are still here then file not found, so 404
	connections[h].ht_state=HT_STATE_NOTFOUND;
	connections[h].ht_filename[0]=0;

// .cgi has precedence over .html so test for that first
//JA
#ifdef CGI_INSTEAD_OF_NOTFOUND
	if (has_index_cgi==TRUE) 
	{
		os_printf("Sending index.cgi instead of 404\n");
		sendafile(arg,h,"index.cgi");
		return;
	}
#endif


#ifdef INDEX_INSTEAD_OF_NOTFOUND
	if (has_index_html==TRUE) 
	{
		os_printf("Sending index.html instead of 404\n");
		sendafile(arg,h,"index.cgi");
		return;
	}
#endif
	send_header (arg, h);
    }
}



void ICACHE_FLASH_ATTR tcp_send_data ( void *arg )
{
    //struct espconn *xconn = (struct espconn *)arg;
    int h;
    char buf[1024];
    char st[65];
    int fsize=0;
    int sendsize=0;

    //os_printf("tcp_send_data, ht_state %d, ht_count %d\n",ht_state,ht_count);
    h=connectionhandle(arg, HANDLE_LOOKUP);								// Find our connection details
    if (h<0)
  	return;


    buf[0]=0;
    switch (connections[h].ht_state)
    {
    	case HT_STATE_CLOSE:
		os_printf("Doing close %d\n",h);
		espconn_disconnect(arg);
    		h=connectionhandle(arg, HANDLE_REMOVE);	
		return;
	break;

	case HT_STATE_SENDING_FILE:
		send_file_chunk(arg);									// Send more file
		connections[h].ht_count++;
	break; 

	case HT_STATE_NOTFOUND:
		os_sprintf(st,"<html><head>\n404 Not found</head>\n");
    		espconn_send ( arg, (uint8 *)st, strlen(st)  );	
		os_printf("404 - Not found\n");
		connections[h].ht_state=HT_STATE_CLOSE;
		return;
	break;
    }

    espconn_send ( arg, (uint8 *)&buf, sendsize );							// send next chunk of tcp data
    if (connections[h].ht_state==HT_STATE_CLOSE)
    {
	espconn_disconnect(arg);
    	h=connectionhandle(arg, HANDLE_REMOVE);	
    }
}


 
// This is a TCP error handler
void ICACHE_FLASH_ATTR tcp_reconnect_cb ( void *arg, sint8 err )
{
    os_printf ( "TCP reconnect (error)\n" );
}


void ICACHE_FLASH_ATTR tcp_disconnect_cb ( void *arg )
{
    int h;
    struct espconn *conn = (struct espconn *)arg;

    h=connectionhandle(conn, HANDLE_REMOVE);
}

 

void ICACHE_FLASH_ATTR tcp_connect_cb ( void *arg )
{
    struct espconn *conn = (struct espconn *)arg;
    int h;
    char buf[16];

    //os_printf ( "TCP connect\n" );
    //os_printf ( "  remote ip: %s\n", ip2str ( buf, conn->proto.tcp->remote_ip ) );
    //os_printf ( "  remote port: %d\n", conn->proto.tcp->remote_port );

    h=connectionhandle(conn, HANDLE_CREATE);
    if (h<0)
          os_printf("Error trying to create new handle\n");
    //else  os_printf("Got handle %d\n",h);

    espconn_regist_recvcb( conn, tcp_receive_data );
    espconn_regist_sentcb( conn, tcp_send_data );
}



static struct espconn server_conn;
static esp_tcp my_tcp_conn;




void ICACHE_FLASH_ATTR init_jahttp( int port )
{
    register struct espconn *c = &server_conn;
    char *x;
    int i;
    char st[32];
    int fsize;

    c->type = ESPCONN_TCP;
    c->state = ESPCONN_NONE;
    my_tcp_conn.local_port=port;
    c->proto.tcp=&my_tcp_conn;

    espconn_regist_reconcb ( c, tcp_reconnect_cb);
    espconn_regist_connectcb ( c, tcp_connect_cb);
    espconn_regist_disconcb ( c, tcp_disconnect_cb);
    espconn_tcp_set_max_con(MAX_CONNECTIONS);

    if ( espconn_accept(c) != ESPCONN_OK ) {
	os_printf("Error starting server %d\n", 0);
	return;
    }

    /* Interval in seconds to timeout inactive connections */
    espconn_regist_time(c, 20, 0);

    // x = (char *) os_zalloc ( 4 );
    // os_printf ( "Got mem: %08x\n", x );


    // See if the memory file system image contains index.html ?
    for (i=1;i<=MFS_FILECOUNT;i++)
    {
        if ( MFSFileList(i, (char*)&st, &fsize )==0)
	{
		//os_printf("compare [%s][index.html]\n",st);
		if (strcmp(st,"index.html")==0)
			has_index_html=TRUE; 
		if (strcmp(st,"index.cgi")==0)
			has_index_cgi=TRUE;
	}
    }

    // Mark all handles unused
    for (i=0;i<MAX_CONNECTIONS;i++)
	connections[i].connected = FALSE; 

    os_printf ( "Server ready,\n");
    //if (has_index_html==TRUE)
	//os_printf("Using index.html from flash\n");
    os_printf("Listening for HTTP connections on port %d\n",my_tcp_conn.local_port );


#ifdef DEBUG
    os_timer_setfn(&sh_timer,(os_timer_t *)sh_timer_cb,NULL);
    os_timer_arm(&sh_timer,100,1);
#endif

#ifdef LED_ACTIVITY
    GPIO_OUTPUT_SET(LED_ACTIVITY,0);                    // low
    os_delay_us(2000);
    GPIO_OUTPUT_SET(LED_ACTIVITY,1);                    // gpio high
    os_delay_us(1000);
    os_timer_setfn(&led_off_timer,(os_timer_t *)led_off_timer_cb,NULL);
    os_timer_arm(&led_off_timer,500,1);
#endif


}


