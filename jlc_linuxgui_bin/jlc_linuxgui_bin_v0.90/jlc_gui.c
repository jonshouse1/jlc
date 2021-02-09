// jlc_gui.c
// Generic wrapper for GUI code


#include <stdio.h>
#include <ctype.h>

#include "jlc.h"
#include "jlc_prototypes.h"


// Returns 0 for success
int jlc_gui_init()
{
#ifdef GUIXFORMS
	jlc_gui_init_xforms();
#endif
#ifdef GUIGTK
	jlc_gui_init_gtk();
#warning WITH GTK
#endif
	return(0);
}


int jlc_gui_defaults()
{
#ifdef GUIXFORMS
	jlc_gui_defaults_xforms();
#endif
#ifdef GUIGTK
	jlc_gui_defaults_gtk();
#endif
	return(0);
}



// Control returns only if the application closes, all interaction is now via callbacks
int jlc_gui_run()
{
#ifdef GUIXFORMS
	return(jlc_gui_run_xforms());
#endif
#ifdef GUIGTK
	return(jlc_gui_run_gtk());
#endif
}


