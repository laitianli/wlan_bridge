#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"

static int tun_fd = -1;
static int rx_task_is_running = 1;
static int exit_threads = 0;
#if 0
static void usr1_handler(int signo)
{
    exit_threads = 1;
	rx_task_is_running = 0;
}
#endif

static int tap_alloc(char *dev, int flags)
{

	struct ifreq ifr;
	int fd, err;
	const char *clonedev = "/dev/net/tun";

	if ((fd = open(clonedev, O_RDWR)) < 0)
	{
		LOG_ERR("open failed");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = flags;
	if (*dev)
	{
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
	{
		LOG_ERR("ioctl failed");
		close(fd);
		return -1;
	}

	strcpy(dev, ifr.ifr_name);

	ioctl(fd, TUNSETNOCSUM, 1);

	ioctl(fd, TUNSETDEBUG, 1);
	return fd;
}

static inline void generate_random_mac(unsigned char *mac)
{
	for (int i = 0; i < 6; ++i)
	{
		mac[i] = rand() % 256;
	}
	mac[0] = (mac[0] & 0xFE) | 0x02;
}

int setup_tuntap_device(const char *nic_name, int port)
{
	int fd;
	struct ifreq ifr;
	int gen_fd;
	char fp_name[IFNAMSIZ];
	struct sockaddr hwaddr;

	memset(&hwaddr, 0x0, sizeof(hwaddr));

	snprintf(fp_name, IFNAMSIZ, "%s%d", nic_name, port);
	fp_name[IFNAMSIZ - 1] = 0;

	fd = tap_alloc(fp_name, IFF_TAP | IFF_NO_PI);
	if (fd < 0)
	{
		LOG_ERR("tap_alloc failed");
		return -1;
	}
	srand((unsigned)time(NULL));
	unsigned char mac[6] = {0x00, 0xab, 0x11, 0x12, 0x13, 0x14};
	generate_random_mac(mac);
	printf("Random MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	hwaddr.sa_family = AF_UNIX;
	memcpy(hwaddr.sa_data, mac, sizeof(mac));

	memset(&ifr, 0x0, sizeof(ifr));
	strncpy(ifr.ifr_name, fp_name, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	memcpy(&ifr.ifr_hwaddr, &hwaddr, sizeof(ifr.ifr_hwaddr));

	if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0)
	{
		LOG_ERR("Failed to set MAC address: %s", strerror(errno));
		close(fd);
		return -1;
	}

	ioctl(fd, TUNSETPERSIST, 1);

	gen_fd = socket(PF_INET, SOCK_DGRAM, 0);

	memset(&ifr, 0x0, sizeof(ifr));
	strncpy(ifr.ifr_name, fp_name, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	ifr.ifr_mtu = 1500;
	LOG_DBG("Fastpath device %s MTU %i", fp_name, ifr.ifr_mtu);

	if (ioctl(gen_fd, SIOCSIFMTU, &ifr) < 0)
	{
		LOG_ERR("Failed to set MTU: %s", strerror(errno));
		close(gen_fd);
		close(fd);
		return -1;
	}

	memset(&ifr, 0x0, sizeof(ifr));
	strncpy(ifr.ifr_name, fp_name, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	if (ioctl(gen_fd, SIOCGIFFLAGS, &ifr) < 0)
	{
		LOG_ERR("Failed to get interface flags: %s", strerror(errno));
		close(gen_fd);
		close(fd);
		return -1;
	}

	if (!(ifr.ifr_flags & IFF_UP))
	{
		ifr.ifr_flags |= IFF_UP | IFF_NOARP;
		if (ioctl(gen_fd, SIOCSIFFLAGS, &ifr) < 0)
		{
			LOG_ERR("Failed to set interface flags: %s",
					strerror(errno));
			close(gen_fd);
			close(fd);
			return -1;
		}
	}

	memset(&ifr, 0x0, sizeof(ifr));
	strncpy(ifr.ifr_name, fp_name, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	if (ioctl(gen_fd, SIOCGIFINDEX, &ifr) < 0)
	{
		LOG_ERR("Failed to get interface index: %s", strerror(errno));
		close(gen_fd);
		close(fd);
		return -1;
	}
	close(gen_fd);
	LOG_DBG("fd: %d", fd);
	return fd;
}

int send_data_use_tuntap(int len, unsigned char *buf)
{
	int send_data = write(tun_fd, buf, len);
	if (send_data != len)
	{
		LOG_ERR("send_data: %d, len: %d, errno: %u", send_data, len, errno);
		return -1;
	}
	return send_data;
}

int send_data_use_tuntap_burst(int data_len[], unsigned char *buf[], int msg_mun)
{
	struct iovec iov[msg_mun];
	memset(iov, 0, sizeof(iov));
	int i = 0;
	for (i = 0; i < msg_mun; i++)
	{
		iov[i].iov_base = buf[i];
		iov[i].iov_len = data_len[i];
	}
	int send_data = writev(tun_fd, iov, msg_mun);
	if (send_data != msg_mun)
	{
		LOG_ERR("writev: %d, msg_mun: %d, errno: %u", send_data, msg_mun, errno);
		return -1;
	}
	return send_data;
}

int read_data_from_tuntap(int max_len, unsigned char *buf)
{
	int len = read(tun_fd, buf, max_len);
	if (len <= 0)
	{
		LOG_ERR("read tuntap data len error!");
		return -1;
	}
	return len;
}

int read_data_from_tuntap_burst(int max_len, int data_len[], unsigned char *buf[], int max_msg_mun)
{
	struct iovec iov[max_msg_mun];
	memset(iov, 0, sizeof(iov));
	int i = 0;
	for (i = 0; i < max_msg_mun; i++)
	{
		iov[i].iov_base = buf[i];
		iov[i].iov_len = max_len;
	}
	int len = readv(tun_fd, iov, max_msg_mun);
	if (len <= 0)
	{
		LOG_ERR("readv tuntap data len error! errno: %d", errno);
		return -1;
	}
	LOG_INFO("len: %d", len);
	for (i = 0; i < len; i++)
	{
		data_len[i] = iov[i].iov_len;
	}
	return len;
}

int create_tuntap(const char *nic_name)
{
	tun_fd = setup_tuntap_device(nic_name, 0);
	if (tun_fd <= 0)
	{
		LOG_ERR("setup_tuntap_device failed!");
		return -1;
	}
	return tun_fd;
}

void destroy_tuntap(void)
{
	rx_task_is_running = 0;
	if (tun_fd != -1)
	{
		close(tun_fd);
	}
}
