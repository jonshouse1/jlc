#include <netinet/in.h>
#include <forms.h>


// jlc_linuxgui.c
void dumphex(const void* data, size_t size);
void monitor_printf(int e, const char *fmt, ...);
char* datetimes(int fmt);
char* dev_type_name(int dev_type);
void connect_to_server();
void init_group();
void init_devf();
void init_devn();
void init_screenblanker();
unsigned long long current_timems();
unsigned long current_times();
char* gltype_to_text(int gltype);
void conn_disconnect(int i, int caller);
void conn_set_defaults(int i);



// jlc_linuxgui_tcp.c
char* printuid(char unsigned *uid);
int sockinit(struct sockaddr_in* server, char* server_ipaddr, int server_tcpport, int nonblock);
int poll_tcpsocket(int sockfd, char*buf);
int align_for_binary_mode(int sockfd);
void xprintf(int fd, const char *fmt, ...);


// jlc_gui.c
int jlc_gui_init();
int jlc_gui_defaults();
//void jlc_gchange(int msg_type, int idx);
int jlc_gui_init();
int jlc_gui_run();


// jlc_gui_xforms.c
void expand_tabs(char *txt, int txtsize);
void persistant_binmsg_poll();
void reap_all();
void screenblanker_screenon();
void screenblanker_screenoff();
void group_clear(int g);
void devf_clear(int i);
void devn_clear(int i);
void idle_callback();
int jlc_gui_init_xforms();
void jlc_gui_defaults_xforms();
int jlc_gui_run_xforms();
void jlc_change(int msg_type, int idx);
void trimbuf( char* txt, int txtsize, int lines);
void init_slider_lockoutcounters();


// jlc_binmsg.c
void print_header(struct bin_header* hdr);
int bin_msg_poll(int i, char* payload);



// parse_commandline.c
int parse_commandlineargs(int argc,char **argv,char *findthis);
int parse_findargument(int argc,char **argv,char *findthis);



// jlc_gui_xforms_form_devices.c
void form_device_jlc_change(int msg_type, int idx);
void form_devices_activate();



// jlc_gui_xforms_form_control.c
void form_control_jlc_change(int msg_type, int idx);
void modify_pixmap_rgbbar(FL_OBJECT *obj, int r, int g, int b, int w);


// jlc_gui_xforms_form_terminal.c
void terminal_poll();
void terminal_init();


// jlc_gui_xforms_form_scripts.c
void update_script_visuals(char *textline);
void scripts_init();
void scripts_poll();



// jlc_gui_gtk.c
int jlc_gui_init_gtk();
void jlc_gui_defaults_gtk();
int jlc_gui_run_gtk();
void jlc_change(int msg_type, int idx);


// jcp_discover.c
int jcp_find_server(char* ipaddr, int timeoutms);


// get_x11_colour_under_cursor.c
void getcolor_under_cursor(int x, int y, unsigned int* R, unsigned int* G, unsigned int *B);


// jlc_gui_xforms_form_colorselector.c
void colorselector_form_defaults(int g);
void form_colorselector_jlc_change(int msg_type, int idx);



