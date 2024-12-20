#include <linux/if_ether.h>
#include "uthash/include/utlist.h"
#include "udp.h"
#include "wlan_br_forward.h"
#include "wlan_fdb.h"
#include "statistics.h"

typedef struct _peer_addr_info_
{
    struct sockaddr_in addr;
    unsigned short count;
    unsigned short flood_flag;
} peer_addr_info_t;

typedef struct _wlan_list_entry_
{
    unsigned char key_remote_mac[ETH_ALEN];
    peer_addr_info_t *peer_addr;
    struct _wlan_list_entry_ *next, *prev;
} wlan_list_entry_t;

static wlan_list_entry_t *l_head = NULL;

static inline bool is_multicast_ether_addr(const u8 *addr)
{
    u16 a = *(const u16 *)addr;
    return 0x01 & a;
}

static inline bool is_zero_ether_addr(const u8 *addr)
{
    return (*(const u16 *)(addr + 0) |
            *(const u16 *)(addr + 2) |
            *(const u16 *)(addr + 4)) == 0;
}

static inline bool is_broadcast_ether_addr(const u8 *addr)
{
    return (*(const u16 *)(addr + 0) &
            *(const u16 *)(addr + 2) &
            *(const u16 *)(addr + 4)) == 0xffff;
}

static inline bool is_unicast_ether_addr(const u8 *addr)
{
    return !is_multicast_ether_addr(addr);
}

static inline bool is_valid_ether_addr(const u8 *addr)
{
    /* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
     * explicitly check for it here. */
    return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
}

static inline bool ether_addr_equal(const u8 *addr1, const u8 *addr2)
{
    const u16 *a = (const u16 *)addr1;
    const u16 *b = (const u16 *)addr2;

    return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2])) == 0;
}

static inline void ether_addr_copy(u8 *dst, const u8 *src)
{
    u16 *a = (u16 *)dst;
    const u16 *b = (const u16 *)src;

    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
}

int wlan_br_forward(int len, unsigned char *buf, unsigned char *dest_mac, struct sockaddr_in *curr_addr)
{
    wlan_fdb_entry_t *entry = wlan_br_find(dest_mac);
    if (entry)
    {
        if (entry->peer_addr.sin_addr.s_addr == curr_addr->sin_addr.s_addr &&
            entry->peer_addr.sin_port == curr_addr->sin_port)
        {
            return 0;
        }
        LOG_ETHER_ADDR(&entry->peer_addr, entry->key_remote_mac, "send udp to data len: %d", len);
        int send_len = send_data_to_udp_client(len, buf, &entry->peer_addr);
        if (send_len != len)
        {
            LOG_ERR("send_data_to_udp_client failed, send_len: %d, len: %d", send_len, len);
            return -2;
        }
        return send_len;
    }
    return -1;
}
int wlan_br_forward_to_array(int len, unsigned char *buf, unsigned char *dest_mac, struct sockaddr_in *curr_addr,
                             unsigned int *to_data_len, unsigned char **to_buf, struct sockaddr_in *to_peer_addr)
{
    wlan_fdb_entry_t *entry = wlan_br_find(dest_mac);
    if (entry)
    {
        wlan_ip_static_update_rx(curr_addr->sin_addr.s_addr, 1, len);
        if (entry->peer_addr.sin_addr.s_addr == curr_addr->sin_addr.s_addr &&
            entry->peer_addr.sin_port == curr_addr->sin_port)
        {
            return 0;
        }
        LOG_ETHER_ADDR(&entry->peer_addr, entry->key_remote_mac, "send udp to data len: %d", len);
        *to_data_len = len;
        *to_buf = buf;
        memcpy(to_peer_addr, &entry->peer_addr, sizeof(struct sockaddr_in));
        wlan_ip_static_update_tx(to_peer_addr->sin_addr.s_addr, 1, len);
        return 1;
    }
    return -1;
}

void wlan_br_dump_fdb(void *handle, dump_fn fn)
{
    peer_addr_info_t *pa = NULL;
    wlan_list_entry_t *entry, *tmp, *tmp2;
    char eth_addr[20];
    char str_buf[128] = {0};
    CDL_FOREACH_SAFE(l_head, entry, tmp, tmp2)
    {
        pa = entry->peer_addr;
        snprintf(str_buf, sizeof(str_buf) - 1, "%15s:%-5d -> %s", inet_ntoa((struct in_addr)pa->addr.sin_addr), ntohs(pa->addr.sin_port),
           ether_addr_to_str(entry->key_remote_mac, eth_addr, sizeof(eth_addr)));
        fn(handle, str_buf);
    }
}

