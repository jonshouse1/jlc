// jlc_gui_xforms_form_devices.c

#include <stdlib.h>
#include "jcp_protocol_devs.h"
#include "jlc_gui_xforms_form_devices.h"
#include "jlc_linuxgui.h"
#include "jlc.h"
#include "jlc_prototypes.h"
#include "jlc_group.h"

void dlist_cb( FL_OBJECT * obj, long data );
void devtype_cb( FL_OBJECT * obj, long data );
void form_devices_update_display();

extern FL_COLOR		clr_highlight;
extern FL_COLOR		clr_active;				// Exists but "off"
extern FL_COLOR		clr_inactive;				// Not assigned to anything yet
extern FL_COLOR		clr_selected;


// Across Down Width height

//FL_COLOR	butpushedcolour = FL_DARKGREEN;
static char	dtnames[8][32]={"FIXTURE","RELAY","SWITCH","DOORBELL","MOVESENS","TEMPSENS","PLAYSOUND","DISPLAYS"};
extern int			withborders;
extern int 			eature_show_switches;
extern FD_form_devices		*fd_form_devices;
extern int			app_width,  app_height;

// Two tables of device and device states
extern struct jlc_devf          devf[MAX_DEVF];
extern struct jlc_devn          devn[MAX_DEVN];

// Groups
extern struct jlc_group_cfg     grpc[MAX_GROUPS+1];
extern struct jlc_group_val     grpv[MAX_GROUPS+1];

// Table for timed events
extern struct timed_events      te[MAX_TIMED_EVENTS];


int		infolines_len   = 4;
int		dtype_len	= 8;
int		dlist_len	= 24;
int		selecteddev	= 0;			// 0=first line
int		selectedtype	= 0;			// 0=fixture, 1=relay etc
int 		pselectedtype	=-1;





// Populate the details section of the display
void form_devices_showdetails(int idx)
{
	char st[1024];
	char stb[512];
	char stc[512];
	int i=0;
	//printf("form_devices_showdetails()  idx=%d\n",idx); fflush(stdout);

	if (idx<0)									// out of range ?
	{										// Display blank form
		for (i=0;i<infolines_len;i++)
			fl_set_object_label ( fd_form_devices->infolines[i],"");
		return;
	}


	if (selectedtype == 0)								// 0=fixture
	{
		if (devf[idx].dev_type == 0)
			return;
		// 1st line
		sprintf(st,"TYPE: %-16s    IPADDR: %-16s JCP UDP Port:%d  JLP UDP Port:%d",dev_type_name(devf[idx].dev_type),
			 devf[idx].ipaddr, devf[idx].jcp_udp_port, devf[idx].jlp_udp_port);
		fl_set_object_label( fd_form_devices->infolines[0], st);

		// 2nd line
		sprintf(st,"UID: %-16s     NAME: %-32s ",printuid(devf[idx].uid), devf[idx].dev_name);
		fl_set_object_label( fd_form_devices->infolines[1], st);

		// 3rd line
		sprintf(st,"MODEL: %-16s   UNIV: %-4d    FCHAN: %-5d", devf[idx].device_model, devf[idx].univ, devf[idx].fchan);
		fl_set_object_label( fd_form_devices->infolines[2], st);

		// 4th line
		sprintf(st,"NOC: %-5d   ", devf[idx].noc);
		fl_set_object_label( fd_form_devices->infolines[3], st);
		return;
	}

	// 1st line
	sprintf(st,"TYPE: %-16s    IPADDR: %-16s JCP UDP Port:%d  JLP UDP Port:%d ",dev_type_name(devn[idx].dev_type), devn[idx].ipaddr,
		devn[idx].jcp_udp_port, devn[idx].jlp_udp_port);
	fl_set_object_label( fd_form_devices->infolines[0], st);

	// 2nd line
	sprintf(st,"UID: %-16s     NAME: %-32s ",printuid(devn[idx].uid), devn[idx].dev_name);
	fl_set_object_label( fd_form_devices->infolines[1], st);

	// 3rd line
	bzero(&stb,sizeof(stb));
	for (i=0;i<MAX_TOPICS;i++)
		stb[i]=devn[idx].topics[i];
	sprintf(st,"GROUP: %-4d               MODEL: %-16s  TOPICS[%s] ", devn[idx].group, devn[idx].device_model, stb);
	for (i=0;i<MAX_TOPICS;i++)
	fl_set_object_label( fd_form_devices->infolines[2], st);

	// 4th line
	sprintf(st,"ASCORBIN: %-1d   VAL1: %-5d VAL2: %-5d VBLEN: %-3d   VB:",
		devn[idx].ds.asciiorbinary, devn[idx].ds.value1, devn[idx].ds.value2, devn[idx].ds.valuebyteslen);
	if (devn[idx].ds.asciiorbinary==1)						// ASCII
		sprintf(stb,"%-32s",devn[idx].ds.valuebytes);
	else
	{
		stb[0]=0;
		for (i=0;i<32;i++)
		{
			if (i<devn[idx].ds.valuebyteslen)
			{
				sprintf(stc,"%02X",(unsigned char)devn[idx].ds.valuebytes[i]);
				strcat(stb,stc);
			}
		}
	}	
	strcat(st,stb);
	fl_set_object_label( fd_form_devices->infolines[3], st);
}



