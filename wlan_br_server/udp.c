#define _GNU_SOURCE
#include <sys/socket.h>
#include "common.h"
static int serv_sock = -1;
int create_udp_server(unsigned short port)
{
    struct sockaddr_in serv_adr;

    serv_sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (serv_sock == -1)
    {
        LOG_ERR("UDP socket creation error, errno: %d", errno);
        return -1;
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(port);
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        LOG_ERR("bind() error, errno: %d", errno);
        return -1;
    }
    int nRecvBuf = 1024 * 1024;
    int ret = setsockopt(serv_sock, SOL_SOCKET, SO_RCVBUF, (const char *)&nRecvBuf, sizeof(int));
    if (ret)
    {
        LOG_ERR("ret: %d, errno: %d", ret, errno);
    }

    int nSendBuf = 1024 * 1024;
    ret = setsockopt(serv_sock, SOL_SOCKET, SO_SNDBUF, (const char *)&nSendBuf, sizeof(int));
    if (ret)
    {
        LOG_ERR("ret: %d, errno: %d", ret, errno);
    }
    return serv_sock;
}

int send_data_to_udp_client(int len, unsigned char *buf, struct sockaddr_in *peer_addr)
{
    socklen_t socklen = sizeof(struct sockaddr_in);
    int sendlen = sendto(serv_sock, buf, len, 0, (struct sockaddr *)peer_addr, socklen);
    if (sendlen != len)
    {
        LOG_ERR("len: %d, sendlen: %d, errno: %d", len, sendlen, errno);
        return sendlen;
    }
    // LOG_DBG("sendlen:%d", sendlen);
    return sendlen;
}

int send_data_to_udp_client_burst(unsigned char *buf[], unsigned int data_len[], struct sockaddr_in peer_addr[], int msg_num)
{
    struct mmsghdr mmsg[msg_num];
    struct mmsghdr resend_mmsg[msg_num];
    struct iovec iov[msg_num];
    memset(mmsg, 0, sizeof(mmsg));
    memset(iov, 0, sizeof(iov));
    int i = 0;

    for (i = 0; i < msg_num; i++)
    {
        iov[i].iov_base = buf[i];
        iov[i].iov_len = data_len[i];
        mmsg[i].msg_hdr.msg_iov = &iov[i];
        mmsg[i].msg_hdr.msg_iovlen = 1;
        mmsg[i].msg_hdr.msg_name = (void *)&peer_addr[i];
        mmsg[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
    }
    int retval = sendmmsg(serv_sock, mmsg, msg_num, 0);
    if (retval != msg_num)
    {
        int resend_msg_num = 0;
        LOG_INFO("msg_num: %d, sendcount: %d, errno: %d", msg_num, retval, errno);
        if (errno == EAGAIN)
        {
            for (i = 0; i < msg_num; i++)
            {
                LOG_INFO("mmsg[%d].msg_len:%d, mmsg[%d].msg_hdr.msg_iov->iov_len:%lu", i, mmsg[i].msg_len, i, mmsg[i].msg_hdr.msg_iov->iov_len);
                if (mmsg[i].msg_len != mmsg[i].msg_hdr.msg_iov->iov_len)
                    memcpy(&resend_mmsg[resend_msg_num++], &mmsg[i], sizeof(struct mmsghdr));
            }
            if (resend_msg_num > 0)
            {
                LOG_INFO("so wait 20us to resend [%d] packet.", resend_msg_num);
                usleep(20);
                retval = sendmmsg(serv_sock, resend_mmsg, resend_msg_num, 0);
                if (retval == resend_msg_num)
                    goto END;
                else
                {
                    LOG_ERR("msg_num: %d, sendcount: %d, errno: %d", msg_num, retval, errno);
                    return -1;
                }
            }
        }
        return -1;
    }
END:
    // LOG_INFO("sendmmsg msg num: %d", msg_num);
    return retval;
}

int read_data_from_udp_client(int max_len, unsigned char *buf, struct sockaddr_in *peer_addr)
{
    if (!buf || !peer_addr)
    {
        LOG_ERR("argument error!");
        return -1;
    }
    socklen_t clnt_adr_sz = sizeof(struct sockaddr_in);
    int recv_len = recvfrom(serv_sock, buf, max_len, 0, (struct sockaddr *)peer_addr, &clnt_adr_sz);
    if (recv_len <= 0)
    {
        if (errno == EAGAIN)
        {
            return -2;
        }
        LOG_ERR("read udp data len error!");
        return -1;
    }
    return recv_len;
}

int read_data_from_udp_client_burst(int max_len, unsigned char *buf[], unsigned int data_len[], struct sockaddr_in peer_addr[], int max_num)
{
    struct timespec timeout;
    struct mmsghdr mmsg_arr[max_num];
    struct iovec iovecs_arry[max_num];
    memset(mmsg_arr, 0, sizeof(mmsg_arr));
    memset(iovecs_arry, 0, sizeof(iovecs_arry));
    timeout.tv_sec = 0;
    timeout.tv_nsec = 1 * 1000000;
    int i = 0;
    for (i = 0; i < max_num; i++)
    {
        iovecs_arry[i].iov_base = buf[i];
        iovecs_arry[i].iov_len = max_len;
        mmsg_arr[i].msg_hdr.msg_iov = &iovecs_arry[i];
        mmsg_arr[i].msg_hdr.msg_iovlen = 1;
        mmsg_arr[i].msg_hdr.msg_name = (void *)&peer_addr[i];
        mmsg_arr[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
    }
    int recv_len = recvmmsg(serv_sock, mmsg_arr, max_num, 0, &timeout);
    if (recv_len <= 0)
    {
        if (errno == EAGAIN)
        {
            return -2;
        }
        LOG_ERR("read udp data len error!");
        return -1;
    }
    for (i = 0; i < recv_len; i++)
    {
        data_len[i] = mmsg_arr[i].msg_len;
    }
    return recv_len;
}

void destroy_udp_server(void)
{
    if (serv_sock != -1)
    {
        close(serv_sock);
        serv_sock = -1;
    }
}
