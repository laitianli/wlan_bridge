#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/if_ether.h>

#define LOG_ERR(format, ...) printf("[Error][%s:%d]" format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define LOG_DBG(format, ...) printf("[Debug][%s:%d]" format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...) printf("[INFO][%s:%d]" format "\n", __func__, __LINE__, ##__VA_ARGS__)
#if 1
#define LOG_ETHER_ADDR(ether, format, ...)                                                              \
    do                                                                                                  \
    {                                                                                                   \
        char src_eth_addr[20], dest_eth_addr[20];                                                       \
        printf("[INFO][%s:%d][src mac:%s, dst mac: %s, proto: 0x%04x]" format "\n", __func__, __LINE__, \
               ether_addr_to_str((ether)->h_source, src_eth_addr, sizeof(src_eth_addr)),                \
               ether_addr_to_str((ether)->h_dest, dest_eth_addr, sizeof(dest_eth_addr)),                \
               ntohs((ether)->h_proto), ##__VA_ARGS__);                                                 \
    } while (0)
#else
#define LOG_ETHER_ADDR(ether, format, ...)
#endif
typedef int (*recv_tuntap_fn)(int len, unsigned char *buf);

int create_tuntap(const char *nic_name);
void destroy_tuntap(void);
int send_data_use_tuntap(int len, unsigned char *buf);
int read_data_from_tuntap(int max_len, unsigned char *buf);
int read_data_from_tuntap_burst(int max_len, int data_len[], unsigned char *buf[], int max_msg_mun);
int send_data_use_tuntap_burst(int data_len[], unsigned char *buf[], int msg_mun);

int create_udp_client(unsigned short port, const char *server_ip);
int send_data_use_udp(int len, unsigned char *buf);
int read_data_from_udp(int max_len, unsigned char *buf);
int send_data_use_udp_burst(unsigned char *buf[], unsigned int data_len[], int msg_num);
int read_data_from_udp_burst(int max_len, unsigned char *buf[], unsigned int data_len[], int max_num);
void destroy_udp_client(void);

static inline char *ether_addr_to_str(unsigned char addr[ETH_ALEN], char *buf, int len)
{
    if (!buf || len < 17)
        return NULL;
    snprintf(buf, len, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return buf;
}

#endif