void highlight_one_dlist(int x)
{
	int i=0;

	// 3 colours here, inactive, active and selected.
	for (i=0;i<dlist_len;i++)
	{
		if (i==x)
			fl_set_object_color( fd_form_devices->btn_dlist[i], clr_selected, clr_selected );
		//else	fl_set_object_color( fd_form_devices->btn_dlist[i], clr_inactive, butpushedcolour );
		else
		{
			if (fd_form_devices->btn_dlist[i]->u_ldata <0)
				fl_set_object_color( fd_form_devices->btn_dlist[i], clr_inactive, clr_selected );
			else	fl_set_object_color( fd_form_devices->btn_dlist[i], clr_active, clr_selected );
		}
	}
	form_devices_showdetails(fd_form_devices->btn_dlist[x]->u_ldata);
}



// Buttons across the bottom, highlight one
void highlight_one_devtype(int x)
{
	int i=0;

	//printf("highlight_one_devtype()  %d\n",x); fflush(stdout);
	for (i=0;i<dtype_len;i++)
	{
		if (i==x)
			fl_set_object_color( fd_form_devices->btn_devtype[i], clr_selected, clr_selected );
		else	fl_set_object_color( fd_form_devices->btn_devtype[i], clr_active, clr_selected );
	}
}



void devtype_clear_list()
{
	int i=0;

	for (i=0;i<dlist_len;i++)
	{
		fl_set_object_label( fd_form_devices-> btn_dlist[i], "");
		fd_form_devices-> btn_dlist[i]-> u_ldata=-1;
	}
}



