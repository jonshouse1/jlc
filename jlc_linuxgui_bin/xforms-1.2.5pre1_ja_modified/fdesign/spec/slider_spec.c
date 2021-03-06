/* Form definition file generated by fdesign on Thu Dec 12 21:27:07 2013 */

#include <stdlib.h>
#include "slider_spec.h"



/***************************************
 ***************************************/

FD_sliderattrib *
create_form_sliderattrib( void )
{
    FL_OBJECT *obj;
    FD_sliderattrib *fdui = ( FD_sliderattrib * ) fl_malloc( sizeof *fdui );

    int old_bw = fl_get_border_width( );
    fl_set_border_width( -1 );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->sliderattrib = fl_bgn_form( FL_NO_BOX, 540, 280 );

    fdui->background = obj = fl_add_box( FL_FLAT_BOX, 0, 0, 540, 280, "" );

    obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 30, 20, 470, 240, "Slider Attributes" );
    fl_set_object_lsize( obj, FL_MEDIUM_SIZE );
    fl_set_object_lstyle( obj, FL_TIMESBOLD_STYLE );

    fdui->prec = obj = fl_add_counter( FL_NORMAL_COUNTER, 335, 135, 114, 23, "Precision" );
    fl_set_object_lalign( obj, FL_ALIGN_TOP );
    fl_set_object_callback( obj, adjust_precision, 0 );
    fl_set_counter_precision( obj, 0 );
    fl_set_counter_bounds( obj, 0, 0 );
    fl_set_counter_step( obj, 0, 0 );

    fdui->minval = obj = fl_add_input( FL_FLOAT_INPUT, 205, 41, 82, 25, "Value at bottom/left" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, minmax_change, 0 );

    fdui->maxval = obj = fl_add_input( FL_FLOAT_INPUT, 205, 72, 82, 25, "Value at top/right" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, minmax_change, 0 );

    fdui->returnsetting = obj = fl_add_menu( FL_PULLDOWN_MENU, 325, 80, 143, 24, "Return Setting" );
    fl_set_object_boxtype( obj, FL_UP_BOX );
    fl_set_object_callback( obj, returnsetting_change, 0 );

    fdui->initial_val = obj = fl_add_input( FL_FLOAT_INPUT, 205, 104, 82, 25, "Initial Value" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, initialvalue_change, 0 );

    fdui->slsize = obj = fl_add_input( FL_FLOAT_INPUT, 205, 136, 82, 25, "Slider Size" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, slsize_change, 0 );

    fdui->step = obj = fl_add_input( FL_FLOAT_INPUT, 205, 166, 82, 25, "StepSize" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, step_change, 0 );

    fdui->ldelta = obj = fl_add_input( FL_FLOAT_INPUT, 205, 196, 82, 25, "Leftmouse Increment" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, increment_change, 0 );

    fdui->rdelta = obj = fl_add_input( FL_FLOAT_INPUT, 205, 226, 82, 25, "Rightmouse Increment" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, increment_change, 0 );

    fl_end_form( );

    fdui->sliderattrib->fdui = fdui;
    fl_set_border_width( old_bw );

    return fdui;
}
