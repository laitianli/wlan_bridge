#include "statistics.h"
static wlan_ip_statistics_t *h_ip_stat_head = NULL;
static inline wlan_ip_statistics_t *wlan_ip_static_find(u32 ip4_addr)
{
    wlan_ip_statistics_t *e = NULL;
    HASH_FIND_INT(h_ip_stat_head, &ip4_addr, e);
    return e;
}

void wlan_ip_static_update_rx(u32 ip4_addr, u32 rx_packet, u32 rx_byte)
{
    wlan_ip_statistics_t *e = NULL;
    if ((e = wlan_ip_static_find(ip4_addr)) == NULL)
    {
        e = calloc(1UL, sizeof(wlan_ip_statistics_t));
        if (!e)
        {
            LOG_ERR("malloc memory fail!");
            return;
        }
        memset(e, 0, sizeof(wlan_ip_statistics_t));
        e->key_ip4_addr = ip4_addr;
        HASH_ADD_INT(h_ip_stat_head, key_ip4_addr, e);
    }
    e->rx_packets += rx_packet;
    e->rx_bytes += rx_byte;
    
}

void wlan_ip_static_update_tx(u32 ip4_addr, u32 tx_packet, u32 tx_byte)
{
    wlan_ip_statistics_t *e = NULL;
    if ((e = wlan_ip_static_find(ip4_addr)) == NULL)
    {
        e = calloc(1UL, sizeof(wlan_ip_statistics_t));
        if (!e)
        {
            LOG_ERR("malloc memory fail!");
            return;
        }
        memset(e, 0, sizeof(wlan_ip_statistics_t));
        e->key_ip4_addr = ip4_addr;
        HASH_ADD_INT(h_ip_stat_head, key_ip4_addr, e);
    }
    e->tx_packets += tx_packet;
    e->tx_bytes += tx_byte;
}

void wlan_ip_dump_static(void *handle, dump_fn fn)
{
    wlan_ip_statistics_t *ce;
    char str_buf[256] = {0};
    struct in_addr a = {0};
    for (ce = h_ip_stat_head; ce != NULL; ce = (wlan_ip_statistics_t *)(ce->hh.next))
    {
        a.s_addr = ce->key_ip4_addr;
        snprintf(str_buf, sizeof(str_buf) - 1, "%15s: rx_packets: %-8llu tx_packets: %-8llu rx_bytes: %-10llu tx_bytes: %-10llu",
                 inet_ntoa(a), ce->rx_packets, ce->tx_packets, ce->rx_bytes, ce->tx_bytes);
        fn(handle, str_buf);
    }
}