FD_form_devices *form_devices_create( int aw, int ah )
{
	FL_OBJECT *obj;
	FD_form_devices *fdui = ( FD_form_devices * ) fl_malloc( sizeof *fdui );
	int as=0, ds=0;
	int font_height=0;
	int i=0;
	int x=0;
	int ch=0;									// control height
	int cw=0;									// control width
	int osd=0;									// offset down
	int osr=0;									// offset right

	font_height=fl_get_string_height(FL_FIXED_STYLE, FL_SMALL_SIZE, "H", 1, &as, &ds);
	printf("font_height=%d\n",font_height);
	font_height=16;			// for now just override it so form always looks the same on different machines
	fdui->vdata = fdui->cdata = NULL;
	fdui->ldata = 0;
	fdui->form_devices = fl_bgn_form( FL_NO_BOX, aw, ah );
	obj = fl_add_box( FL_FLAT_BOX, 0, 0, aw, ah, "" );
	fl_set_object_color( obj, FL_BLACK, FL_WHITE );

	osr=0;
	ch=30;
	osd=ah-20-ch;

	// button for other buttons to sit on, gives a coloured block
	fdui->btn_base = obj = fl_add_button (FL_TOUCH_BUTTON, 0, osd, aw, ch, "");
	fl_set_object_color( obj, FL_DARKBLUE, FL_WHITE);
	fl_set_object_boxtype( obj, FL_FLAT_BOX );
	fl_set_object_bw( obj, 0 );							// box line width
	fl_set_button_mouse_buttons( obj, 0 );						// nothing changes when pushed

	// Device type selector along screen bottom
	for (i=0;i<dtype_len;i++)			// Would be NUM_DEVN_TYPES but we only want to display some of them
	{
		//printf("%s\n",dtnames[i]);
		fdui->btn_devtype[i] = obj = fl_add_button( FL_NORMAL_BUTTON, osr, osd, 90, ch, (char*)&dtnames[i] );
		fl_set_object_boxtype( obj, FL_BORDER_BOX );
		fl_set_object_bw( obj, 1 );						// box line width
		fl_set_object_callback( obj, devtype_cb, i );
		osr=osr+102;
	}


	// List of devices of selected type
	ch=font_height+12;
	osr=4;
	osd=10;
	cw=250;
	x=0;
	for (i=0;i<dlist_len;i++)
	{
		fdui->btn_dlist[i] = obj = fl_add_button( FL_NORMAL_BUTTON, osr, osd, cw, ch, "-------------" );
		fl_set_object_boxtype( obj, FL_BORDER_BOX );
		fl_set_object_bw( obj, 1 );
    		fl_set_object_color( obj, clr_inactive, clr_selected );
    		fl_set_object_lcolor( obj, FL_WHITE );
		fl_set_object_callback( obj, dlist_cb, i );
		fl_set_object_lalign( obj, FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
		osd=osd+ch+6;
		x++;
		if (x>=8)
		{
			x=0;
			osr=osr+cw+20;
			osd=10;
		}
	}


	// coloured box details sit on
	osr=10;
	osd=295;
	ch=100;
	fdui->btn_details = obj = fl_add_button (FL_TOUCH_BUTTON, 0, osd, aw, ch, "");
	fl_set_object_color( obj, FL_STEELBLUE, FL_WHITE);
	fl_set_object_boxtype( obj, FL_FLAT_BOX );
	fl_set_object_bw( obj, 0 );
	fl_set_button_mouse_buttons( obj, 0 );						// nothing changes when pushed

	// Lines of text for device info
	osd=305;
	osr=10;
	for (i=0;i<infolines_len;i++)
	{
		fdui->infolines[i] = obj = fl_add_text( FL_NORMAL_TEXT, osr, osd, aw-20, 20, "line");
		fl_set_object_lstyle( obj, FL_FIXED_STYLE );
		fl_set_object_color( obj, FL_STEELBLUE, FL_WHITE);
    		fl_set_object_lcolor( obj, FL_WHITE );
		osd=osd+22;
	}


	// Switches, displayed only when SWITCHES tab is selected
	osd=400;
	ch=20;										// control height
	cw=34;
	osr=10;
	for (i=1;i<=MAX_SWITCH_VISUALS;i++)
	{
		fdui->switchstate[i] = obj = fl_add_button( FL_NORMAL_BUTTON, osr, osd, cw, ch, " " );
		fl_hide_object( obj );
		fl_set_object_boxtype( obj, FL_NO_BOX );
		osr=osr+cw+10;
	}

	fl_end_form( );
	fdui->form_devices->fdui = fdui;
	return fdui;
}


void form_devices_switch_default(FD_form_devices* fdui, int i)
{
        FL_OBJECT *obj;

        obj = fdui->switchstate[i];
        fl_set_object_boxtype( obj, FL_BORDER_BOX );
        fl_set_object_color( obj, clr_inactive, FL_DARKBLUE );
        fl_set_object_lcolor( obj, FL_WHITE );
        fl_set_object_label( obj, "");

	if (selectedtype==2)
		fl_show_object( obj );
	else	fl_hide_object( obj );
}



void form_devices_populate_line_devf(int idx, int li)
{
	FL_OBJECT *obj;
	char labeltext[1024];
					
	sprintf(labeltext,"%s (%s) ",printuid(devf[idx].uid), devf[idx].dev_name);
	//printf("labeltext=%s\n",labeltext); fflush(stdout);

	obj = fd_form_devices-> btn_dlist[li];
	fl_set_object_label( obj, labeltext);
	fl_set_object_color( obj, clr_active, 0);
	obj->u_ldata = idx;							// store devn[] index in visual object
	fl_show_object( obj );
}


// use devn[idx] to populate displayed line li
void form_devices_populate_line(int idx, int li)
{
	FL_OBJECT *obj;
	char labeltext[1024];
					
	sprintf(labeltext,"%s (%s) ",printuid(devn[idx].uid), devn[idx].dev_name);
	//printf("labeltext=%s\n",labeltext); fflush(stdout);

	obj = fd_form_devices-> btn_dlist[li];
	fl_set_object_label( obj, labeltext);
	fl_set_object_color( obj, clr_active, 0);
	obj->u_ldata = idx;							// store devn[] index in visual object
	fl_show_object( obj );
}


void form_device_update_switches()
{
	int idx=0;
	int i=0;

        for (i=1;i<=MAX_SWITCH_VISUALS;i++)
                form_devices_switch_default(fd_form_devices, i);				// set all displaying inactive

	i=1;											// switch index are 1 to MAX_SWITCH_VISUALS
	for (idx=0;idx<MAX_DEVN;idx++)                                                          // for all possible devices
	{
		switch (devn[idx].dev_type)
		{
			case DEVN_SWITCH:
                        case DEVN_SWITCHPBT:
                        case DEVN_SWITCHPBM:
				if (i<MAX_SWITCH_VISUALS)
				{
					//printf("switch=%d\n",i); fflush(stdout);
					if (devn[idx].ds.value1==1)
						fl_set_object_color( fd_form_devices->switchstate[i], clr_highlight, FL_DARKBLUE );
					else	fl_set_object_color( fd_form_devices->switchstate[i], clr_active, FL_DARKBLUE );
					i++;
				}
			break;
		}
	}
}






void form_devices_update_display()
{
	int idx=0;
	int li=0;										// list index, 0=first line

	//printf("form_devices_update_display()   selectedtype=%d  selecteddev=%d\n",selectedtype,selecteddev); fflush(stdout);
	highlight_one_devtype(selectedtype);
	highlight_one_dlist(selecteddev);
	devtype_clear_list();

	if (selectedtype==0)									// fixture
	{
        	for (idx=0;idx<MAX_DEVF;idx++)							// for all possible devices
		{
			if (devf[idx].dev_type != 0)						// Its a a device
			{
				form_devices_populate_line_devf(idx, li);
				li++;
			}
		}
	}
	else
	{
        	for (idx=0;idx<MAX_DEVN;idx++)							// for all possible devices
        	{
			switch (selectedtype)
			{
				case 1:								// RELAY
					if (devn[idx].dev_type == DEVN_RELAY)
					{
						form_devices_populate_line(idx, li);
						li++;
					}
				break;


				case 2:								// SWITCH
					if (devn[idx].dev_type == DEVN_SWITCH || devn[idx].dev_type == DEVN_SWITCHPBT || 
					    devn[idx].dev_type == DEVN_SWITCHPBM)
					{
						form_device_update_switches();
						form_devices_populate_line(idx, li);
						li++;
					}
				break;

				case 3:								// DOORBELL
					if (devn[idx].dev_type == DEVN_DOORBELL)
					{
						form_devices_populate_line(idx, li);
						li++;
					}
				break;

				case 4:								// MOVESENS
					if (devn[idx].dev_type == DEVN_MOVEMENTSENSOR)
					{
						form_devices_populate_line(idx, li);
						li++;
					}
				break;

				case 5:								// TEMPSENS
					if (devn[idx].dev_type == DEVN_TEMPSENS)
					{
						form_devices_populate_line(idx, li);
						li++;
					}
				break;

				case 6:								// PLAYSOUND
					if (devn[idx].dev_type == DEVN_PLAYSOUNDS || devn[idx].dev_type == DEVN_PLAYSOUNDM )
					{
						form_devices_populate_line(idx, li);
						li++;
					}
				break;

				case 7:								// DISPLAYS
					if (devn[idx].dev_type == DEVN_CLOCKCAL || devn[idx].dev_type == DEVN_LEDSIGN )
					{
						form_devices_populate_line(idx, li);
						li++;
					}
				break;
			}
		}
	}
	highlight_one_devtype(selectedtype);
	highlight_one_dlist(selecteddev);
	form_device_update_switches();
}




// Default values for all controls on the form
void form_devices_defaults(FD_form_devices* fdui)
{
	FL_OBJECT *obj;
	int i=0;

	//printf("form_devices_defaults() \n"); fflush(stdout);

	for (i=0;i<dtype_len;i++)
	{
		obj = fdui->btn_devtype[i];
    		fl_set_object_lalign( obj, FL_ALIGN_CENTER );
    		fl_set_object_lcolor( obj, FL_WHITE );
    		fl_set_object_color( obj, clr_inactive, clr_selected );
	}
	form_devices_update_display();

        for (i=1;i<=MAX_SWITCH_VISUALS;i++)
                form_devices_switch_default(fdui, i);
}




// Something has changed. The arrays devn[] grpc[] grpv[] have already been updated, reflect the new state
void form_device_jlc_change(int msg_type, int idx)
{
	//printf("form_device_jlc_change()\n"); fflush(stdout);

	switch (msg_type)
	{

		// Only update the switches if devn[] state has changed
		case BIN_MSG_DEVN_CLEAR:
		case BIN_MSG_DEVN:
			form_device_update_switches();
			form_devices_update_display();
		break;
	}
}


void form_devices_activate()
{
	//printf("\n\n\nform_devices_activate()\n"); fflush(stdout);
	devtype_clear_list();
	pselectedtype=selectedtype;
	highlight_one_devtype(selectedtype);
	form_devices_update_display();
}



void dlist_cb( FL_OBJECT * obj, long data )
{
	//int devnidx=obj->u_ldata;

	selecteddev=(int)data;
	highlight_one_dlist(selecteddev);
	//printf("devnidx=%d\n",devnidx); fflush(stdout);
}


void devtype_cb( FL_OBJECT * obj, long data )
{
	//printf("devtype_cb()  %d\n",(int)data); fflush(stdout);
	selectedtype=(int)data;
	devtype_clear_list();
	pselectedtype=selectedtype;
	highlight_one_devtype(selectedtype);
	selecteddev=0;
fl_rect(0,0,app_width,app_height-80,FL_BLACK);

	form_devices_update_display();
}

