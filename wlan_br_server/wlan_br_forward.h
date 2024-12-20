#ifndef __WLAN_BR_FORWARD_H_
#define __WLAN_BR_FORWARD_H_
#include "common.h"
void wlan_br_handle(int len, unsigned char *buf, struct sockaddr_in *peer_addr);
void wlan_br_handle_burst(int recv_count, unsigned char *buf[], unsigned int data_len[], struct sockaddr_in peer_addr[]);
void wlan_br_dump_fdb(void *handle, dump_fn fn);
#endif