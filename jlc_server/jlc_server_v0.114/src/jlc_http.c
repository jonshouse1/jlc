/*
	jlc_http.c
	mini crappy WWW server with CGI like feature 

	To work over the internet proper it needs to accept and service
	connections in parallel
*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>

#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_prototypes.h"

#define PAGE_NONE	0				// May mean an error
#define PAGE_HTM	1
#define PAGE_JPG	2
#define PAGE_PNG	3
#define PAGE_GIF	4
#define PAGE_JS		5
#define PAGE_TXT	6
#define PAGE_PDF	7
#define PAGE_BIN	10


extern struct    universe* uni[MAX_UNIVERSE+1];
extern uint16_t  session_id;

extern struct jlc_devf          devf[MAX_DEVF];
extern struct jlc_devn          devn[MAX_DEVN];              

int wwwfd=-1;
struct sockaddr_in serv_addr;
struct sockaddr_in new_addr;
char reqbuf[8192];
char wwwpath[]="./www\0";


void http_datetime(char* buf, int blen)
{
	//static char buf[1000];
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	strftime(buf,blen, "%a, %d %b %Y %H:%M:%S %Z", &tm);
}


// lookup variable by name, return value as text string
void http_varparse(char *vname, char *value)
{
	char st[2048];
	char name[2048];
	int len=strlen(vname);
	int i=0;
	int a=0;
	int b=0;
	int v=0;

	bzero(st,sizeof(st));
	strcpy(value,st);								// failsafe, return blank string
	if (len<=0)
		return;

	for (i=0;i<len;i++)								// Convert to upper case
		name[i]=toupper(vname[i]);

	//printf("name=[%s]\n",name); fflush(stdout);
	if (name[0]!='$' || len<3)
		return;
	if (name[1]=='U')
	{
// TODO add way to read mask as well as value
		sscanf(vname,"$U%dC%d",&a,&b);
		v=universe_return_ch_value(-1, a, b);
		if (v>=0)
			sprintf(st,"%d",uni[a]->ch[b].value);
		else	strcpy(st,"Err");
	}
	strcpy(value,st);								// return text to caller
}



// Setup tcp port for listening, return socket file descriptor if it works, return -1 on error
int init_http(int http_tcp_port)
{
        wwwfd = socket(AF_INET, SOCK_STREAM, 0);
        memset(&serv_addr, '0', sizeof(serv_addr));
        bzero(&serv_addr,sizeof(struct sockaddr_in));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(http_tcp_port);

        // If we are in a TCPWAIT for the socket we can bypass the process and use it again
        int y = 1;
        if ( setsockopt(wwwfd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) == -1 )
        {
		printf("http_init, setsockopt() failed, %s\n",strerror(errno));
		return(-1);
	}
        if (bind(wwwfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <0)
        {
		printf("http init, bind() failed, %s\n",strerror(errno));
		return(-1);
        }
        listen(wwwfd, 1);
        if (fcntl(wwwfd,F_SETFL,O_NDELAY) <0)
        {
		printf("http_init, setting No delay failed, %s\n",strerror(errno));
		return(-1);
        }
        int flags = fcntl(wwwfd,F_GETFL,0);
        if (fcntl(wwwfd, F_SETFL, flags | O_NONBLOCK)<0)
        {
		printf("http_init, set non blocking failed, %s\n",strerror(errno));
		return(-1);
        }
	printf("Listening for http on port %d\n",http_tcp_port); fflush(stdout);
        return(wwwfd);
}



// Handle new tcp connect
void http_newconn(int wconfd, char* ipaddr)
{
	//printf("IP %s connected\n",ipaddr);
	//fflush(stdout);
	bzero(&reqbuf,sizeof(reqbuf));

}


		
// Return the size of a file 
unsigned int http_getfilesize(char *url)
{
	FILE *fp=NULL;
	unsigned int size=0;
	char filename[MAX_FILENAMELEN];

	sprintf(filename,"%s/%s",wwwpath,url);
	fp=fopen(filename,"r");
	if (fp==NULL)
		return(0);
	fseek(fp, 0, SEEK_END); // seek to end of file
	size = ftell(fp);
	return(size);
}



char *print_content_type(int ct)
{
	static char cts[1024];

	cts[0]=0;
	switch (ct)
	{
		case PAGE_HTM:	sprintf(cts,"Content-Type: text/html; charset=UTF-8");	break;
		case PAGE_JPG:	sprintf(cts,"Content-Type: text/html; charset=UTF-8");	break;
		case PAGE_PNG:	sprintf(cts,"Content-Type: image/jpeg;");		break;
		case PAGE_GIF:	sprintf(cts,"image/png");				break;
		case PAGE_JS:	sprintf(cts,"image/gif");				break;
		case PAGE_TXT:	sprintf(cts,"text/plain");				break;
		case PAGE_PDF:	sprintf(cts,"application/pdf");				break;
		case PAGE_BIN:	sprintf(cts,"application/octet-stream");		break;
		default:	sprintf(cts,"Content-Type: text/html; charset=UTF-8");	break;
	}
	return((char*)&cts);
}



// Given a URL work out what the content type should be
int http_content_type(char* url)
{
	char ext[32];
	int len=strlen(url);
	int i=0;
	int didx;
	int c=0;
	//static char ct[1024];
	int ct=0;

	bzero(&ext,sizeof(ext));
	//bzero(&ct,sizeof(ct));
	//sprintf(ct,"Content-Type: text/html; charset=UTF-8");
	//if (len<=0)
		//return((char*)&ct);
	didx=-1;
	for (i=0;i<len;i++)
	{
		if (url[i]=='.')
			didx=i+1;	
	}
	if (didx<0)
		return(PAGE_NONE);

	c=0;
	for (i=didx;i<len;i++)
	{
		ext[c++]=toupper(url[i]);
	}

	ct=PAGE_HTM;
	//printf("extension = %s\n\n\n",ext); fflush(stdout);
	if (strncmp(ext,"HTM",3)==0)
		ct=PAGE_HTM;
	if (strncmp(ext,"JPG",3)==0)
		ct=PAGE_JPG;
	if (strncmp(ext,"PNG",3)==0)
		ct=PAGE_PNG;
	if (strncmp(ext,"GIF",3)==0)
		ct=PAGE_GIF;
	if (strncmp(ext,"JS",2)==0)
		ct=PAGE_JS;
	if (strncmp(ext,"TXT",3)==0)
		ct=PAGE_TXT;
	if (strncmp(ext,"PDF",3)==0)
		ct=PAGE_PDF;
	if (strncmp(ext,"BIN",3)==0)
		ct=PAGE_BIN;
	return(ct);
}


int http_output_header(int *wconfd, char *ipaddr, char*url, int response_code, int contentlen)
{
	int ct=0;
	int w=*wconfd;
	char dt[1024];

	//printf("contentlen=%d\n",contentlen);
	bzero(&ct,sizeof(ct));
	if (response_code==404)
		xprintf(w,"HTTP/1.1 %d Not found\n",response_code);
	if (response_code==200)
		xprintf(w,"HTTP/1.1 %d OK\n",response_code);
	ct=http_content_type(url);
	xprintf(w,"%s\n", print_content_type(ct));
	xprintf(w,"Server: jlc\n");
	xprintf(w,"Cache-Control: no-cache\n");
	xprintf(w,"Cache-Control: no-store\n");
	xprintf(w,"Cache-Control: no-transform\n");
	http_datetime((char*)&dt,sizeof(dt));	
	xprintf(w,"Date: %s\n",dt);
	if (contentlen>0 && ct!=PAGE_HTM)
	{								// not htm or html 
		xprintf(w, "Content-Length: %d\n",contentlen); 
		//printf("%s = length %d\n",url,contentlen); fflush(stdout);
	}
	xprintf(w,"\n");	
	return(ct);
}



// Outputs a standard header, then the requested page
void http_handle_page_request(int *wconfd, char* ipaddr, char *url)
{
	char filename[MAX_FILENAMELEN];
	int w=*wconfd;
	char* rawpagebuf=NULL;
	char* newpagebuf=NULL;
	int n=0;
	int x=0;
	int filefd=-1;
	off_t fsize=0;
	static char word[8192];
	static char worduc[8192];
	int i=0;
	int ct=0;

	int idx=0;
	int nidx=0;

	word[0]=0;
	worduc[0]=0;
	//printf("%s requested URL [%s]\n", ipaddr, url);  fflush(stdout);
	sprintf(filename,"%s/%s",wwwpath,url);
	filefd=open(filename,O_RDONLY);
	if (filefd<0)
	{
		monitor_printf(MSK_WWW,"http_handle_page_request() FILE:[%s] ERR:[%s]\n",filename,strerror(errno));
		ct=http_output_header(wconfd, ipaddr, url, 404, -1);
		xprintf(w,"\n<p>404 %s Not found </p>",filename);
		return;
	}

	fsize = lseek(filefd, 0, SEEK_END);
	rawpagebuf=malloc(fsize);									// Buffer for page
	newpagebuf=malloc(fsize*2);									// processed page
	if (rawpagebuf<0 || newpagebuf<0)
	{
		fprintf(stderr,"jlc_http.c: Error malloc failed for %ld bytes\n",fsize*2);
		return;
	}
	newpagebuf[0]=0;
	ct=http_output_header(wconfd, ipaddr, url, 200, fsize);

	lseek(filefd,0,SEEK_SET);
	n=read(filefd,rawpagebuf,fsize);								// Read entire file into RAM
	if (n<0)
	{
		fprintf(stderr,"jlc_http.c: Error %s\n",strerror(errno));
		return;
	}
	close(filefd);

	nidx=0;
	if (ct==PAGE_HTM)										// Only if an HTM page
	{
		for (i=0;i<n;i++)									// for every byte in html file
		{
			if (rawpagebuf[i]==' ')
			{
				word[idx++]=0;
				worduc[idx++]=0;
				for (x=0;x<strlen(word);x++)
					worduc[x]=toupper(word[x]);					// make an upper case version
				worduc[x]=0;
				if (variable_subs(worduc)==TRUE)					// A word was replaced with a variable
				{
					//printf("DID SUBS\n\n\n\n");
					strcpy(word,worduc);
				}
				//printf("word=[%s]  worduc[%s]\n",word,worduc);
				strcat(word," ");
				memcpy(newpagebuf+nidx,word,strlen(word));
				nidx=nidx+strlen(word);	
				idx=0;
			}
			else
			{
				if (rawpagebuf[i]>=33)
				{
					word[idx]=rawpagebuf[i];
					idx++;
				}
				else newpagebuf[nidx++]=rawpagebuf[i];
			}
		}
		newpagebuf[nidx++]=0;
		write(w,newpagebuf,nidx);	
	}
	else	write(w,rawpagebuf,fsize);								// NON html file, just output the file	
	monitor_printf(MSK_WWW,"HOST[%s] BYTES:%d URL[%s]\n",ipaddr,fsize,filename);
	free(rawpagebuf);
	free(newpagebuf);
}



// Parse the header to extract the URL
void http_parse_url(int *wconfd, char *ipaddr, char* reqbuf)
{
	char *ptr=NULL;
	static char url[2048];
	int i=0;

	//printf("%s\n",reqbuf); fflush(stdout);
	bzero(&url,sizeof(url));
	ptr=strstr(reqbuf,"GET");
	if (ptr==NULL)
		return;

	ptr=ptr+5;
	while (*ptr!=10 && *ptr!=13 && *ptr!=' ' && i<2000)
	{
		url[i++]=*ptr++;
	}	

	if (strcmp(url,"/")==0 || strlen(url)==0)
		strcpy(url,"index.html");
	http_handle_page_request(wconfd, ipaddr, url);
}



// Poll for http data, collect data, parse data when complete
// Kind of crappy, handles connections one at a time, reads then writes then closes for each...
void http_poll()
{
	static int wconfd=-1;
	int br=0;									// bytes read
	char buf[2];
	int n=0;
	static char ipaddr[64];
	struct sockaddr_in clientname;
	socklen_t size=sizeof(clientname);

	if (wconfd<0)									// if we don't currently have a connection
	{
		wconfd = accept (wwwfd,(struct sockaddr*)&clientname, &size);
		if (wconfd>0)								// got a new connection?
		{
			fcntl(wconfd, F_SETFL, O_NONBLOCK);				// make sure new socket in nonblock
			sprintf(ipaddr,"%s",(char*)inet_ntoa(clientname.sin_addr));	// take a note of the IP address
			http_newconn(wconfd, (char*)&ipaddr);				// handler
		}
		return;									// next time through we will start reading
	}
	else
	{
		do
		{
			n=read(wconfd,&buf,1);	
			if (n==EAGAIN || n==EWOULDBLOCK)				// if no data
			{
				printf("EA EW = %d\n",n); fflush(stdout);
				return;
			}

			if (n>0)
			{
				if (br>=sizeof(reqbuf)-1)
				{
					return;
					printf("too large\n"); fflush(stdout);
				}
				reqbuf[br]=buf[0];	
				br++;
			}
		} while (n>0);								// read all it has
		//printf("closed now, Read %d bytes \n%s\n",br,reqbuf);
		http_parse_url(&wconfd, (char*)&ipaddr, (char*)&reqbuf);
		close(wconfd);								// hangup on client
		wconfd=-1;
	}
}



