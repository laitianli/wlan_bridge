#ifndef __WLAN_FDB_H__
#define __WLAN_FDB_H__
#include "include/uthash.h"
#include "common.h"

typedef struct _wlan_fdb_entry_
{
    unsigned char key_remote_mac[ETH_ALEN];
    struct sockaddr_in peer_addr;
    char device_id[64];
    char seq_id[64];
    UT_hash_handle hh;
} wlan_fdb_entry_t;

int wlan_fdb_update(struct sockaddr_in *peer_addr, unsigned char *remote_mac);
wlan_fdb_entry_t *wlan_br_find(unsigned char *remote_mac);

#endif
