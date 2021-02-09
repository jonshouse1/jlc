/*
	jcp_client.h
*/


// Prototypes
void jcp_init(int jcpport, int jlpport, int timertickinterval);
void jcp_init_b(char *uidb, uint16_t sid, int jcp_udp_iport, int jlp_udp_iport, int tick_interval_ms, char* device_model, char* device_fw_ver);
void jcp_parse_and_reply(char *data, int len, char *ipaddr);
void jcp_timertick_handler();

void jcp_register_dev(unsigned char idx, int dev_type, int noc, char* topics);
void jcp_subscribe(char *st);
void jcp_send_device_state(int idx, uint16_t value1, uint16_t value2, int asciiorbinary, char* valuebytes, int valuebyteslen);

// Callbacks from jcp_client.c to app
//void jcp_dev_state_cb(int idx, unsigned char* uid, int value1, int value2, char valuebytes[MAX_VALUEBYTESLEN]);
void jcp_dev_state_cb(int idx, struct dev_state* ds);
void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata);
void jcp_dev_config(char *dt, int len);


