// jlc_gui_xforms_form_colorselector.c

#include <stdlib.h>
#include "jlc_gui_xforms_form_colorselector.h"
#include "jlc.h"
#include "jlc_gui_xforms_form_control.h"
#include "jlc_prototypes.h"
#include "mainform.h"
#include "forms.h"
#include "jcp_protocol_devs.h"
#include "jlc_group.h"

#include "block.xpm"								// block_xpm[]


unsigned int R,G,B,W;
unsigned int pR,pG,pB,pW;

extern int              online;                                                 // connected to server ?
extern FD_mainform      *fd_mainform;
extern FD_form_control  *fd_form_control;
extern FD_form_colorsel *fd_form_colorsel;
extern int              colorsel_active;                                        // negative=off, positive is group colour is being selected for
extern int              fd_terminal;
extern int              fd_monitor;
extern int              fd_binmsg;
extern struct jlc_colourpreset  clrp[MAX_COLOUR_PRESETS+1];

int			sendrate_hz=15;
extern int		locktime_ms;						// lock applies for N milliseconds
uint64_t		slider_lockoutcounter;					// Do not update slider while counter is active
int			sendnow=FALSE;


// Groups
extern struct jlc_group_cfg     grpc[MAX_GROUPS+1];
extern struct jlc_group_val     grpv[MAX_GROUPS+1];


void colourpreset_cb( FL_OBJECT * obj, long data );
void colourpreset_sliderw_cb( FL_OBJECT * obj, long data );


// Pixmap button is obj, modify the image.
void modify_pixmap_block(FL_OBJECT *obj,  int r, int g, int b, int w)
{
        char newpixmap[100][4096];
        char *pmps[16384];
        int x=0;

        //printf("modify_pixmap_block()\n"); fflush(stdout);
        for (x=0;x<52;x++)
        {
                strcpy(newpixmap[x], block_xpm[x]);
                pmps[x]=(char*)&newpixmap[x];                                   // Pixmaps are an array of pointers each to a line of text
                //printf("%s\n",newpixmap[x]);
        }
        sprintf(newpixmap[1]+5,"%02X%02X%02X",(unsigned int)r, (unsigned int)g, (unsigned int)b);
        //printf("%s\n",newpixmap[7]);
        fl_free_pixmapbutton_pixmap(obj);                                       // release the RAM the object owns for pixmap
        fl_set_pixmapbutton_data(obj, pmps);                                    // replace pixmap with new one
}




// Across Down Width height

