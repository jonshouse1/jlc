// jlc_gui_gtk.c
// apt install libgtk2.0-dev

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <gtk/gtk.h>

//#include <gst/gst.h>


//#include "mainform.h"
//#include "form_terminal.h"
//#include "jlc_gui_xforms_form_scripts.h"
//#include "jlc_gui_xforms_form_control.h"
#include "wavplay.h"


#include "jlc.h"
#include "jcp_protocol_devs.h"
#include "jlc_group.h"
#include "jlc_prototypes.h"


int                     lockcounter_top=70;

extern char		server_datetime[512];
extern int		online;							// connected to server ?
extern int		fd_terminal;
extern int		fd_monitor;
extern int		fd_binmsg;                         
int			selected_form=1;
int			font_height=0;
int			termlines=0;
int			scriptlines=0;
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

char			termbuf[1024*16];
char			termtext[1024*16];
char			scriptstext[8192];

// Two tables of device and device states
extern struct jlc_devf		devf[MAX_DEVF];
extern struct jlc_devn		devn[MAX_DEVN];

// Groups
extern struct jlc_group_cfg	grpc[MAX_GROUPS+1];
extern struct jlc_group_val	grpv[MAX_GROUPS+1];

// Table for timed events
extern struct timed_events	te[MAX_TIMED_EVENTS];



static void activate (GtkApplication* app, gpointer user_data)
{
	GtkWidget *window;
GtkWidget *vbox;
GtkWidget *label, *hbox, *elapsed, *scale;


	//printf("activate()\n"); fflush(stdout);
	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "Window");
	gtk_window_set_default_size (GTK_WINDOW (window), app_width, app_height);

vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
gtk_container_add (GTK_CONTAINER (window), vbox);
hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
label = gtk_label_new ("Elapsed: ");
elapsed = gtk_label_new ("0.0");
gtk_container_add (GTK_CONTAINER (hbox), label);
gtk_container_add (GTK_CONTAINER (hbox), elapsed);
gtk_container_add (GTK_CONTAINER (vbox), hbox);


/* volume */
hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
label = gtk_label_new ("volume");
gtk_container_add (GTK_CONTAINER (hbox), label);
scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, -90.0, 10.0, 0.2);
gtk_range_set_value (GTK_RANGE (scale), 0.0);
gtk_widget_set_size_request (scale, 100, -1);
gtk_container_add (GTK_CONTAINER (hbox), scale);
gtk_container_add (GTK_CONTAINER (vbox), hbox);
//g_signal_connect (scale, "value-changed", G_CALLBACK (value_changed_callback), volume);



	gtk_widget_show_all (window);
}



// Returns 0 for success
int jlc_gui_init_gtk()
{
	return(0);
}


void jlc_gui_defaults_gtk()
{
}


//GtkWidget *h_scale;


// Control returns when application closes
int jlc_gui_run_gtk()
{
        int argc=1;
        char argvn[32];
        char* argvx = (char*)&argvn;
        char** argv=&argvx;
	GtkApplication *app;
	int status;

        sprintf(argvn,"hello");
	app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);
	return status;
}




void jlc_change(int msg_type, int idx)
{
	switch (msg_type)
	{	
		case BIN_MSG_GROUP_CLEAR:
		break;

		case BIN_MSG_DEVF_CLEAR:
		break;

		case BIN_MSG_DEVN_CLEAR:
		break;

		case BIN_MSG_GROUP_CFG:
		break;

		case BIN_MSG_GROUP_VAL:
		break;

		case BIN_MSG_DEVF:
		break;

		case BIN_MSG_DEVN:
		break;

		case BIN_MSG_DATETIME:
		break;
	}
}



