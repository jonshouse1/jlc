// form_control_control.c

// https://sourceforge.net/p/lyx/svn/4431/tree//lyx-devel/trunk/src/frontends/xforms/FormMathsDelim.C
// fl_get_pixmap_pixmap


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#include "jlc_gui_xforms_form_control.h"
#include "jlc_gui_xforms_form_colorselector.h"
#include "jlc_linuxgui.h"
#include "jlc.h"
#include "jlc_prototypes.h"
#include "jcp_protocol_devs.h"
#include "jlc_group.h"

#include "mainform.h"

#include "rgbbar.xpm"


// Snapshot from X.h
#define Button1Mask     (1<<8)
#define Button2Mask     (1<<9)
#define Button3Mask     (1<<10)



extern FL_COLOR		clr_highlight;
extern FL_COLOR		clr_active;
extern FL_COLOR		clr_inactive;

extern int withborders;
extern int feature_show_switches;

// Prototypes
void btn_off_group_cb (FL_OBJECT * obj, long data );
void btn_on_group_cb (FL_OBJECT * obj, long data );
void rgbw_cb (FL_OBJECT * obj, long data );


// All xforms GUI state lives here
extern FD_mainform	*fd_mainform;
extern FD_form_control  *fd_form_control;
extern FD_form_colorsel *fd_form_colorsel;
int			locktime_ms=50;						// lock applies for N milliseconds
uint64_t		slider_lockoutcounter[MAX_GROUP_VISUALS];               // Do not update slider while counter is active

extern char		server_datetime[512];
extern char		server_ipaddr[32];
extern int		online;							// connected to server ?
extern int		fd_terminal;
extern int		fd_monitor;
extern int		fd_binmsg;                         
extern int		selected_form;
extern int		app_height;
extern int		app_width;
extern int		colorsel_active;					// negative=off, positive is group colour is being selected for

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



// Reformat a temperature string
char* format_tempstring(char* st)
{
        static char sbuf[32];

        bzero(&sbuf,sizeof(sbuf));
        if (strlen(st)==0)
                return ((char*)&sbuf);
        sprintf(sbuf,"%s c",st);
        if (st[0]=='-' && st[1]=='0')
                sprintf(sbuf,"-%s c",st+2);
        if (st[0]=='+')
                sprintf(sbuf,"%s c",st+1);
        if (sbuf[0]=='0')
                sprintf(sbuf,"%s c",st+2);
        return ((char*)&sbuf);
}






// Pixmap button is obj, modify the image.
// if pixmap is RGB rather than RGBW then w=-1
#define PIXMAPLINES 61								// fuckin orrible format
#define PIXMAPLINELEN 4096
void modify_pixmap_rgbbar(FL_OBJECT *obj, int r, int g, int b, int w)
{
	//int i=obj->u_ldata;							// group number
	char newpixmap[PIXMAPLINES][PIXMAPLINELEN];
	char *pmps[PIXMAPLINES];
	int x=0;

	//printf("modify_pixmap_rgbbar()  r=%d\tg=%d\b=%d\tw=%d\n",r,g,b,w); fflush(stdout);
	for (x=0;x<PIXMAPLINES;x++)
	{
   		strcpy(newpixmap[x], rgbbar_xpm[x]);
   		pmps[x]=(char*)&newpixmap[x];					// Pixmaps are an array of pointers each to a line of text
		//printf("%s\n",newpixmap[x]);
	}
	sprintf(newpixmap[7]+5,"%02X%02X%02X",(unsigned int)r, (unsigned int)g, (unsigned int)b);
	//printf("%s\n",newpixmap[7]);
	if (w==-1)								// RGB? Make white block selected colour instead
		sprintf(newpixmap[8]+5,"%02X%02X%02X",(unsigned int)r, (unsigned int)g, (unsigned int)b);
	else	sprintf(newpixmap[8]+5,"%02X%02X%02X",(unsigned int)w, (unsigned int)w, (unsigned int)w);	// Include a white block
	//else	sprintf(newpixmap[8]+5,"FFFFFF");

	fl_free_pixmapbutton_pixmap(obj);					// release the RAM the object owns for pixmap
	fl_set_pixmapbutton_data(obj, pmps);					// replace pixmap with new one
}



