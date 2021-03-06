/* Header file generated by fdesign on Thu Dec 12 21:28:45 2013 */

#ifndef FD_control_h_
#define FD_control_h_

#include  "../lib/include/forms.h" 

/* Callbacks, globals and object handlers */

void func_cb( FL_OBJECT *, long );
void form_cb( FL_OBJECT *, long );
void group_cb( FL_OBJECT *, long );
void object_cb( FL_OBJECT *, long );
void align_cb( FL_OBJECT *, long );
void test_cb( FL_OBJECT *, long );
void help_cb( FL_OBJECT *, long );
void optionmenu_callback( FL_OBJECT *, long );
void filemenu_callback( FL_OBJECT *, long );
void formmenu_callback( FL_OBJECT *, long );
void objectmenu_callback( FL_OBJECT *, long );
void groupmenu_callback( FL_OBJECT *, long );

void doalign_cb( FL_OBJECT *, long );
void snap_cb( FL_OBJECT *, long );
void undoalign_cb( FL_OBJECT *, long );
void exitalign_cb( FL_OBJECT *, long );

void stoptest_cb( FL_OBJECT *, long );
void clearlog_cb( FL_OBJECT *, long );

void showhelp_cb( FL_OBJECT *, long );
void exithelp_cb( FL_OBJECT *, long );



/* Forms and Objects */

typedef struct {
    FL_FORM   * control;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * title;
    FL_OBJECT * seltype;
    FL_OBJECT * oristatus;
    FL_OBJECT * fkey_group;
    FL_OBJECT * formbrowser;
    FL_OBJECT * groupbrowser;
    FL_OBJECT * sizestatus;
    FL_OBJECT * objectbrowser;
    FL_OBJECT * shortcut_group;
    FL_OBJECT * menubar_group;
    FL_OBJECT * optionmenu;
    FL_OBJECT * filemenu;
    FL_OBJECT * formmenu;
    FL_OBJECT * objectmenu;
    FL_OBJECT * groupmenu;
    FL_OBJECT * selname;
    FL_OBJECT * selcb;
} FD_control;

FD_control * create_form_control( void );
typedef struct {
    FL_FORM   * align;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * left;
    FL_OBJECT * hcenter;
    FL_OBJECT * right;
    FL_OBJECT * hequal;
    FL_OBJECT * snapobj;
    FL_OBJECT * bottom;
    FL_OBJECT * vcenter;
    FL_OBJECT * top;
    FL_OBJECT * vequal;
    FL_OBJECT * undo;
    FL_OBJECT * dismiss;
} FD_align;

FD_align * create_form_align( void );
typedef struct {
    FL_FORM   * test;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * browser;
    FL_OBJECT * stoptesting;
} FD_test;

FD_test * create_form_test( void );
typedef struct {
    FL_FORM   * helpform;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * browser;
    FL_OBJECT * vbutt;
} FD_helpform;

FD_helpform * create_form_helpform( void );
typedef struct {
    FL_FORM   * resize;
    void      * vdata;
    char      * cdata;
    long        ldata;
    FL_OBJECT * width;
    FL_OBJECT * height;
    FL_OBJECT * quit;
    FL_OBJECT * set_size;
} FD_resize;

FD_resize * create_form_resize( void );

#endif /* FD_control_h_ */
