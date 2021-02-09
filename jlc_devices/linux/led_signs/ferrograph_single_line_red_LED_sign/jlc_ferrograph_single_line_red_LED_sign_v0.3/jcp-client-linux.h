// jcp-client-linux.h

unsigned long long current_timems();
int getmac(char *iface, char *mac);
int jcp_udp_listen(int port);
void jcp_message_poll(int dsockfd);
char* printuid(char unsigned *uid);
int jcp_linux_udplisten();
int jcp_poll();

