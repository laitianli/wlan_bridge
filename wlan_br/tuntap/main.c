#include "common.h"
static int epfd = -1;
static int tuntap_fd = -1;
static int udp_sock_fd = -1;
static int rx_task_is_running = 1;
#define UDP_PORT 29870
#define SERVER_IP "172.16.121.125"
// #define SERVER_IP "172.16.123.134"

static void usr1_handler(int signo)
{
    rx_task_is_running = 0;
}

static void runing(void)
{
    int nfds = -1;
    int i = 0;
    struct epoll_event evs[20];
    unsigned char tuntap_buf[1518] = {0};
    unsigned char udp_buf[1518] = {0};
    int len = 0;
    int send_len = 0;
    int recv_len = 0;
    struct ethhdr *ether;

    int ev_size = sizeof(evs) / sizeof(struct epoll_event);
    while (rx_task_is_running)
    {
        nfds = epoll_wait(epfd, evs, ev_size, 2000);
        for (i = 0; i < nfds; i++)
        {
            if (evs[i].events & EPOLLIN)
            {
                if (evs[i].data.fd == tuntap_fd)
                {
                    len = read_data_from_tuntap(sizeof(tuntap_buf), tuntap_buf);
                    if (len <= 0)
                    {
                        LOG_ERR("read tuntap data len error!");
                        continue;
                    }
                    ether = (struct ethhdr *)tuntap_buf;
                    LOG_ETHER_ADDR(ether, "tuntap read len: %d", len);
                    send_len = send_data_use_udp(len, tuntap_buf);
                    if (send_len != len)
                    {
                        LOG_ERR("send_data_use_udp error, len: %d, send_len: %d", len, send_len);
                        continue;
                    }
                }
                else if (evs[i].data.fd == udp_sock_fd)
                {
                    recv_len = read_data_from_udp(sizeof(udp_buf), udp_buf);
                    if (recv_len <= 0)
                    {
                        LOG_ERR("read udp data len error!");
                        continue;
                    }
                    ether = (struct ethhdr *)udp_buf;
                    LOG_ETHER_ADDR(ether, "udp read len: %d", recv_len);
                    send_len = send_data_use_tuntap(recv_len, udp_buf);
                    if (send_len != recv_len)
                    {
                        LOG_ERR("send_data_use_tuntap error, recv_len: %d, send_len: %d", recv_len, send_len);
                        continue;
                    }
                }
            }
            else
            {
                LOG_INFO("recv unknown event: %d", evs[i].events);
            }
        }
    }
    LOG_INFO("run exit!");
}

int main(int argv, char **argc)
{
    signal(SIGUSR1, usr1_handler);
    tuntap_fd = create_tuntap("ttap");
    if (tuntap_fd == -1)
    {
        LOG_ERR("create_tuntap failed!");
        return -1;
    }
    udp_sock_fd = create_udp_client(UDP_PORT, SERVER_IP);
    if (udp_sock_fd == -1)
    {
        LOG_ERR("create_udp_client failed!");
        return -1;
    }
    struct epoll_event ev;
    epfd = epoll_create(1);
    ev.data.fd = tuntap_fd;
    ev.events = EPOLLIN /*| EPOLLET*/;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tuntap_fd, &ev);

    ev.data.fd = udp_sock_fd;
    ev.events = EPOLLIN /*| EPOLLET*/;
    epoll_ctl(epfd, EPOLL_CTL_ADD, udp_sock_fd, &ev);

    runing();

    destroy_tuntap();
    destroy_udp_client();
    return 0;
}