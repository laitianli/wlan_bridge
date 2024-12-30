#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "common.h"
#define BUF_SIZE 2000
static int udp_sock = -1;
static struct sockaddr_in serv_adr;
int create_udp_client(unsigned short port, const char *server_ip)
{
	int str_len;
	socklen_t adr_sz;
	udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (udp_sock == -1)
	{
		LOG_ERR("socket() error, errno: %d", errno);
		return -1;
	}
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(server_ip);
	serv_adr.sin_port = htons(port);
	LOG_INFO("udp port: %u, udp server ip: %s", port, server_ip);
	return udp_sock;
}

int send_data_use_udp(int len, unsigned char *buf)
{
	socklen_t socklen = sizeof(struct sockaddr_in);
	int sendlen = sendto(udp_sock, buf, len, 0, (struct sockaddr *)&serv_adr, socklen);
	if (sendlen != len)
	{
		LOG_ERR("len: %d, sendlen: %d, errno: %d", len, sendlen, errno);
		return sendlen;
	}
	// LOG_DBG("sendlen:%d", sendlen);
	return sendlen;
}
int send_data_use_udp_burst(unsigned char *buf[], unsigned int data_len[], int msg_num)
{
	struct mmsghdr mmsg[msg_num];
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
		mmsg[i].msg_hdr.msg_name = (void *)&serv_adr;
		mmsg[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
	}
	int retval = sendmmsg(udp_sock, mmsg, msg_num, 0);
	if (retval != msg_num)
	{
		LOG_ERR("msg_num: %d, sendcount: %d, errno: %d", msg_num, retval, errno);
		return -1;
	}
	// LOG_INFO("sendmmsg msg num: %d", msg_num);
	return retval;
}
int read_data_from_udp(int max_len, unsigned char *buf)
{
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz = sizeof(clnt_adr);
	int recv_len = recvfrom(udp_sock, buf, max_len, 0, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
	if (recv_len <= 0)
	{
		LOG_ERR("read udp data len error!");
		return -1;
	}
	return recv_len;
}

int read_data_from_udp_burst(int max_len, unsigned char *buf[], unsigned int data_len[], int max_num)
{
	struct timespec timeout;
	struct sockaddr_in clnt_adr[max_num];
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
		mmsg_arr[i].msg_hdr.msg_name = (void *)&clnt_adr[i];
		mmsg_arr[i].msg_hdr.msg_namelen = sizeof(struct sockaddr_in);
	}
	int recv_len = recvmmsg(udp_sock, mmsg_arr, max_num, 0, &timeout);
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

void destroy_udp_client(void)
{
	memset(&serv_adr, 0, sizeof(serv_adr));
	if (udp_sock != -1)
	{
		close(udp_sock);
		udp_sock = -1;
	}
}
