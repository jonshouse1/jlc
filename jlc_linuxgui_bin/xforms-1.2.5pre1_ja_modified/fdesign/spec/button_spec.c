/* Form definition file generated by fdesign on Thu Dec 12 21:27:07 2013 */

#include <stdlib.h>
#include "button_spec.h"



/***************************************
 ***************************************/

FD_buttonattrib *
create_form_buttonattrib( void )
{
    FL_OBJECT *obj;
    FD_buttonattrib *fdui = ( FD_buttonattrib * ) fl_malloc( sizeof *fdui );

    int old_bw = fl_get_border_width( );
    fl_set_border_width( -1 );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->buttonattrib = fl_bgn_form( FL_NO_BOX, 540, 280 );

    fdui->background = obj = fl_add_box( FL_FLAT_BOX, 0, 0, 540, 280, "" );

    obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 50, 20, 490, 240, "Button Attributes" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_lstyle( obj, FL_TIMESBOLD_STYLE );

    fdui->initialval = obj = fl_add_checkbutton( FL_PUSH_BUTTON, 160, 60, 30, 30, "Initially Set" );
    fl_set_object_lalign( obj, FL_ALIGN_LEFT );
    fl_set_object_callback( obj, initialval_change, 0 );

    fdui->filename = obj = fl_add_input( FL_NORMAL_INPUT, 160, 130, 210, 25, "Icon File" );
    fl_set_object_callback( obj, iconbutton_filename_change, 0 );

    fdui->pixalign = obj = fl_add_choice( FL_NORMAL_CHOICE2, 280, 80, 155, 25, "" );
    fl_set_object_callback( obj, pixalign_change, 0 );

    fdui->showfocus = obj = fl_add_checkbutton( FL_PUSH_BUTTON, 160, 90, 30, 30, "Show Focus" );
    fl_set_object_lalign( obj, FL_ALIGN_LEFT );
    fl_set_object_callback( obj, showfocus_change, 0 );
    fl_set_button( obj, 1 );

    fdui->browse = obj = fl_add_button( FL_NORMAL_BUTTON, 370, 130, 66, 25, "Browse ...." );
    fl_set_button_shortcut( obj, "#B", 1 );
    fl_set_object_callback( obj, lookfor_pixmapfile_cb, 0 );

    fdui->use_data = obj = fl_add_checkbutton( FL_PUSH_BUTTON, 220, 210, 30, 30, "'#include' image file" );
    fl_set_object_lalign( obj, FL_ALIGN_LEFT );
    fl_set_object_callback( obj, usedata_change, 0 );

    fdui->fullpath = obj = fl_add_checkbutton( FL_PUSH_BUTTON, 360, 210, 30, 30, "Use full path" );
    fl_set_object_lalign( obj, FL_ALIGN_LEFT );
    fl_set_object_callback( obj, fullpath_cb, 0 );

    fdui->focus_filename = obj = fl_add_input( FL_NORMAL_INPUT, 160, 170, 210, 25, "Focus icon File" );
    fl_set_object_callback( obj, focusiconbutton_filename_change, 0 );

    fdui->browse2 = obj = fl_add_button( FL_NORMAL_BUTTON, 370, 170, 66, 25, "Browse ...." );
    fl_set_object_callback( obj, lookfor_pixmapfile_cb, 1 );

    fdui->mbuttons_label = obj = fl_add_text( FL_NORMAL_TEXT, 60, 40, 100, 20, "Mouse buttons" );
    fl_set_object_lalign( obj, FL_ALIGN_CENTER );

    fdui->react_left = obj = fl_add_roundbutton( FL_PUSH_BUTTON, 160, 40, 20, 20, "Left" );
    fl_set_object_callback( obj, react_to_button, 0 );
    fl_set_button( obj, 1 );

    fdui->react_middle = obj = fl_add_roundbutton( FL_PUSH_BUTTON, 210, 40, 20, 20, "Middle" );
    fl_set_object_callback( obj, react_to_button, 1 );
    fl_set_button( obj, 1 );

    fdui->react_right = obj = fl_add_roundbutton( FL_PUSH_BUTTON, 280, 40, 20, 20, "Right" );
    fl_set_object_callback( obj, react_to_button, 2 );
    fl_set_button( obj, 1 );

    fdui->react_up = obj = fl_add_roundbutton( FL_PUSH_BUTTON, 340, 40, 20, 20, "Up" );
    fl_set_object_callback( obj, react_to_button, 3 );
    fl_set_button( obj, 1 );

    fdui->react_down = obj = fl_add_roundbutton( FL_PUSH_BUTTON, 390, 40, 20, 20, "Down" );
    fl_set_object_callback( obj, react_to_button, 4 );
    fl_set_button( obj, 1 );

    fl_end_form( );

    fdui->buttonattrib->fdui = fdui;
    fl_set_border_width( old_bw );

    return fdui;
}