// Across Down Width height
FD_form_control *form_control_create( int aw, int ah )
{
    FL_OBJECT *obj;
    FD_form_control *fdui = ( FD_form_control * ) fl_malloc( sizeof *fdui );
    int as=0, ds=0;
    int font_height=0;
    int i=0;
    int c=0;
    int w=0;
    int ch=0;									// control height
    int cw=0;									// control width
    int osd=0;									// offset down
    int osr=0;									// offset right

    font_height=fl_get_string_height(FL_FIXED_STYLE, FL_SMALL_SIZE, "H", 1, &as, &ds);
    printf("font_height=%d\n",font_height);
    font_height=16;			// for now just override it so form always looks the same on different machines
    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;
    fdui->form_control = fl_bgn_form( FL_NO_BOX, aw, ah );
    obj = fl_add_box( FL_FLAT_BOX, 0, 0, aw, ah, "" );
    fl_set_object_color( obj, FL_BLACK, FL_WHITE );

    // ********* GROUPS 
    if (withborders==TRUE)
    {
       fdui->frame_groups = obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 10, 8, aw-20, 265, "" );
       fl_set_object_color( obj, FL_BLACK, FL_BLACK );
       fl_set_object_lcolor( obj, FL_YELLOW );
    }



    ch=40;									// height of the button/slider not the text
    osd=0;
    osr=0;
    for (i=1;i<=MAX_GROUP_VISUALS;i++)
    {
	// ON OFF and Slider control for group intensity
    	fdui->groupname[i] = obj = fl_add_text( FL_NORMAL_TEXT, 90+osr, 15+osd, 200, 20, "groupname" );
	fl_set_object_lsize( obj, FL_SMALL_SIZE );
	fdui->btn_group_off[i] = obj = fl_add_button( FL_MENU_BUTTON, 20+osr, 35+osd, 40, ch, "OFF" );
	fl_set_object_callback( obj, btn_off_group_cb, i );
	obj->u_ldata=i;								// Make it simple to recover group number from object data
	fdui->slider_group[i] = obj = fl_add_slider( FL_HOR_BROWSER_SLIDER, 70+osr, 35+osd, 240, ch, "" );
	fl_set_object_callback( obj, slider_group_cb, i );
	obj->u_ldata=i;
	fdui->btn_group_on[i] = obj = fl_add_button( FL_MENU_BUTTON, 320+osr, 35+osd, 40, ch, "ON" );
	fl_set_object_callback( obj, btn_on_group_cb, i );
	obj->u_ldata=i;


	fdui->rgbw_group[i] = obj = fl_add_pixmapbutton( FL_NORMAL_BUTTON, 68+osr, 33+osd, 245, 44, "" );
	fl_set_object_boxtype( obj, FL_NO_BOX );
	fl_set_object_callback( obj, rgbw_cb, i );
	obj->u_ldata=i;
	//fl_set_pixmapbutton_file(obj, "rgbbar.xpm" );
	modify_pixmap_rgbbar(obj,  0x80, 0x80, 0x80, -1);


	osd = osd + ch+font_height+6;
// FIXME: 
	if ( i == (MAX_GROUP_VISUALS/2) )
	{
		osr=400;							// move controls right
		osd=0;
	}
    }


    // ********* TEMPERATURES
    if (feature_show_switches==TRUE)
    	 osd=282;
    else osd=286;
    osr=0;
    c=0;
    w=170;

    if (withborders==TRUE)
    {
       fdui->frame_tempsensor = obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 10, osd, aw-20, 40, "" );
       fl_set_object_color( obj, FL_BLACK, FL_BLACK );
       fl_set_object_lcolor( obj, FL_YELLOW );
    }
    for (i=1;i<=MAX_TEMP_VISUALS;i++)
    {
	fdui->tempsens_box[i] = obj = fl_add_box( FL_BORDER_BOX, 18+osr, osd+8+(c*32), 164, 24, "" );
	fdui->tempsens_name[i] = obj = fl_add_text( FL_NORMAL_TEXT, 20+osr, osd+10+(c*32), 140, 20, "name" );
	fdui->tempsens_value[i] = obj = fl_add_text( FL_NORMAL_TEXT, 120+osr, osd+10+(c*32), 60, 20, "sXX.XXC" );
	c++;
	if (c>=1)
	{
		c=0;
		osr=osr+w+20;							// move control right a column
	}
    }


    // ********* SENSORS
    if (feature_show_switches==TRUE)
         osd=332;								// how far down the display to start
    else osd=340;
    ch=34;									// control height
    osr=0;									// offset right
    w=136;									// width of control
    c=0;

    if (withborders==TRUE)
    {
       fdui->frame_movesensor = obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 10, osd, aw-20, 106, "" );
       fl_set_object_color( obj, FL_BLACK, FL_BLACK );
       fl_set_object_lcolor( obj, FL_YELLOW );
    }

    for (i=1;i<=MAX_SENSOR_VISUALS;i++)
    {
	fdui->btn_movement_sensor_box[i] = obj = fl_add_box( FL_BORDER_BOX, 18+osr, osd+8+(c*ch), w+4, 24, "" );
	fdui->btn_movement_sensor[i] = obj = fl_add_button( FL_NORMAL_BUTTON, 20+osr, osd+10+(c*ch), w, 20, "" );
	fl_set_object_boxtype( obj, FL_NO_BOX );
	fl_set_object_callback( obj, sensors_cb, i );
	c++;
	if (c>2)
	{
		c=0;
		osr=osr+w+20;							// move control right a column
	}
     }



    // ********** SWITCHES
    if (feature_show_switches==TRUE)
    {
	osd=445;
	ch=12;									// control height
	cw=34;
	osr=10;
	for (i=1;i<=MAX_SWITCH_VISUALS;i++)
	{
		fdui->switchstate[i] = obj = fl_add_button( FL_NORMAL_BUTTON, osr, osd, cw, ch, " " );
		fl_set_object_boxtype( obj, FL_NO_BOX );
		osr=osr+cw+10;
	}
    }


    fl_end_form( );
    fdui->form_control->fdui = fdui;
    return fdui;
}