void wlan_br_flood(int len, unsigned char *buf, struct sockaddr_in *curr_addr)
{
    wlan_ip_static_update_rx(curr_addr->sin_addr.s_addr, 1, len);
    peer_addr_info_t *pa = NULL;
    wlan_list_entry_t *entry, *tmp, *tmp2;
    CDL_FOREACH_SAFE(l_head, entry, tmp, tmp2)
    {
        pa = entry->peer_addr;
        if (!pa)
        {
            LOG_ERR("peer addr is null!!!");
            continue;
        }
        if (pa->flood_flag == 1 ||
            (pa->addr.sin_addr.s_addr == curr_addr->sin_addr.s_addr &&
             pa->addr.sin_port == curr_addr->sin_port))
        {
            continue;
        }

        pa->flood_flag = 1;
        /* skip mul mac same ip */
        LOG_ETHER_ADDR(&pa->addr, entry->key_remote_mac, "send udp to data len: %d", len);
        int send_len = send_data_to_udp_client(len, buf, &pa->addr);
        if (send_len != len)
        {
            LOG_ERR("send_data_to_udp_client failed, send_len: %d, len: %d", send_len, len);
            continue;
        }
        wlan_ip_static_update_tx(pa->addr.sin_addr.s_addr, 1, len);
    }
    /*clear flood flag */
    CDL_FOREACH_SAFE(l_head, entry, tmp, tmp2)
    {
        pa = entry->peer_addr;
        if (!pa)
        {
            LOG_ERR("peer addr is null!!!");
            continue;
        }
        pa->flood_flag = 0;
    }
}

static int wlan_list_update(struct sockaddr_in *peer_addr, unsigned char *remote_mac)
{
    wlan_list_entry_t *elem;
    peer_addr_info_t *pa = NULL;
    u8 has_find = 0;
    elem = calloc(1UL, sizeof(wlan_list_entry_t));
    if (!elem)
    {
        LOG_ERR("malloc memory fail!");
        return 0;
    }
    memset(elem, 0, sizeof(wlan_list_entry_t));
    memcpy(elem->key_remote_mac, remote_mac, ETH_ALEN);

    wlan_list_entry_t *entry, *tmp, *tmp2;
    CDL_FOREACH_SAFE(l_head, entry, tmp, tmp2)
    {
        if (entry->peer_addr->addr.sin_addr.s_addr == peer_addr->sin_addr.s_addr &&
            entry->peer_addr->addr.sin_port == peer_addr->sin_port)
        {
            pa = entry->peer_addr;
            pa->count++;
            has_find = 1;
            pa->flood_flag = 0;
            break;
        }
    }
    if (!has_find)
    {
        pa = calloc(1UL, sizeof(peer_addr_info_t));
        if (!pa)
        {
            LOG_ERR("malloc memory fail!");
            return 0;
        }
        memset(pa, 0, sizeof(peer_addr_info_t));
        memcpy(&pa->addr, peer_addr, sizeof(struct sockaddr_in));
        pa->count = 1;
        pa->flood_flag = 0;
    }
    elem->peer_addr = pa;
    CDL_PREPEND(l_head, elem);
    return 1;
}

void wlan_br_handle(int len, unsigned char *buf, struct sockaddr_in *peer_addr)
{
    struct ethhdr *ether = (struct ethhdr *)buf;
    LOG_IP_ETHER_ADDR(peer_addr, ether, "recv udp data len: %d", len);
    if (wlan_fdb_update(peer_addr, ether->h_source) == 1)
    {
        wlan_list_update(peer_addr, ether->h_source);
    }
    if (is_multicast_ether_addr(ether->h_dest))
    {
        wlan_br_flood(len, buf, peer_addr);
    }
    else
    {
        int ret = wlan_br_forward(len, buf, ether->h_dest, peer_addr);
        if (-1 == ret)
        {
            wlan_br_flood(len, buf, peer_addr);
        }
    }
}

void wlan_br_handle_burst(int recv_count, unsigned char *buf[], unsigned int data_len[], struct sockaddr_in peer_addr[])
{
    int i = 0;
    unsigned int to_data_len[recv_count];
    unsigned char *to_buf[recv_count];
    struct sockaddr_in to_peer_addr[recv_count];
    int j = 0;
    for (i = 0; i < recv_count; i++)
    {
        struct ethhdr *ether = (struct ethhdr *)buf[i];
        LOG_IP_ETHER_ADDR(&peer_addr[i], ether, "[%d:%d]recv udp data len: %d", recv_count, i, data_len[i]);
        if (wlan_fdb_update(&peer_addr[i], ether->h_source) == 1)
        {
            wlan_list_update(&peer_addr[i], ether->h_source);
        }
        if (is_multicast_ether_addr(ether->h_dest))
        {
            wlan_br_flood(data_len[i], buf[i], &peer_addr[i]);
        }
        else
        {
            int ret = wlan_br_forward_to_array(data_len[i], buf[i], ether->h_dest, &peer_addr[i],
                                               &to_data_len[j], &to_buf[j], &to_peer_addr[j]);
            if (1 == ret)
            {
                j++;
            }
            else if (-1 == ret)
            {
                wlan_br_flood(data_len[i], buf[i], &peer_addr[i]);
            }
        }
    }
    if (j > 0)
    {
        send_data_to_udp_client_burst(to_buf, to_data_len, to_peer_addr, j);
    }
}