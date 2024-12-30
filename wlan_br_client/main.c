#include "common.h"
#include "cli.h"
static int epfd = -1;
static int tuntap_fd = -1;
static int udp_sock_fd = -1;
static int rx_task_is_running = 1;
epoll_stat_info_t epoll_stat_info = {0};
#define UDP_PORT 29870
#define SERVER_IP "172.16.123.137"
#define TAP_NAME "ttap"

int log_level = 0;
static char server_addr[20] = {0};
static char tap_name[16] = {0};

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"server", required_argument, 0, 's'},
    {"tap", required_argument, 0, 't'},
    {NULL, 0, 0, 0}
};

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
        if (nfds == 0) {
            epoll_stat_info.epoll_timeout ++;
            continue;
        }
        else if (unlikely(nfds < 0)) {
            epoll_stat_info.epoll_error ++;
            continue;
        }
        else {
            epoll_stat_info.epool_recv += nfds;
        }
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
    }/* code */
    LOG_INFO("run exit!");
}

void usage(void)
{
    exit(0);
}

int main(int argv, char **argc)
{
    signal(SIGUSR1, usr1_handler);
    int c;
    int option_index = 0;
    while ((c = getopt_long(argv, argc, "hs:t:", long_options, &option_index)) != -1) {
        switch (c)
        {
        case 'h':
            usage();
            break;
        case 's':
            strcpy(server_addr, optarg);
            break;
        case 't':
            strcpy(tap_name, optarg);
            break;
        default:
            break;
        }
    }
    if (strlen(server_addr) == 0) {
        strcpy(server_addr, SERVER_IP);
    }
    if (strlen(tap_name) == 0) {
        strcpy(tap_name, TAP_NAME);
    }

    cli_main();
    tuntap_fd = create_tuntap(tap_name);
    if (tuntap_fd == -1)
    {
        LOG_ERR("create_tuntap failed!");
        return -1;
    }
    udp_sock_fd = create_udp_client(UDP_PORT, server_addr);
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