void form_control_group_default(FD_form_control* fdui, int i)
{
	FL_OBJECT *obj;

	//printf("form_control_group_default i=%d\n",i); fflush(stdout);
	obj = fdui->groupname[i];
    	fl_set_object_lalign( obj, FL_ALIGN_CENTER );
    	fl_set_object_lcolor( obj, FL_WHITE );
    	fl_set_object_color( obj, FL_BLACK, FL_BLACK );
	fl_set_object_label( obj, "");

	obj = fdui->btn_group_off[i];
	fl_set_object_boxtype( obj, FL_BORDER_BOX );
	fl_set_object_lcolor( obj, clr_inactive );
	fl_set_object_color( obj, clr_inactive, FL_DARKBLUE );

	obj = fdui->slider_group[i];
	fl_set_slider_bounds( obj, 0, 255);
	fl_set_object_boxtype( obj, FL_BORDER_BOX );
	fl_set_object_lcolor( obj, FL_WHITE );
	fl_set_object_color( obj, clr_inactive, clr_inactive );
	fl_set_slider_value( obj , 0 );

	obj=fdui->btn_group_on[i];
	fl_set_object_boxtype( obj, FL_BORDER_BOX );
	fl_set_object_lcolor( obj, clr_inactive );
	fl_set_object_color( obj, clr_inactive, FL_DARKBLUE );

	fl_hide_object( fd_form_control-> rgbw_group[i]);
	fl_show_object( fd_form_control-> slider_group[i]);		// default is show mono slider
}