FD_form_colorsel *create_form_colorsel( int aw, int ah )
{
    FL_OBJECT *obj;
    FD_form_colorsel *fdui = ( FD_form_colorsel * ) fl_malloc( sizeof *fdui );
    int i=0;
    int ch=0;                                                                   // control height
    int cw=0;                                                                   // control width
    int osd=0;                                                                  // offset down
    int osr=0;                                                                  // offset right


    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->colorsel = fl_bgn_form( FL_NO_BOX, aw, ah );
    obj = fl_add_box( FL_FLAT_BOX, 0, 0, aw, ah, "" );
    fl_set_object_color(obj, FL_BLACK, FL_BLACK);

    fdui->colorselectorgraphic = obj = fl_add_pixmap( FL_NORMAL_PIXMAP, 30, 10, 450, 450, "" );
    fl_set_pixmap_file( obj, "conic-spectrum1.xpm" );

    //fdui->txt_r = obj = fl_add_text( FL_NORMAL_TEXT, 10, 10, 60, 35, "R" );
    //fl_set_object_color(obj, FL_BLACK, FL_BLACK);
    //fl_set_object_lcolor(obj, FL_WHITE);

    //fdui->txt_g = obj = fl_add_text( FL_NORMAL_TEXT, 10, 40, 60, 35, "G" );
    //fl_set_object_color(obj, FL_BLACK, FL_BLACK);
    //fl_set_object_lcolor(obj, FL_WHITE);

    //fdui->txt_b = obj = fl_add_text( FL_NORMAL_TEXT, 10, 70, 60, 35, "B" );
    //fl_set_object_color(obj, FL_BLACK, FL_BLACK);
    //fl_set_object_lcolor(obj, FL_WHITE);


    // Colour presets upto 16 but display may only fits 14
    osd=15;
    osr=aw-285;
    ch=50;
    cw=132;
    for (i=1;i<=14;i++)
    {
	fdui->colourpreset[i] = obj = fl_add_pixmapbutton( FL_NORMAL_BUTTON, osr, osd, cw, ch, "" );
	fl_set_object_boxtype( obj, FL_NO_BOX );
	fl_set_object_callback( obj, colourpreset_cb, i );					// not needed
	obj->u_ldata=i;
	//fl_set_pixmapbutton_file(obj, "colourpreset.xpm" );
	osd=osd+ch+6;
	if (osd>400)
	{
		osd=15;
		osr=osr+cw+6;
	}
    }


    // Colour button bottom of form
    fdui->btn_ok = obj = fl_add_button( FL_NORMAL_BUTTON, aw-285, ah-65, 270, 50, "" );
    fl_set_object_boxtype( obj, FL_BORDER_BOX );
    fl_set_object_color(obj, FL_FREE_COL1, FL_FREE_COL1 );
    fl_set_object_lcolor(obj, FL_WHITE );


    // Optional brightness slider to right of graphic, hidden as default
    fdui->slider_w = obj = fl_add_slider( FL_VERT_BROWSER_SLIDER, 460, 20, 30, 375, "" );
    fl_set_object_callback( obj, colourpreset_sliderw_cb, 0);
    fl_set_slider_bounds( obj, 255, 0);
    fl_hide_object( obj );

    fl_end_form( );
    fdui->colorsel->fdui = fdui;
    return fdui;
}


// When form is selected by user this is called. Setup the colour selector form for the RGB or RGBW type group
void colorselector_form_defaults(int g)
{
	printf("colorselector_form_defaults() g=%d\n",g); fflush(stdout);
	switch (grpc[g].gltype)
	{
		case GROUP_LIGHT_TYPE_RGB:
			fl_hide_object (fd_form_colorsel->slider_w);
			fl_set_object_position(fd_form_colorsel->colorselectorgraphic ,30, 10);
		break;
		case GROUP_LIGHT_TYPE_RGBW:
			fl_show_object (fd_form_colorsel->slider_w);
			fl_set_object_position(fd_form_colorsel->colorselectorgraphic ,1, 10);		// Move it left a bit
			W=grpv[g].W;
			fl_set_slider_value(fd_form_colorsel->slider_w, W);
		break;
	}
	slider_lockoutcounter=current_timems();
}




// Called from OK button callback on colour selector form or mouse click on a colour
// We pretend that colour is always RGBW, the W will be ignored if the value is really RGB
void color_selected(FL_OBJECT * obj, long data)
{
	//printf("User colour selected ,  R=%d G=%d B=%d W=%d\tcolorsel_active=%d\n",R,G,B,W,colorsel_active); fflush(stdout);
	fl_hide_form(fd_form_colorsel->colorsel);
	modify_pixmap_rgbbar(fd_form_control->rgbw_group[colorsel_active], R, G, B, W);
	xprintf(fd_monitor,"g %d r%d g%d b%d w%d\r",colorsel_active, R, G, B, W);			// send final values
	colorsel_active=-1;                                                                             // cancel the form
}



