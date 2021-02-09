// jlc_gui_xforms_form_scripts.h


#include <forms.h>



void form_scripts_timer_cb( FL_OBJECT *, long );
//void scriptsinput_cb( FL_OBJECT *, long );


/* Forms and Objects */

typedef struct {
    FL_FORM   * form_scripts;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * scriptsform_timer;
    FL_OBJECT * scriptstext;
    //FL_OBJECT * scriptsinput;
} FD_form_scripts;

FD_form_scripts * form_scripts_create( int aw, int ah );