void form_control_temp_default(FD_form_control* fdui, int i)
{
	FL_OBJECT *obj;

	//printf("form_control_temp_default i=%d\n",i); fflush(stdout);
	obj = fdui->tempsens_box[i];
    	fl_set_object_color( obj, FL_DARKGREEN, 0 );
    	fl_set_object_lcolor( obj, FL_WHITE );

	obj = fdui->tempsens_name[i];
    	fl_set_object_color( obj, FL_BLACK, FL_BLACK );
	fl_set_object_lcolor( obj, FL_WHITE );	
	fl_set_object_label( obj, "");
	obj->u_vdata=NULL;
	obj->u_ldata=-1;

	obj = fdui->tempsens_value[i];
    	fl_set_object_color( obj, FL_BLACK, FL_BLACK );
	fl_set_object_lcolor( obj, FL_WHITE );
	fl_set_object_label( obj, "");
}



void form_control_sensor_default(FD_form_control* fdui, int i)
{
	FL_OBJECT *obj;

	//printf("form_control_sensor_default i=%d\n",i); fflush(stdout);
	obj = fdui->btn_movement_sensor_box[i];
    	fl_set_object_color( obj, FL_DARKGREEN, 0 );
    	fl_set_object_lcolor( obj, FL_WHITE );

	obj = fdui->btn_movement_sensor[i];
	fl_set_object_boxtype( obj, FL_BORDER_BOX );
	fl_set_object_color( obj, FL_BLACK, FL_WHITE );
	fl_set_object_lcolor( obj, FL_WHITE );
	fl_set_object_label( obj, "");
	obj->u_vdata=NULL;							// will point at a UID later
	obj->u_ldata=-1;
}


void form_control_switch_default(FD_form_control* fdui, int i)
{
	FL_OBJECT *obj;

	if (feature_show_switches!=TRUE)
		return;
	obj = fdui->switchstate[i];
	fl_set_object_boxtype( obj, FL_BORDER_BOX );
	fl_set_object_color( obj, clr_inactive, FL_DARKBLUE );
	fl_set_object_lcolor( obj, FL_WHITE );
	fl_set_object_label( obj, "");
}



// Set defaults for entire control form, this can be done more than once
void form_control_defaults( FD_form_control* fdui )
{
    int i=0;
    
    for (i=1;i<=MAX_GROUP_VISUALS;i++)
	form_control_group_default(fdui, i);

    for (i=1;i<=MAX_TEMP_VISUALS;i++)
	form_control_temp_default(fdui, i);

    for (i=1;i<=MAX_SENSOR_VISUALS;i++)
	form_control_sensor_default(fdui, i);

    if (feature_show_switches==TRUE)
    {
	for (i=1;i<=MAX_SWITCH_VISUALS;i++)
		form_control_switch_default(fdui, i);
    }
}



void init_slider_lockoutcounters()
{
	int i=0;

	for (i=0;i<MAX_GROUP_VISUALS;i++)
		slider_lockoutcounter[i]=current_timems();
}


// For a given update the off and on buttons
void xf_gui_update_group_buttons(int idx)
{
	fl_set_object_lcolor( fd_form_control->btn_group_off[idx], FL_WHITE );
	fl_set_object_lcolor( fd_form_control->btn_group_on[idx], FL_WHITE );
	if (grpv[idx].onoff==0)								// off ?
	{
		fl_set_object_color( fd_form_control->btn_group_off[idx] , clr_highlight, FL_GREY);
		fl_set_object_color( fd_form_control->btn_group_on[idx] , clr_active, FL_GREY);
	}
	else
	{
		fl_set_object_color( fd_form_control->btn_group_off[idx] , clr_active, FL_GREY);
		fl_set_object_color( fd_form_control->btn_group_on[idx] , clr_highlight, FL_GREY);
	}

        switch (grpc[idx].gltype)
	{
		case GROUP_LIGHT_TYPE_MONO:
		break;

		case GROUP_LIGHT_TYPE_RGB:
		case GROUP_LIGHT_TYPE_RGBW:
		break;
	}
}



