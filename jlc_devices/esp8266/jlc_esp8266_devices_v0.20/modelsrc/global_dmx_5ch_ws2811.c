// global_dmx_5ch_ws2811.c


// GPIO, use -1 if the function is not available on the device
#pragma message ( "Building for DMX_5CH_WS2811" )
#define STATUS_LED      5                                                       // RED


// PWM
#define PWM_DEPTH       255
#define PWM_1S          1000000

// In the order Red, Green, Blue, W1, W2
#define PWM_CHANNELS    5                                                       // 5 channels
#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDO_U
#define PWM_0_OUT_IO_NUM 15
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO15

#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
#define PWM_1_OUT_IO_NUM 13
#define PWM_1_OUT_IO_FUNC  FUNC_GPIO13

#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_2_OUT_IO_NUM 12
#define PWM_2_OUT_IO_FUNC  FUNC_GPIO12

#define PWM_3_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
#define PWM_3_OUT_IO_NUM 14
#define PWM_3_OUT_IO_FUNC  FUNC_GPIO14

#define PWM_4_OUT_IO_MUX PERIPHS_IO_MUX_GPIO4_U
#define PWM_4_OUT_IO_NUM 4
#define PWM_4_OUT_IO_FUNC FUNC_GPIO4

// LEDs
int wifi_led = 5;


struct espconn conn1;
static esp_udp udpsd;

uint32 dt[PWM_CHANNELS+1];



// Prepare to flash device by stopping all timers
void ICACHE_FLASH_ATTR app_suspend_timers()
{
}


// Server is sending us new state.  idx is the index used when regisering the device. 
void jcp_dev_state_cb(int idx, struct dev_state* ds)
{
	if (idx==-100)								// devices are registered now
		return;
	os_printf("jcp_dev_state_cb()\tidx=%d\tds->value1=%d\tds->value2=%d\n",idx,ds->value1, ds->value2);
}



void jcp_topic_cb(int idx, unsigned char* uid, int topic, int tlen, unsigned char* tdata)
{
}



// Take an 8 bit number and scale it to a value suitable for the pwm generator
// pwm values for 1Khz are 0 to 22,000 - so 22,000/8=2750 per bit
uint32 ICACHE_FLASH_ATTR eightbitscale(int d)
{
        return(d*d);										// Log scale 8 bit to 16
}



static ICACHE_FLASH_ATTR void process_jlp_packet(char *ipaddr, unsigned char *data, int len)
{
	int c=0;
	struct jlp_pkt *p=(struct jlp_pkt*)data;

	if (len < 23)
	{
		os_printf("JLP packet too short\n");
		return;
	}

	if (strncmp(p->magic, JLP_MAGIC, 3)!=0)
	{
		os_printf("Bad magic from %s\n",ipaddr);
		return;
	}
	
	os_printf("Good packet from %s, NOC:%d ",ipaddr,p->nov);
//also see p->voffset, should be 0 for dmx but will move around for ws2811 data
	for (c=0;c<p->nov;c++)									// [0] through [4] 
	{
		os_printf("%d ",p->values[c]);
		dt[c]=eightbitscale(p->values[c]);
		pwm_set_duty(dt[c],c);
	}
	os_printf("\n");
}



static ICACHE_FLASH_ATTR void udprx_cb(void* arg, char* p_data, unsigned short len)
{
        struct espconn* conn = (struct espconn*)arg;
        char ipaddr[32];

        // Extract the IP address of sender as a string
        if(conn->type == ESPCONN_UDP)
        {
                remot_info *remote = NULL;
                if(espconn_get_connection_info(conn, &remote, 0) == 0)
                {
                        // Extract the IP address of sender as a string
                        yip2str((char*)&ipaddr,remote->remote_ip);
                        //os_printf("JLP, Got packet from [%s] %d bytes\n",ipaddr,len);
			process_jlp_packet((char*)&ipaddr, p_data, len);
        		pwm_start();
		}
	}
}



void ICACHE_FLASH_ATTR init_udprx( void )
{
	conn1.type = ESPCONN_UDP;
	conn1.state = ESPCONN_NONE;
	udpsd.local_port = JLP_SERVER_PORT+1;
	conn1.proto.udp = &udpsd;
	espconn_create(&conn1);
	espconn_regist_recvcb(&conn1, udprx_cb);
	os_printf("Listening for JLP UDP data on port %d\n",udpsd.local_port);
}


void ICACHE_FLASH_ATTR connected()
{
	build_send_regiserdevs();
}


