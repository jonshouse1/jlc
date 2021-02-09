// jlc_gui_xforms_form_terminal.c

#include <stdlib.h>
#include "form_terminal.h"
#include "jlc.h"
#include "jlc_prototypes.h"

extern int			app_width;
extern int			app_height;

extern int			app_width;
extern int			app_height;
extern int			font_height;
extern int			termlines;
extern FD_form_terminal        *fd_form_terminal;
extern char             server_ipaddr[32];
extern int			fd_terminal;


char                  termbuf[1024*16];
char                  termtext[1024*16];



// Across Down Width height


FD_form_terminal *form_terminal_create( int aw, int ah )
{
    FL_OBJECT *obj;
    FD_form_terminal *fdui = ( FD_form_terminal * ) fl_malloc( sizeof *fdui );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->form_terminal = fl_bgn_form( FL_NO_BOX, aw, ah );

    obj = fl_add_box( FL_FLAT_BOX, 0, 0, 879, 732, "" );
    fl_set_object_color( obj, FL_BLACK, FL_WHITE );
    fl_set_object_lcolor( obj, FL_WHITE );

    fdui->form_terminal_timer = obj = fl_add_timer( FL_HIDDEN_TIMER, 10, 10, 20, 20, "timer" );
    fl_set_object_lalign( obj, FL_ALIGN_CENTER );
    fl_set_object_callback( obj, terminalform_timer_cb, 0 );

    fdui->termtext = obj = fl_add_text( FL_NORMAL_TEXT, 0, 0, app_width, app_height, "text" );
    fl_set_object_color( obj, FL_BLACK, FL_WHITE );
    fl_set_object_lcolor( obj, FL_WHITE );
    fl_set_object_lsize( obj, FL_SMALL_SIZE );
    fl_set_object_lstyle( obj, FL_FIXED_STYLE );
    fl_set_object_lalign( obj, FL_ALIGN_LEFT_TOP | FL_ALIGN_INSIDE );


    // Command line entry
    fdui->terminput = obj = fl_add_input( FL_NORMAL_INPUT, 0, app_height-34-font_height, app_width-44, 30, "terminput" );
    //fl_set_input_return(obj, FL_RETURN_END);
    fl_set_object_callback( obj, terminput_cb, 10 );


    // Launches new terminal window
    fdui->termlauncher = obj = fl_add_button( FL_NORMAL_BUTTON, app_width-43, app_height-34-font_height, 40, 28, "TERM"  );
    fl_set_object_callback( obj, termlauncher_cb, 1001 );
    fl_set_object_color( obj, FL_DARKGREEN, FL_WHITE );
    fl_set_object_lcolor( obj, FL_WHITE );


    fl_end_form( );
    fdui->form_terminal->fdui = fdui;
    return fdui;
}






void terminal_poll()
{
	int n=0;
                
	do
	{
		n=poll_tcpsocket(fd_terminal, (char*)&termbuf);
		if (n>0)
		{
			strcat(termtext,termbuf);
			trimbuf(termtext,sizeof(termtext),termlines);
			expand_tabs(termtext,sizeof(termtext));
			fl_set_object_label(fd_form_terminal->termtext,termtext);
		}
	} while (n>0);
}




void terminal_init()
{
        bzero(&termtext,sizeof(termtext));
        bzero(&termbuf,sizeof(termbuf));
}



void terminput_cb( FL_OBJECT * obj, long data )
{
        char st[8192];
        char st1[8200];

        //if (blockme==TRUE)
        //{
                //blockme=FALSE;
                //return;
        //}
        printf("terminput_cb data=%d\n",(int)data); fflush(stdout);
        strcpy(st,fl_get_input(obj));
        sprintf(st1,"%s\n",st);
        xprintf(fd_terminal,"%s\r",st);

        strcat(termtext,st1);
        fl_set_input(obj, "");

        termlines=((app_height-30) / font_height) - 2;
        trimbuf(termtext,sizeof(termtext),termlines);
}


void termlauncher_cb( FL_OBJECT * obj, long data )
{
        char cmd[2048];

        printf("termlauncher_cb data=%d\n",(int)data); fflush(stdout);
        //blockme=TRUE;
        sprintf(cmd,"xterm -geom 132x40 -bg darkgreen -fg white -fa 'Monospace' -fs 14 -e telnet %s 1111 &",server_ipaddr);
        system(cmd);
}



void terminalform_timer_cb( FL_OBJECT * obj, long  data )
{
        //printf("terminalform_timer_cb()\n"); fflush(stdout);
        idle_callback();
        fl_set_timer(fd_form_terminal->form_terminal_timer,0.001);
}




