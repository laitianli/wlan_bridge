#ifndef __COMMON_H__
#define __COMMON_H__
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/if_ether.h>
#include "uthash/include/uthash.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char bool;

static inline char *ether_addr_to_str(unsigned char addr[ETH_ALEN], char *buf, int len)
{
    if (!buf || len < 17)
        return NULL;
    snprintf(buf, len, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return buf;
}

#define LOG_ERR(format, ...) printf("[Error][%s:%d]" format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define LOG_DBG(format, ...) printf("[Debug][%s:%d]" format "\n", __func__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(format, ...) printf("[INFO][%s:%d]" format "\n", __func__, __LINE__, ##__VA_ARGS__)
#if 0
#define LOG_ETHER_ADDR(peer_addr, eth, format, ...)                                            \
    do                                                                                         \
    {                                                                                          \
        char eth_addr[20];                                                                     \
        printf("[INFO][%s:%d][%s:%u][mac:%s]" format "\n", __func__, __LINE__,                 \
               inet_ntoa((struct in_addr)(peer_addr)->sin_addr), ntohs((peer_addr)->sin_port), \
               ether_addr_to_str(eth, eth_addr, sizeof(eth_addr)), ##__VA_ARGS__);             \
    } while (0)
#define LOG_IP_ETHER_ADDR(peer_addr, ether, format, ...)                                                       \
    do                                                                                                         \
    {                                                                                                          \
        char src_eth_addr[20], dest_eth_addr[20];                                                              \
        printf("[INFO][%s:%d][%s:%u][src mac:%s, dst mac: %s, proto: 0x%04x]" format "\n", __func__, __LINE__, \
               inet_ntoa((struct in_addr)(peer_addr)->sin_addr), ntohs((peer_addr)->sin_port),                 \
               ether_addr_to_str((ether)->h_source, src_eth_addr, sizeof(src_eth_addr)),                       \
               ether_addr_to_str((ether)->h_dest, dest_eth_addr, sizeof(dest_eth_addr)),                       \
               ntohs((ether)->h_proto), ##__VA_ARGS__);                                                        \
    } while (0)
#else
#define LOG_ETHER_ADDR(peer_addr, eth, format, ...)
#define LOG_IP_ETHER_ADDR(peer_addr, ether, format, ...)
#endif
#endif