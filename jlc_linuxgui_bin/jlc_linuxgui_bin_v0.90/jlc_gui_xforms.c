// jlc_gui_xforms.c

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


#include "mainform.h"
#include "form_terminal.h"
#include "jcp_protocol_devs.h"
#include "jlc_gui_xforms_form_scripts.h"
#include "jlc_gui_xforms_form_control.h"
#include "jlc_gui_xforms_form_devices.h"
#include "jlc_gui_xforms_form_colorselector.h"
#include "wavplay.h"


#include "jlc.h"
#include "jlc_group.h"
#include "jlc_prototypes.h"

void colorselector_handler(FL_OBJECT * obj, long data);


FL_COLOR		clr_highlight		= FL_RED;			// State is "on" or true
FL_COLOR		clr_active		= FL_GRAY;			// control is in active use but in an idle or off state
//FL_COLOR		clr_inactive		= FL_DARKBLUE;			// control is not available, no device assigned to it
//FL_COLOR		clr_inactive		= FL_RIGHT_BCOL;			// control is not available, no device assigned to it
FL_COLOR		clr_inactive		= FL_FREE_COL2;			// control is not available, no device assigned to it
FL_COLOR		clr_selected		= FL_DARKGREEN;


FD_mainform             *fd_mainform		= NULL;
FD_form_terminal        *fd_form_terminal	= NULL;
FD_form_scripts         *fd_form_scripts	= NULL;
FD_form_control         *fd_form_control	= NULL;
FD_form_devices         *fd_form_devices	= NULL;
FD_form_colorsel	*fd_form_colorsel	= NULL;
int                     slider_lockoutcounter[MAX_GROUP_VISUALS];               // Do not update slider while counter is active

extern char		server_datetime[512];
extern char		server_ipaddr[32];
extern int		online;							// connected to server ?
extern int		fd_monitor;
extern int		fd_binmsg;                         
int			selected_form=1;
int			font_height=0;
int			termlines=0;
int			scriptlines=0;
int			colorsel_active=-1;					// negative for off, positive is the group being modified
extern int		app_height;
extern int		app_width;
extern char		windowtitle[];
extern int		xserver_width;
extern int		xserver_height;
extern char*		binpayload;

extern int		feature_fullscreen;
extern int		feature_terminal;
extern int		feature_caption;
extern int		feature_manage_screen_blanking;
extern int		feature_unblank_for_sounds;				// turn screen blanker off when playing sound
extern int		feature_unblank_for_movement_sensor;
extern int		feature_show_switches;


// Two tables of device and device states
extern struct jlc_devf		devf[MAX_DEVF];
extern struct jlc_devn		devn[MAX_DEVN];

// Groups
extern struct jlc_group_cfg	grpc[MAX_GROUPS+1];
extern struct jlc_group_val	grpv[MAX_GROUPS+1];

// Table for timed events
extern struct timed_events	te[MAX_TIMED_EVENTS];



// Ensure buffer contains only the last "lines"
void trimbuf( char* txt, int txtsize, int lines)
{
	char b1[16384];
	int i=0;
	int l=0;
	int cutpoint=0;
	int s=0;

	// Work from end of buffer towards start
	bzero(&b1,sizeof(b1));
	for (i=txtsize-1;i>=0;i--)
	{
		//if (txt[i]!='\n' && txt[i]!=0)
		if (txt[i]!=0)
			s++;
		if (txt[i]=='\n')
			l++;											// found another line
		if (l==lines)											// line cap reached ?
		{
			cutpoint=i;
		}
	}
	if (l<lines)
		return;

	//printf("copied %d bytes\n",s); fflush(stdout);
	memcpy(&b1,txt+cutpoint,s);
	bzero(txt,txtsize);
	memcpy(txt,&b1,s);
}


#define TABSIZE	8												// Tab stops every N chars
void expand_tabs(char *txt, int txtsize)
{
	char b1[1024*64];
	int b1idx=0;
	int i=0;
	int count=0;
	int x=0;

	// Walk through buffer copying into new buffer and expanding spaces as we go
	bzero(&b1,sizeof(b1));
	for (i=0;i<txtsize;i++)
	{
		if (txt[i]==0)
		{
			memcpy(txt,&b1,b1idx+1);
			return;
		}
		if (txt[i]=='\t')
		{
			for (x=count;x<TABSIZE;x++)
				b1[b1idx++]=' ';
			count=0;
			//printf("x=%d\n",x);
		}
		else	
		{
			b1[b1idx++]=txt[i];
			count++;
			if (count>=TABSIZE)
				count=0;
		}
		if (txt[i]=='\n' || txt[i]=='\r')
			count=0;
		//printf("i=%d b1idx=%d\n",i,b1idx); fflush(stdout);
	}
	memcpy(txt,&b1,b1idx+1);
}




