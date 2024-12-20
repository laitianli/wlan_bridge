#ifndef __STATISTICS_H__
#define __STATISTICS_H__
#include "uthash/include/uthash.h"
#include "common.h"

typedef struct _wlan_ip_statistics_
{
    u32 key_ip4_addr;
    u64 rx_packets;
    u64 rx_bytes;
    u64 rx_err;
    u64 tx_packets;
    u64 tx_bytes;
    u64 tx_err;
    UT_hash_handle hh;
}wlan_ip_statistics_t;

void wlan_ip_static_update_rx(u32 ip4_addr, u32 rx_packet, u32 rx_bytes);
void wlan_ip_static_update_tx(u32 ip4_addr, u32 tx_packet, u32 tx_byte);
void wlan_ip_dump_static(void *handle, dump_fn fn);

#endif