// Update group value and/or control itself
void xf_gui_update_group_value(int idx)
{
	FL_OBJECT *obj;

	//printf("xf_gui_update_group_value()  grpc[idx].noc=%d\n",grpc[idx].noc); fflush(stdout);
	if (grpc[idx].noc<0)									// group is active ?
		return;

	xf_gui_update_group_buttons(idx);
        switch (grpc[idx].gltype)
	{
		case GROUP_LIGHT_TYPE_MONO:
			obj=fd_form_control->slider_group[idx];
			fl_show_object( obj );
			fl_set_slider_value( obj, grpv[idx].I);
			fl_set_object_color( obj ,clr_active, FL_WHITE);
		break;

		case GROUP_LIGHT_TYPE_RGB:
		case GROUP_LIGHT_TYPE_RGBW:
			fl_hide_object ( fd_form_control->slider_group[idx] );
			fl_show_object ( fd_form_control->rgbw_group[idx] );
			xf_gui_update_group_buttons(idx);
			if (grpc[idx].gltype==GROUP_LIGHT_TYPE_RGBW)
				modify_pixmap_rgbbar(fd_form_control->rgbw_group[idx], grpv[idx].R, grpv[idx].G, grpv[idx].B, grpv[idx].W);
			else	modify_pixmap_rgbbar(fd_form_control->rgbw_group[idx], grpv[idx].R, grpv[idx].G, grpv[idx].B, -1);
		break;
	}                    
}



// Reset the groups gui controls to defaults, then walk the grpc[] and grpv[] arrays populating the GUI controls.
void xf_gui_refresh_groups()
{
	FL_OBJECT *obj;
	int idx=0;

	printf("xf_gui_refresh_groups()\n"); fflush(stdout);
	for (idx=1;idx<MAX_GROUP_VISUALS;idx++)							// Clear all visuals for groups	
		form_control_group_default(fd_form_control, idx);

	for (idx=1;idx<MAX_GROUPS;idx++)							// for all possible groups
	{
		if (grpc[idx].noc>0)								// group is active ?
		{
			obj=fd_form_control->groupname[idx];
			fl_set_object_lcolor( obj, FL_WHITE);
			fl_set_object_label( obj, grpc[idx].name);
			fl_show_object( obj );
			//printf("grpc[idx].name=%s\n",grpc[idx].name); fflush(stdout);
			xf_gui_update_group_value(idx);
		}
	}
}




// idx=index of devn[]  vc=index of visual control
void xf_gui_update_sensor (int vc)
{
	FL_OBJECT *obj;
	struct jlc_devn*	dn=fd_form_control->btn_movement_sensor[vc]->u_vdata;		// pointer to devn[] stored in xforms object

	//printf("dn->ds->value1=%d\n", dn->ds.value1); fflush(stdout);
	fl_show_object( fd_form_control->btn_movement_sensor_box[vc] );
	obj=fd_form_control->btn_movement_sensor[vc];
	fl_set_object_label( obj, dn->dev_name);
	fl_set_object_lcolor( obj, FL_WHITE );
	fl_show_object( obj );

	if (dn->ds.value1==1)									// Sensor has triggered ?
	{
		fl_set_object_color(fd_form_control->btn_movement_sensor[vc], clr_highlight, FL_RED);
		if (feature_unblank_for_movement_sensor==TRUE)
			screenblanker_screenon();
	}
	else	fl_set_object_color(fd_form_control->btn_movement_sensor[vc], FL_BLACK, FL_RED);
}



void xf_gui_update_temp(int vc)
{
	FL_OBJECT *obj;
	struct jlc_devn*	dn=fd_form_control->tempsens_name[vc]->u_vdata;			// pointer to devn[] from xforms object

	obj=fd_form_control->tempsens_value[vc];
	fl_set_object_label( obj, format_tempstring(dn->ds.valuebytes));
	fl_set_object_lcolor( obj, FL_WHITE );
	fl_show_object( obj );

	obj=fd_form_control->tempsens_name[vc];
	fl_set_object_label( obj,  dn->dev_name);
	fl_set_object_lcolor( obj, FL_WHITE );
	fl_show_object( obj );
}



