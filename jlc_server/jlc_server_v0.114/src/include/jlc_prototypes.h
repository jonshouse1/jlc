// Prototypes 

// jlc_monitor.c
void monitor_printf(int e,const char *fmt, ...);

// jlc_console.c
void init_console();
void console_poll();


// jlc_udp.c
int udp_generic_send(char *d, int len, char *destination_ip, int destination_port, int broadcast);

// jlc_tcpconn.c
void init_conn();
void conn_set_defaults(int i);
int conn_count();
int conn_find_index(int fd);
void conn_disconnect(int i, int caller);
void xprintf(int fd, const char *fmt, ...);
void conn_poll();
void conn_monitor(uint32_t e, char *msg);
void conn_set_binmode(int fd);
int init_tcpconn(int port);


// jlc_universe.c
void init_universe();
int universe_set_allchan(int textout, int u, int v);
int universe_set_colours(int textout, int u, char* colours, int v);
int universe_return_ch_value(int textout, int u, int c);
int universe_valid_range(int textout, int univ, int fl);
int channel_valid_range(int textout, int ch, int fl);
int universe_channel_set(int textout, int u, int c, int v);
int universe_check_if_idle(int u);
int universe_count(int mode);
int universe_destroy(int u);
void universe_initial_settings(int u);
int universe_create(int u, int noc);
void universe_summary(int u,char *c);
int universe_load(int textout, int u);
void universe_show(int textout, int u, int iscolour);
void list_universes(int textout, int u);
void universe_dump(int textout, int u);
void universe_timeout_check();
int universe_channel_currently_available(int u, int c);


// jcp.c
void list_ip_details(int textout);
int jcp_init(int port, int fl_reuseport);
void jcp_message_poll(int dsockfd);
void jcp_update_lastheard(char *ipaddr, struct jcp_header *jh);
int jcp_lastheard_ms(char *ipaddr);
int jcp_lastheard_s(char *ipaddr);
void jcp_timeout_check();
void jcp_announce(int port);
void jcp_send_payload(char *ipaddr, int port, int msg_type, char *payload, int payloadlen);



// jlc_commands.c
void jlc_cmd_quit(int textfd);

// jlc_interpreter.c
int interpreter(int textout, char *textline, int textlength, int verbose);
int cmd_colour(int textout);
int iscolour(int textout);


// jlc_runscript.c
int run_script(int textout, char *scriptname);

// jlc_time.c
void set_realtime(void);
//unsigned long long current_timems();
uint64_t current_timems();
int timetext_to_ms(char *text);
char* ms_to_timetext(unsigned long int tms);
void timer_hundredms();
void timer_onehz();
void timer_fast();
char* datetimes(int fmt);
void printsize(size_t size, char *st);
int te_clear(int i);
void init_te();
void te_list(int textout);
void te_set(int textout, int i, int intervalms, int rerun, char* cmd);
int te_add(int textout, int intervalms, int rerun, char* cmd);
int te_find_idx_by_cmd(int textout, char*cmd);



// jlc_devices.c
char* printuid(char unsigned *uid);
char* dev_type_name(int dev_type);
void dev_jlc_register(struct jcp_dev *d ,char *ipaddr);
void list_devices(int textout, char* w1, char* w2);
void list_map(int textout);
void dev_timeout(char *ipaddr);
int dev_count(int t);
void device_map_uc(int textout, char *uids, int univ, int fchan);
void device_map_group(int textout, char *uids, int group);
void device_group_value_change(int g);
void device_update_state(char *ipaddr, struct dev_state* ds);
void list_dev_state(int textout, char *uids);
void dumphex(const void* data, size_t size);
void dev_set_name(int textout, char* uids, char* name);
void dev_set_state(int textout, char*uids, char *value1s, char* value2s, char *valuebytess);
void list_temperatures(int textout);
void recntly_idle_groups();
void devf_clear(int i);
void devn_clear(int i);
void init_devf();
void init_devn();
void dev_hide(int textout, char*uids, char *value1s);
void dev_config(int textout, char*uids, char *cmd, char*text);
void dev_send_topic(int i, int topic);
int dev_lookup_idx_by_uid_string(int t, char *uids);
void dev_subs_event(int etype);
void list_subs(int textout, char *w);
void list_dev_monitor(int textout);
void list_dev_names(int textout);
void dev_playsound(int g, int v, char* samplename);



