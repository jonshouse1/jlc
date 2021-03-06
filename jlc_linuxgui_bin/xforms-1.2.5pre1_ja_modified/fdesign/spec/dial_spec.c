/* Form definition file generated by fdesign on Thu Dec 12 21:27:07 2013 */

#include <stdlib.h>
#include "dial_spec.h"



/***************************************
 ***************************************/

FD_dialattrib *
create_form_dialattrib( void )
{
    FL_OBJECT *obj;
    FD_dialattrib *fdui = ( FD_dialattrib * ) fl_malloc( sizeof *fdui );

    int old_bw = fl_get_border_width( );
    fl_set_border_width( -1 );

    fdui->vdata = fdui->cdata = NULL;
    fdui->ldata = 0;

    fdui->dialattrib = fl_bgn_form( FL_NO_BOX, 540, 280 );

    fdui->background = obj = fl_add_box( FL_FLAT_BOX, 0, 0, 540, 280, "" );

    fdui->returnsetting = obj = fl_add_menu( FL_PULLDOWN_MENU, 305, 90, 138, 24, "Return Setting" );
    fl_set_object_boxtype( obj, FL_UP_BOX );
    fl_set_object_callback( obj, dial_returnsetting_change, 0 );

    fdui->minval = obj = fl_add_input( FL_FLOAT_INPUT, 195, 45, 92, 24, "Dial Min" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, dial_minmax_change, 0 );

    fdui->maxval = obj = fl_add_input( FL_FLOAT_INPUT, 195, 77, 92, 24, "DialMax" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, dial_minmax_change, 0 );

    fdui->initialval = obj = fl_add_input( FL_FLOAT_INPUT, 195, 109, 92, 24, "Initial Value" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, dial_initialvalue_change, 0 );

    fdui->step = obj = fl_add_input( FL_FLOAT_INPUT, 195, 141, 92, 24, "Dial Step" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, dial_stepchange_cb, 0 );

    fdui->thetai = obj = fl_add_input( FL_FLOAT_INPUT, 195, 173, 92, 24, "Min. Angle" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, dial_thetachange_cb, 0 );

    fdui->thetaf = obj = fl_add_input( FL_FLOAT_INPUT, 195, 205, 92, 24, "Max. Angle" );
    fl_set_object_lsize( obj, FL_NORMAL_SIZE );
    fl_set_object_callback( obj, dial_thetachange_cb, 1 );

    fdui->dir = obj = fl_add_choice( FL_NORMAL_CHOICE2, 306, 155, 134, 25, "Direction" );
    fl_set_object_lalign( obj, FL_ALIGN_TOP );
    fl_set_object_callback( obj, dir_cb, 0 );

    obj = fl_add_labelframe( FL_ENGRAVED_FRAME, 30, 15, 480, 240, "Dial Attributes" );
    fl_set_object_lcolor( obj, FL_DARKVIOLET );
    fl_set_object_lsize( obj, FL_MEDIUM_SIZE );
    fl_set_object_lstyle( obj, FL_TIMESBOLD_STYLE + FL_EMBOSSED_STYLE );

    fl_end_form( );

    fdui->dialattrib->fdui = fdui;
    fl_set_border_width( old_bw );

    return fdui;
}