// This form is for the foldertab to sit on
FD_mainform *create_form_mainform( void )
{
	FL_OBJECT *obj;
	FD_mainform *fdui = ( FD_mainform * ) fl_malloc( sizeof *fdui );

	fdui->vdata = fdui->cdata = NULL;
	fdui->ldata = 0;
	fdui->mainform = fl_bgn_form( FL_NO_BOX, app_width, app_height );

	fl_mapcolor(FL_FREE_COL2, 35, 30, 35);									// RGB, very dark gray
	fdui->preset_label = obj = fl_add_text( FL_NORMAL_TEXT, app_width-210, 0, 400, 16, "" );
	fl_set_object_lsize( obj, FL_SMALL_SIZE);								// Also could be FL_TINY_SIZE
	fl_set_object_color( obj, FL_BLACK, FL_BLACK );
	fl_set_object_lcolor( obj, FL_WHITE );									// label color for tabs

	fdui->tabs = obj = fl_add_tabfolder( FL_TOP_TABFOLDER, 0, 0, app_width, app_height, "" );
	fl_set_object_callback( obj, tabs_cb, 0 );
	fl_set_object_color( obj, FL_GREY, FL_BLACK );
	fl_set_object_lcolor( obj, FL_WHITE );									// label color for tabs

	fl_addto_tabfolder(obj,"CONTROL",fd_form_control->form_control);
	fl_addto_tabfolder(obj,"DEVICES",fd_form_devices->form_devices);
	fl_addto_tabfolder(obj,"SCRIPTS",fd_form_scripts->form_scripts);
	if (feature_terminal==TRUE)
        	fl_addto_tabfolder(obj,"TERMINAl",fd_form_terminal->form_terminal);				// Always last on tabs

	fl_end_form( );
	fdui->mainform->fdui = fdui;
	return fdui;
}


// Returns 0 for success
int jlc_gui_init_xforms()
{
	int i=0;
	int argc=1;
	char argvn[32];
	char* argvx = (char*)&argvn;
	char** argv=&argvx;

	sprintf(argvn,"hello");
	fl_set_icm_color(111, 0x00, 0x46, 0x84);
	fl_initialize( &argc, argv, 0, 0, 0 );
	fl_get_wingeometry(fl_root, &i, &i, &xserver_width, &xserver_height);
	printf("X server has width=%d height=%d\n",xserver_width,xserver_height);
	if (feature_fullscreen==TRUE)
	{
		app_width=xserver_width;
		app_height=xserver_height;
	}

	int as=0;
	int ds=0;
	font_height=fl_get_string_height(FL_FIXED_STYLE, FL_SMALL_SIZE, "H", 1, &as, &ds);
	termlines  =((app_height - 26) / font_height) - 2;
	scriptlines=((app_height - 26) / font_height);

	printf("app_width=%d app_height=%d font_height=%d\tscriptlines=%d\ttermlines=%d\n",app_width,app_height,font_height,scriptlines,termlines);
	fflush(stdout);

	fd_form_control = form_control_create(  app_width, app_height );
	fd_form_terminal = form_terminal_create( app_width, app_height );
	fd_form_scripts = form_scripts_create( app_width, app_height );
	fd_form_devices = form_devices_create( app_width, app_height );
	fd_form_colorsel = create_form_colorsel( app_width, app_height );

	fd_mainform=create_form_mainform();									// must be created last
	fl_set_timer(fd_form_terminal->form_terminal_timer,0.01);
	fl_set_timer(fd_form_scripts->scriptsform_timer,0.01);
	form_control_defaults( fd_form_control );
	form_devices_defaults( fd_form_devices );
        
	fl_show_form(fd_mainform->mainform, FL_PLACE_CENTERFREE, FL_FULLBORDER, windowtitle );                  // fixed size form
	//fl_show_form(fd_mainform->mainform, FL_PLACE_CENTER | FL_FREE_SIZE, FL_FULLBORDER, windowtitle );                  // fixed size form
	fl_set_folder(fd_mainform->tabs,fd_form_control->form_control);                                         // default form

	// Setup idle callback to run as often as possible
	fl_set_idle_callback( (FL_APPEVENT_CB)idle_callback, NULL );
	fl_set_idle_delta(1);
	fl_add_timeout(1, idle_callback, NULL);

	fl_linestyle(FL_SOLID);
	fl_drawmode(GXcopy);
	return(0);
}