// jlc_dev_create_devfiles.c
void dev_create_devfiles(unsigned char *uid, int dev_type);


// jlc_mask.c
int mask_create(int textout, int uni, int noc, char*ptrn);
void mask_show(int textout, int univ);
int mask_valid(char *ptrn);
int mask_valid_char(char m);
char* masktoansi(char m, int bright);

// jlp.c
//void jlp_build_and_send_lighting_packet(unsigned char*uid, char *ipaddr, int udr_hz, int noc, int univ, int fchan);
void jlp_build_and_send_lighting_packet(unsigned char*uid, char *ipaddr, int jlp_port, int udr_hz, int noc, int univ, int fchan);
int jlp_light_sender();


// jlc_http
int init_http(int http_tcp_port);
void http_poll();


// jlc_group.c
void init_group();
int load_group(int textout, int g);
void group_reload(int textout);
int save_group(int textout, int g);
void list_group(int textout, int g);
void list_groups(int textout);
void list_group_channels(int textout, int g);
int group_set_value(int textout, int g, int R, int G, int B, int W, int I, int onoff);
int group_count();
int group_valid_range(int textout, int gr, int fl);
void group_apply_fx(int textout, int g, char* w1, char* w2, char* w3, char *w4);
int group_print(int textout, int g);
void group_onehz_tick();


// jlc_effects.c
void init_effects();
void effects_tick();
void list_fx(int textout);
int fx_lookup (char *fxname);
int fx_apply(int u, int c, int fx, int vals[FX_VALMAX]);



// jlc_isdaylight.c
int isdaylight();


// jlc_statusline.c
void sl_timer();
void slprintf(const char *fmt, ...);


// jlc_log.c
void log_printf(char* logfile, const char *fmt, ...);
//void log_temp_printf( const char *fmt, ...);
//void log_sen_printf( const char *fmt, ...);



// jlc_variable_subs.c
void printbytes(int textout, int bytes);
int variable_subs(char *word);
void variable_list(int textout);



// jlc_audioplus.c
void audioplus_clear_buffers();
int audioplus_findidx(int current_frame, int frames_behind);
int setup_audioplus_transmitting_socket(int port);
int setup_audioplus_receiving_socket(int port);
int slave_receive_audioplus(int ms_sockfd, char* ipstr, struct audioplus *ap_record);
void process_audioplus(struct audioplus *ap_record);
void audioplus_poll();
void audioplus_fake(int s);


// jlc_sound_sequencer
void seq_process_audioframe(char *md5hash, int framenumber);
int seq_load(int textout, char* filename);
int seq_count();
int seq_remove(int textout);
void seq_list(int textout);
int seq_test(int textout);
int seq_step(int textout);
void seq_debug(int textout, int torf);
int seqrunsequence_run(int textout, int s);


// jlc_binmsg.c
int bin_sendmsg(int fd, int msg_type, int idx);
void bin_sendmsgs(int msg_type, int idx);
void bin_send_state(int i);
void bin_onehz_tick();
int bin_msg_poll(int i, char* payload);
int bin_sendmsg_generic(int fd, int msg_type, int idx, char* payload, int payload_len);
void bin_sendmsg_generics(int msg_type, int idx, char* payload, int payload_len);


// jlc_colour_presets.c
void init_colour_presets();
int colour_presets_load(int textout);
int colour_presets_save(int textout);
void colour_presets_list(int textout);


