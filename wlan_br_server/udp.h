#ifndef __UDP__H__
#define __UDP__H__
#include "common.h"
int create_udp_server(unsigned short port);
int send_data_to_udp_client(int len, unsigned char *buf, struct sockaddr_in *peer_addr);
int send_data_to_udp_client_burst(unsigned char *buf[], unsigned int data_len[], struct sockaddr_in peer_addr[], int msg_num);
int read_data_from_udp_client(int max_len, unsigned char *buf, struct sockaddr_in *peer_addr);
int read_data_from_udp_client_burst(int max_len, unsigned char *buf[], unsigned int data_len[], struct sockaddr_in peer_addr[], int max_num);
void destroy_udp_server(void);

#endif