void xf_gui_update_switch(int vc)
{
	FL_OBJECT *obj;
	struct jlc_devn*	dn=fd_form_control->switchstate[vc]->u_vdata;			// pointer to devn[] from xforms object

	obj=fd_form_control->switchstate[vc];
	//printf("vc=%d\tdn->ds->value1=%d\tUID%s\n", vc, dn->ds.value1, printuid(dn->uid)); fflush(stdout);
	if (dn->ds.value1==1)									// switch is on?
	{
		fl_set_object_color( obj , clr_highlight, FL_WHITE);
	}
	else											// else it is off
	{
		fl_set_object_color( obj , clr_active, FL_WHITE);
	}
}





// Redraw everything that feeds from the devn table
// We could clear all visual controls before the loop and them re-populate, safe option but flickers on slower displays.
// Instead we clear visual controls when they no longer point at the same devn[] they where created with. This relies
// on visual_remove() being called. The server must be disciplined in always sending a BIN_MSG_DEVN_CLEAR packet
// to invalidate the devn[] entry of devices that are removed.
void xf_gui_devn_refresh(int doclear)
{
	int ntc=1;										// next visual temp control (starting at 1...)
	int nsc=1;										// next visual sensor control
	int nss=1;										// next visual switch control
	int idx=0;
	int i=0;
	FL_OBJECT *obj;

	//printf("xf_gui_devn_refresh()\n"); fflush(stdout);
	if (doclear==TRUE)									// remove all visual controls tied to devn[]
	{
    		for (i=1;i<=MAX_TEMP_VISUALS;i++)
			form_control_temp_default(fd_form_control, i);
    		for (i=1;i<=MAX_SENSOR_VISUALS;i++)
			form_control_sensor_default(fd_form_control, i);
	}


	for (idx=0;idx<MAX_DEVN;idx++)								// for all possible devices
	{
		switch (devn[idx].dev_type)							// look at device type
		{
			case DEVN_TEMPSENS:							// device is a temperature sensor
				if (ntc<=MAX_TEMP_VISUALS)					// Next visual still displayable ?
				{								// check if the visual still points at this devn
					if (fd_form_control->tempsens_name[ntc]->u_vdata != &devn[idx])
						form_control_temp_default(fd_form_control, ntc);// something changed, reset current visual
					obj=fd_form_control->tempsens_name[ntc];
					obj->u_vdata = (void*)&devn[idx];			// visual object has a pointer to current devn entry
					obj->u_ldata = idx;					// visual control index
					xf_gui_update_temp(ntc);				// display visual control
					ntc++;							// setup temp control index for next time round
				}
			break;

			case DEVN_DOORBELL:
			case DEVN_MOVEMENTSENSOR:
				if (nsc<=MAX_SENSOR_VISUALS)
				{
					if (fd_form_control->btn_movement_sensor[nsc]->u_vdata != &devn[idx])
						form_control_sensor_default(fd_form_control, nsc);
					obj=fd_form_control->btn_movement_sensor[nsc];
					obj->u_vdata = (void*)&devn[idx];
					obj->u_ldata = idx;
					xf_gui_update_sensor(nsc);
					nsc++;
				}
			break;

			case DEVN_SWITCH:
			case DEVN_SWITCHPBT:
			case DEVN_SWITCHPBM:
				if (nss<=MAX_SWITCH_VISUALS)
				{
					if (feature_show_switches==TRUE)
					{
						if (fd_form_control->switchstate[nss]->u_vdata != &devn[idx])
							form_control_switch_default(fd_form_control, nss);	
						obj=fd_form_control->switchstate[nss];
						obj->u_vdata = (void*)&devn[idx];
						obj->u_ldata = idx;
						xf_gui_update_switch(nss);
						nss++;
					}
				}
			break;       
		}
	}
}




