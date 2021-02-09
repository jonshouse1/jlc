// jlc_gui_xforms_form_control.h

#include <forms.h>
#include "jlc_linuxgui.h"


void slider_group_cb( FL_OBJECT *, long data );
void sensors_cb( FL_OBJECT *, long data );




typedef struct {
    FL_FORM   * form_control;
    void      * vdata;
    char      * cdata;
    long        ldata;

// First panel
    FL_OBJECT * frame_groups;
    FL_OBJECT * groupname[MAX_GROUP_VISUALS+1];
    FL_OBJECT * btn_group_off[MAX_GROUP_VISUALS+1];
    FL_OBJECT * slider_group[MAX_GROUP_VISUALS+1];
    FL_OBJECT * btn_group_on[MAX_GROUP_VISUALS+1];
    FL_OBJECT * rgbw_group[MAX_GROUP_VISUALS+1];
    //FL_OBJECT * group_control;

// Second panel
    FL_OBJECT * frame_tempsensor;
    FL_OBJECT * tempsens_box[MAX_TEMP_VISUALS+1];
    FL_OBJECT * tempsens_name[MAX_TEMP_VISUALS+1];
    FL_OBJECT * tempsens_value[MAX_TEMP_VISUALS+1];

// Third panel
    FL_OBJECT * frame_movesensor;
    FL_OBJECT * btn_movement_sensor_box[MAX_SENSOR_VISUALS+1];
    FL_OBJECT * btn_movement_sensor[MAX_SENSOR_VISUALS+1];

// Fourth panel
    FL_OBJECT * switchstate[MAX_SWITCH_VISUALS+1];
} FD_form_control;



// Prototypes
FD_form_control *form_control_create( int aw, int ah );
void form_control_defaults( FD_form_control* fdui );

void form_control_sensor_default(FD_form_control* fdui, int i);
void form_control_group_default(FD_form_control* fdui, int i);
void form_control_temp_default(FD_form_control* fdui, int i);
void form_control_switch_default(FD_form_control* fdui, int i);


