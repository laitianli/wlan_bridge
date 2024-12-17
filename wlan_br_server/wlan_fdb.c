#include "wlan_fdb.h"

static wlan_fdb_entry_t *h_head = NULL;
wlan_fdb_entry_t *wlan_br_find(unsigned char *remote_mac)
{
    wlan_fdb_entry_t *t = NULL;
    HASH_FIND(hh, h_head, remote_mac, ETH_ALEN, t);
    return t;
}

int wlan_fdb_update(struct sockaddr_in *peer_addr, unsigned char *remote_mac)
{
    wlan_fdb_entry_t *entry = wlan_br_find(remote_mac);
    if (entry)
    {
        /* update time*/
        return 0;
    }
    entry = calloc(1UL, sizeof(wlan_fdb_entry_t));
    if (!entry)
    {
        LOG_ERR("malloc memory fail!");
        return -1;
    }
    memset(entry, 0, sizeof(wlan_fdb_entry_t));
    memcpy(entry->key_remote_mac, remote_mac, ETH_ALEN);
    memcpy(&entry->peer_addr, peer_addr, sizeof(struct sockaddr_in));
    /* device_id, seq_id */

    HASH_ADD(hh, h_head, key_remote_mac, ETH_ALEN, entry);
    return 1;
}
