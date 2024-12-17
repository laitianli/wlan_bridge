#include "common.h"
#include "wlan_br_forward.h"
#include "udp.h"
static int epfd = -1;
static int udp_sock_fd = -1;
static int rx_task_is_running = 1;
#define UDP_PORT 29870

static void usr1_handler(int signo)
{
    rx_task_is_running = 0;
}

static void server_runing(void)
{
    int nfds = -1;
    int i = 0;
    struct epoll_event evs[20];
    unsigned char udp_buf[1518] = {0};
    int recv_len = 0;
    int ev_size = sizeof(evs) / sizeof(struct epoll_event);

    struct sockaddr_in peer_addr;

    while (rx_task_is_running)
    {
        nfds = epoll_wait(epfd, evs, ev_size, 1000);
        for (i = 0; i < nfds; i++)
        {
            if (evs[i].events & EPOLLIN)
            {
                if (evs[i].data.fd == udp_sock_fd)
                {
                    while (1)
                    {
                        recv_len = read_data_from_udp_client(sizeof(udp_buf), udp_buf, &peer_addr);
                        if (recv_len <= 0)
                        {
                            if (recv_len == -2)
                            {
                                // LOG_INFO("read data over, so goto epoll_wait!");
                                break;
                            }
                            LOG_ERR("read udp data len error!");
                            break;
                        }

                        wlan_br_handle(recv_len, udp_buf, &peer_addr);
                    }
                }
            }
            else
            {
                LOG_INFO("recv unknown event: %d", evs[i].events);
            }
        }
    }
    LOG_INFO("server running exit!");
}
#define MAX_DATA_LEN 1518
#define MAX_NUM 32
static void server_runing_burst(void)
{
    int nfds = -1;
    int i = 0;
    struct epoll_event evs[20];

    int recv_len = 0;
    int ev_size = sizeof(evs) / sizeof(struct epoll_event);
    unsigned char *udp_buf[MAX_NUM] = {0};
    unsigned int data_len[MAX_NUM] = {0};
    struct sockaddr_in peer_addr[MAX_NUM] = {0};
    for (i = 0; i < MAX_NUM; i++)
    {
        udp_buf[i] = malloc(MAX_DATA_LEN);
        if (!udp_buf[i])
        {
            LOG_ERR("malloc memory failed! so exit!");
            return;
        }
    }
    while (rx_task_is_running)
    {
        nfds = epoll_wait(epfd, evs, ev_size, 1000);
        for (i = 0; i < nfds; i++)
        {
            if (evs[i].events & EPOLLIN)
            {
                if (evs[i].data.fd == udp_sock_fd)
                {
                    while (1)
                    {
                        recv_len = read_data_from_udp_client_burst(MAX_DATA_LEN, udp_buf, data_len, peer_addr, MAX_NUM);
                        if (recv_len <= 0)
                        {
                            if (recv_len == -2)
                            {
                                //LOG_INFO("read data over, so goto epoll_wait!");
                                break;
                            }
                            LOG_ERR("read udp data len error!");
                            break;
                        }

                        wlan_br_handle_burst(recv_len, udp_buf, data_len, peer_addr);
                    }
                }
            }
            else
            {
                LOG_INFO("recv unknown event: %d", evs[i].events);
            }
        }
    }
    LOG_INFO("server running exit!");
    for (i = 0; i < MAX_NUM; i++)
    {
        if (udp_buf[i])
        {
            free(udp_buf[i]);
        }
    }
}

int main(int argv, char **argc)
{
    signal(SIGUSR1, usr1_handler);
    udp_sock_fd = create_udp_server(UDP_PORT);
    if (udp_sock_fd == -1)
    {
        LOG_ERR("create_udp_client failed!");
        return -1;
    }
    int flags = fcntl(udp_sock_fd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(udp_sock_fd, F_SETFL, flags);

    struct epoll_event ev;
    epfd = epoll_create(1);

    ev.data.fd = udp_sock_fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, udp_sock_fd, &ev);

    // server_runing();
    server_runing_burst();

    destroy_udp_server();
    return 0;
}