void jlc_gui_defaults_xforms()
{
	scripts_init();
	terminal_init();
}




// Control returns when application closes
int jlc_gui_run_xforms()
{
	printf("jlc_gui_run_xforms()\n"); fflush(stdout);
        fl_do_forms( );
        fl_finish( );
        return 0;
}




// Something changed on the server, update the GUI to reflect this, the local structured devn[] devf[] grpc[] and grpv[]
// have already been updated with the new server state
void jlc_change(int msg_type, int idx)
{
	switch (msg_type)
	{	
		// Anything that changes the state of the visuals
		case BIN_MSG_GROUP_CLEAR:
		case BIN_MSG_DEVF_CLEAR:
		case BIN_MSG_DEVN_CLEAR:
		//case BIN_MSG_GROUP_CFG:
		case BIN_MSG_GROUP_VAL:
		case BIN_MSG_DEVF:
		case BIN_MSG_DEVN:
			form_device_jlc_change(msg_type, idx);
			form_control_jlc_change(msg_type, idx);
		break;

		case BIN_MSG_GROUP_CFG:
			form_control_jlc_change(msg_type, idx);
		break;

		case BIN_MSG_CLRP:
			form_colorselector_jlc_change(msg_type, idx);
		break;

		case BIN_MSG_DATETIME:
			if (feature_caption==TRUE)
				fl_set_object_label(fd_mainform->preset_label,server_datetime);
		break;
	}
	if (msg_type==BIN_MSG_GROUP_VAL)
		form_colorselector_jlc_change(msg_type, idx);					// if colour selector form is open then update it
}




void once_per_second()
{
	//printf("once_per_second()\n"); fflush(stdout);
	if (online!=TRUE)
	{
		jlc_gui_defaults_xforms();
		init_slider_lockoutcounters();
		init_group();									// clear our copy of the server state
		init_devf();
		init_devn();
		form_control_defaults( fd_form_control );
		if (feature_caption==TRUE)
			fl_set_object_label(fd_mainform->preset_label,"");
		connect_to_server();
	}
	else
	{
		xprintf(fd_monitor, "\r");							// every second send something, helps detect dropped socket
	}
	reap_all();
}



void idle_callback()
{
	int sec=0;
	static int psec=-1;
	int i=0;
	//int n=0;
	//char buf[8192];

//printf("x=%d\ty=%d\n",fd_mainform->mainform->x, fd_mainform->mainform->y); fflush(stdout);

	for (i=0;i<MAX_GROUP_VISUALS;i++)
	{
		if (slider_lockoutcounter[i]>0)
			slider_lockoutcounter[i]--;
	}

	if (feature_terminal==TRUE)
		terminal_poll();
	scripts_poll();

	sec=current_times();
	if (psec!=sec)
	{
		psec=sec;
		once_per_second();
	}
	if (online==TRUE)
		persistant_binmsg_poll();

	if (colorsel_active>0)
		colorselector_handler(NULL, colorsel_active);
}







// callback for when a tab selected.
// The number of tabs and the order of the tabs change depending on configuration,
void tabs_cb( FL_OBJECT * obj, long data )
{
	char fn[32];										// form name

	// The number of tabs and the order of the tabs change depending on configuration so we need to tie 
	// the selected tab to a consistent value.
	sprintf(fn,"%s",fl_get_active_folder_name(obj));
	selected_form=-1;
	if (strncmp(fn,"CONTROL",3)==0)
		selected_form=1;
	if (strncmp(fn,"DEVICES",3)==0)
		selected_form=2;
	if (strncmp(fn,"SCRIPTS",3)==0)
		selected_form=3;
	if (strncmp(fn,"TERMINAl",3)==0)
		selected_form=4;
	if (selected_form<=0)
	{
		printf("Fixme\n");
		exit(1);
	}

	// Fix rendering bug where text shows through from other forms
	fl_rect(0,0,app_width,app_height,FL_BLACK);

	printf("tabs_cb()\tfn=%s\tselected_form=%d\n",fn,selected_form);
	fflush(stdout);
	if (selected_form==4)
		fl_set_focus_object(fd_form_terminal->form_terminal, fd_form_terminal->terminput);
	if (selected_form==2)
		form_devices_activate();
}


