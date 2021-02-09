// jlc_gui_xforms_form_devices.h

#include <forms.h>
//#include "jcp_protocol_devs.h"
#include "jlc_linuxgui.h"


//void slider_group_cb( FL_OBJECT *, long data );
//void sensors_cb( FL_OBJECT *, long data );




typedef struct {
    FL_FORM   * form_devices;
    void      * vdata;
    char      * cdata;
    long        ldata;

    //FL_OBJECT * frame_groups;
    FL_OBJECT * btn_devtype[NUM_DEVN_TYPES];
    FL_OBJECT * btn_dlist[64];
    FL_OBJECT * btn_base;
    FL_OBJECT * btn_details;


    // Info box
    //FL_OBJECT * label_info_uid;
    //FL_OBJECT * label_info_name;
    //FL_OBJECT * label_info_ipaddr; 
    FL_OBJECT *  infolines[16];


    // Switches along bottom
    FL_OBJECT * switchstate[MAX_SWITCH_VISUALS+1];
} FD_form_devices;



// Prototypes
FD_form_devices *form_devices_create( int aw, int ah );
void form_devices_default(FD_form_devices* fdui, int i);
void form_devices_defaults( FD_form_devices* fdui );