// Handle colourselector form.
void colorselector_handler(FL_OBJECT * obj, long data)
{
	unsigned int k;
	int x=0;
	int y=0;
	int relx=0;
	unsigned int r,g,b;
	static uint64_t	lastsendtime;

        // If system has gone offline with colour selector active then make sure the form is closed so user can see greyed out controls
	if (online!=TRUE)
	{
		fl_hide_form(fd_form_colorsel->colorsel);
		colorsel_active=-1;
	}

	fl_get_mouse( &x, &y, &k);
	relx = x - fd_form_colorsel->colorsel->x;
	//printf("x=%d\ty=%d\trelx=%d\n",x,y,relx); fflush(stdout);
	getcolor_under_cursor(x,y,&r,&g,&b);
	if ( k & Button3Mask)									// Button pushed or mouse key click
	{
		color_selected(NULL,0);
		return;
	}

	if (relx<450 || relx>500)								// if not inside slider control
	{
		if ((r==0) && (g==0) && (b==0))							// if colour under mouse is black
		{
		}
		else
		{
			R=r;  G=g;  B=b;
			fl_mapcolor( FL_FREE_COL1, R, G, B);
			fl_redraw_object(fd_form_colorsel->btn_ok);
			sendnow=TRUE;
		}
	}

	// Rate limited sending of values.
	if (sendnow==TRUE)
	{
		if (current_timems() - lastsendtime > (1000/sendrate_hz))
		{
			if ( R!=pR || G!=pG || B!=pB || W!=pW )					// values have changed since last send ?
			{
				xprintf(fd_monitor,"g %d r%d g%d b%d w%d\r",colorsel_active, R, G, B, W);
				//printf("g %d r%d g%d b%d w%d\n",colorsel_active, R, G, B, W); fflush(stdout);
				lastsendtime=current_timems();
				pR=R;
				pG=G;
				pB=B;
				pW=W;
			}
			sendnow=FALSE;
		}
	}
}



// White level slider for RGBW layout of form
void colourpreset_sliderw_cb( FL_OBJECT * obj, long data )
{
	int v=(int)fl_get_slider_value(obj);
	int idx=(int)data;

	//printf("colourpreset_sliderw_cb() data=%d  v=%d\n",(int)data, v); fflush(stdout);
	W=v;
	grpv[idx].W=W;
	sendnow=TRUE;
	slider_lockoutcounter=current_timems();							// record time the control was moved
}



// User clicked the colour selector, start showing the colorselector form on top of the control form
void rgbw_cb (FL_OBJECT * obj, long data )
{
        int i=(int)data;									// group number
	printf("rgbw_cb()  data=%d\n",(int)data); fflush(stdout);

        fl_set_object_callback( fd_form_colorsel->btn_ok, color_selected, data);
        fl_set_form_position(fd_form_colorsel->colorsel, fd_mainform->mainform->x, fd_mainform->mainform->y);   // Sit on top oif main form
        fl_show_form(fd_form_colorsel->colorsel, FL_PLACE_GEOMETRY, FL_NOBORDER, "" );
	colorselector_form_defaults(i);
	colorsel_active=i;                                                                      // This group is being modified
}



// Not really needed
void colourpreset_cb( FL_OBJECT * obj, long data )
{
	//printf("colourpreset_cb()\n"); fflush(stdout);
}



// Server sent us a new value
void form_colorselector_jlc_change(int msg_type, int idx)
{
	uint64_t	tdiff=0;

	//printf("form_colorselector_jlc_change() msg_type=%d\tidx=%d\n",msg_type,idx); fflush(stdout);
	switch (msg_type)
	{
		case BIN_MSG_CLRP:								// colour preset value
			if (idx<=14)
			{
				modify_pixmap_block(fd_form_colorsel->colourpreset[idx], clrp[idx].R, clrp[idx].G, clrp[idx].B, -1);
			}
		break;


		case BIN_MSG_GROUP_VAL:
			if (colorsel_active != idx)						// Not the group we have open ?
				return;	

			tdiff=current_timems() - slider_lockoutcounter;
			if (tdiff < locktime_ms)
				return;
			fl_set_slider_value(fd_form_colorsel->slider_w, grpv[idx].W);
			fl_mapcolor( FL_FREE_COL1, grpv[idx].R, grpv[idx].G, grpv[idx].B);
			fl_redraw_object(fd_form_colorsel->btn_ok);
			R=grpv[idx].R;
			G=grpv[idx].G;
			B=grpv[idx].B;
			W=grpv[idx].W;
		break;
	}
	
}




