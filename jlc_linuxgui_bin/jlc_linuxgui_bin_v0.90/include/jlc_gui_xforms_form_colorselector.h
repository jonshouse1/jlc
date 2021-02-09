// jlc_gui_xforms_form_colorselector.h
#include <forms.h>


typedef struct {
    FL_FORM   * colorsel;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * btn_ok;
    FL_OBJECT * txt_r;
    FL_OBJECT * txt_g;
    FL_OBJECT * txt_b;
    FL_OBJECT * colorobj;
    FL_OBJECT * colorselectorgraphic;
    FL_OBJECT * colourpreset[17];
    FL_OBJECT * slider_w;
} FD_form_colorsel;



// Prototypes
FD_form_colorsel *create_form_colorsel( int aw, int ah );
