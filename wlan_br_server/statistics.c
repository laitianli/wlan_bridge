#include <time.h>
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
    e->curr_stat.rx_packets += rx_packet;
    e->curr_stat.rx_bytes += rx_byte;
    
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
    e->curr_stat.tx_packets += tx_packet;
    e->curr_stat.tx_bytes += tx_byte;
}

static void show_throuthput( wlan_ip_statistics_t*  stats, void *handle, dump_fn fn)
{
	tatistics_t* ps = &stats->prev_stat;
	u64 diff_pkts_rx, diff_pkts_tx, diff_bytes_rx, diff_bytes_tx,
				diff_ns;
	u64 mpps_rx, mpps_tx, mbps_rx, mbps_tx;
	struct timespec cur_time;
	diff_ns = 0;
	if (clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time) == 0) {
		u64 ns;
		ns = cur_time.tv_sec * NS_PER_SEC;
		ns += cur_time.tv_nsec;
		if (ps->time_ns != 0)
			diff_ns = ns - ps->time_ns;
		ps->time_ns = ns;
	}

	diff_pkts_rx = (stats->curr_stat.rx_packets > ps->rx_packets) ?
		(stats->curr_stat.rx_packets - ps->rx_packets) : 0;
	diff_pkts_tx = (stats->curr_stat.tx_packets > ps->tx_packets) ?
		(stats->curr_stat.tx_packets - ps->tx_packets) : 0;
	ps->rx_packets = stats->curr_stat.rx_packets;
	ps->rx_packets = stats->curr_stat.tx_packets;
	mpps_rx = diff_ns > 0 ?
		(double)diff_pkts_rx / diff_ns * NS_PER_SEC : 0;
	mpps_tx = diff_ns > 0 ?
		(double)diff_pkts_tx / diff_ns * NS_PER_SEC : 0;

	diff_bytes_rx = (stats->curr_stat.rx_bytes > ps->rx_bytes) ?
		(stats->curr_stat.rx_bytes - ps->rx_bytes) : 0;
	diff_bytes_tx = (stats->curr_stat.tx_bytes > ps->tx_bytes) ?
		(stats->curr_stat.tx_bytes - ps->tx_bytes) : 0;
	ps->rx_bytes = stats->curr_stat.rx_bytes;
	ps->tx_bytes = stats->curr_stat.tx_bytes;
	mbps_rx = diff_ns > 0 ?
		(double)diff_bytes_rx / diff_ns * NS_PER_SEC : 0;
	mbps_rx *= 8;
	mbps_tx = diff_ns > 0 ?
		(double)diff_bytes_tx / diff_ns * NS_PER_SEC : 0;
	mbps_tx *= 8;
#define RATE_1K 1000
#define RATE_1M 1000000
#define RATE_1G 1000000000
    char str_buf[512] = {0};
    int len = 0;
	if (mbps_rx <= RATE_1K)
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tRx-pps: %9lld  Rx-bps: %9lld bps\n", mpps_rx, mbps_rx);
	else if (mbps_rx <= RATE_1M)
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tRx-pps: %9lld  Rx-bps: %.3f Kbps\n", mpps_rx, ((float)mbps_rx) / RATE_1K);
	else if (mbps_rx <= RATE_1G)
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tRx-pps: %9lld  Rx-bps: %.3f Mbps\n", mpps_rx, ((float)mbps_rx) / RATE_1M);
	else
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tRx-pps: %9lld  Rx-bps: %.3f Gbps\n", mpps_rx, ((float)mbps_rx) / RATE_1G);
	if (mbps_tx <= RATE_1K)
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tTx-pps: %9lld  Tx-bps: %9lld bps\n", mpps_tx, mbps_tx);
	else if (mbps_tx <= RATE_1M)
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tTx-pps: %9lld  Tx-bps: %.3f Kbps\n", mpps_tx, ((float)mbps_tx) / RATE_1K);
	else if (mbps_tx <= RATE_1G)
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tTx-pps: %9lld  Tx-bps: %.3f Mbps\n", mpps_tx, ((float)mbps_tx) / RATE_1M);
	else
		len += snprintf(&str_buf[len], sizeof(str_buf) - len, "\tTx-pps: %9lld  Tx-bps: %.3f Gbps\n", mpps_tx, ((float)mbps_tx) / RATE_1G);
    fn(handle, str_buf);
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
                 inet_ntoa(a), ce->curr_stat.rx_packets, ce->curr_stat.tx_packets, ce->curr_stat.rx_bytes, ce->curr_stat.tx_bytes);
        fn(handle, str_buf);
        show_throuthput(ce, handle, fn);
    }
}