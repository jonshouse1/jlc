// jlc_gui_xforms_form_scripts.c

#include <stdlib.h>
#include "jlc_gui_xforms_form_scripts.h"
#include "jlc.h"
#include "jlc_prototypes.h"

extern int			app_width;
extern int			app_height;
extern int			scriptlines;
extern int			fd_monitor;
char				scriptstext[8192];


extern FD_form_scripts         *fd_form_scripts;


void form_scripts_timer_cb( FL_OBJECT * obj, long  data )
{
        idle_callback();
        fl_set_timer(fd_form_scripts->scriptsform_timer,0.001);
}



FD_form_scripts *form_scripts_create( int aw, int ah )
{
    FL_OBJECT *obj;
    FD_form_scripts *fdui = ( FD_form_scripts * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->form_scripts = fl_bgn_form( FL_NO_BOX, aw, ah );

    obj = fl_add_box( FL_FLAT_BOX, 0, 0, aw, ah, "" );
    fl_set_object_color( obj, FL_BLACK, FL_WHITE );
    fl_set_object_lcolor( obj, FL_WHITE );

    fdui->scriptsform_timer = obj = fl_add_timer( FL_HIDDEN_TIMER, 10, 10, 20, 20, "timer" );
    fl_set_object_lalign( obj, FL_ALIGN_CENTER );
    fl_set_object_callback( obj, form_scripts_timer_cb, 0 );

    fdui->scriptstext = obj = fl_add_text( FL_NORMAL_TEXT, 0, 0, aw, ah, "" );
    fl_set_object_color( obj, FL_BLACK, FL_WHITE );
    fl_set_object_lcolor( obj, FL_WHITE );
    fl_set_object_lsize( obj, FL_SMALL_SIZE );
    fl_set_object_lstyle( obj, FL_FIXED_STYLE );
    fl_set_object_lstyle( obj, FL_FIXED_STYLE );
    fl_set_object_lalign( obj, FL_ALIGN_LEFT_TOP | FL_ALIGN_INSIDE );

    fl_end_form( );

    fdui->form_scripts->fdui = fdui;
    return fdui;
}



void update_script_visuals(char *textline)
{
        char st[4096];

        sprintf(st,"%s\n",textline);
        strcat(scriptstext,st);
        trimbuf(scriptstext,sizeof(scriptstext),scriptlines);
        expand_tabs(scriptstext,sizeof(scriptstext));
        fl_set_object_label(fd_form_scripts->scriptstext,scriptstext);
}



void scripts_poll()
{
	int n=0;
	char buf[8192];

        do
        {
                // TODO: This looks like a hack, something it wrong here.
                n=poll_tcpsocket(fd_monitor, (char*)&buf);
                if (n>1)
                {
                        if ( (buf[0]=='*' && buf[1]=='S' && buf[2]=='C' && buf[3]=='R') ||
                             (buf[0]=='S' && buf[1]=='C' && buf[2]=='R') )
                        {
                                buf[strlen(buf)-1]=0;                                   // remove \n
                                if (buf[0]=='*')
                                        update_script_visuals(buf+1);
                                else update_script_visuals(buf);
                        }
                }
        } while (n>0);
}



void scripts_init()
{
	bzero(&scriptstext,sizeof(scriptstext));
}