// User pushed a sensor button on the control form
void sensors_cb( FL_OBJECT * obj, long data )
{
	char cmd[1024];
	int i=(int)data;									// Index of fd_form_control->btn_movement_sensor 
	int idx=(int)obj->u_ldata;								// index in dev_n[]

	printf("SENSORS CALLBACK i=%d   obj->u_ldata=%d\n",i, (int)obj->u_ldata); 
	if (idx<0)
		return;										// No visual control active
	printf("UID=%s",printuid((unsigned char*)devn[idx].uid));	

	sprintf(cmd,"dev %s state 1 1",printuid((unsigned char*)devn[idx].uid));
	printf("cmd [%s]\n",cmd);
	xprintf(fd_monitor,"%s\r",cmd);								// send it to servers interpreter
	fflush(stdout);

	pid_t pid= fork();
	if (pid==0)
	{
		sleep (3);
		sprintf(cmd,"dev %s state 0 0",printuid((unsigned char*)devn[idx].uid));
		printf("cmd [%s]\n",cmd);
		xprintf(fd_monitor,"%s\r",cmd);							// send command to servers interpreter
		exit(0);
	}
}


// User moved a group slider (mono type group only)
void slider_group_cb( FL_OBJECT * obj, long data )
{
	int g=(int)obj->u_ldata;								// group
	int v=(int)fl_get_slider_value(obj);
	//At the moment we treat group value as monochrome

	if (grpc[data].noc<=0)									// slider not assigned a group
	{
		fl_set_slider_value(obj,0);							// force it to be unresponsive
		return;
	}

	//printf("g=%d  v=%d\n\n", g, v); fflush(stdout);
	grpv[g].W=v;
	xf_gui_update_group_value(g);
	xprintf(fd_monitor,"g %d %d\r",g, v);
	slider_lockoutcounter[g]=current_timems();						// Record the time the control was modified
}


void btn_on_group_cb (FL_OBJECT * obj, long data )
{
	int idx=-1;

	//printf("btn_on_group_cb()  data=%d\n",(int)obj->u_ldata);  fflush(stdout);
	if (grpc[data].noc<=0)
		return;
	idx=(int)obj->u_ldata;
	xprintf(fd_monitor,"g %d on\r",idx);
}



void btn_off_group_cb (FL_OBJECT * obj, long data )
{
	int idx=-1;

	//printf("btn_off_group_cb()  data=%d\n",(int)obj->u_ldata);  fflush(stdout);
	if (grpc[data].noc<=0)
		return;
	idx=(int)obj->u_ldata;
	xprintf(fd_monitor,"g %d off\r",idx);
}




// The server sent us a change of state
void form_control_jlc_change(int msg_type, int idx)
{
	uint64_t tdiff=0;
	//printf("form_control_jlc_change()\n"); fflush(stdout);
        switch (msg_type)
        {
                // Rather than sending large data structures just send a message to reset them to defaults
		case BIN_MSG_GROUP_CLEAR:
			xf_gui_refresh_groups();
			//printf("jlc_xforms_change() BIN_MSG_GROUP_CLEAR idx=%d\n", idx); fflush(stdout);
		break;

		case BIN_MSG_DEVF_CLEAR:
		break;

		case BIN_MSG_DEVN_CLEAR:
			//printf("jlc_xforms_change() BIN_MSG_DEVN_CLEAR idx=%d\n",  idx); fflush(stdout);
			xf_gui_devn_refresh(TRUE);				// Clear sensors and temp visuals and re-populate them
		break;


		case BIN_MSG_GROUP_CFG:
			//printf("jlc_xforms_change() BIN_MSG_GROUP_CFG idx=%d\n",  idx); fflush(stdout);
			if (idx>=MAX_GROUP_VISUALS)
				return;
			xf_gui_refresh_groups();
		break;

		case BIN_MSG_GROUP_VAL:
			//printf("jlc_xforms_change() BIN_MSG_GROUP_VAL idx=%d\n",  idx); fflush(stdout);
			if (idx>=MAX_GROUP_VISUALS)
				return;

			tdiff = current_timems() - slider_lockoutcounter[idx];
			//printf("T=%d\n",(int)tdiff); fflush(stdout);
			if ( tdiff < locktime_ms)						// Too soon after control was moved ?
				return;								// then visually ignore the value server sent us
			xf_gui_update_group_value(idx);
                break;

		case BIN_MSG_DEVF:
			//dumphex(&devf[idx],sizeof(struct jlc_devf));
		break;


		case BIN_MSG_DEVN:
			xf_gui_devn_refresh(FALSE);
		break;
        }
}



