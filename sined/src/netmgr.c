/*
 * Network Manager API
 * @author: Yan Zou
 * @date: Apr 16, 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "netmgr.h"
#include "pe.h"

static struct sockaddr_in netmgr_addr;

static struct in_addr hip_addr_v4;
static struct in_addr hip_mask_v4;

static struct in6_addr hip_addr_v6;
static struct in6_addr hip_mask_v6;

static int info_sock = -1;
static int event_sock = -1;

/* TODO: racing on netmgr_addr between netmgr thread and pe thread */
static int init_netmgr()
{
	struct hostent *he;

	debug("hipaddr4: %d", inet_pton(AF_INET, HIP_ADDR_V4, &hip_addr_v4));
	debug("hipmask4: %d", inet_pton(AF_INET, HIP_MASK_V4, &hip_mask_v4));
	debug("hipaddr6: %d", inet_pton(AF_INET6, HIP_ADDR_V6, &hip_addr_v6));
	debug("hipmask6: %d", inet_pton(AF_INET6, HIP_MASK_V6, &hip_mask_v6));

	he = gethostbyname("localhost");
	check(he, "get addr by localhost");

	memcpy(&netmgr_addr.sin_addr, he->h_addr_list[0], he->h_length);
	netmgr_addr.sin_family = AF_INET;
	netmgr_addr.sin_port = htons(18752);

	event_sock = socket(AF_INET, SOCK_DGRAM, 0);
	check(event_sock >= 0, "new event_sock");

	return 0;

error:
	return -1;
}

/* only called by pe during evaluation, no racing */
static void netmgr_send_info(char *buf)
{
	struct sockaddr_in from_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int r;

	if (info_sock < 0) {
		struct timeval tv;
		info_sock = socket(AF_INET, SOCK_DGRAM, 0);
		check(info_sock >= 0, "new info_sock");

		tv.tv_sec = NETMGR_TIMEOUT;
		tv.tv_usec = 0;
		check(!setsockopt(info_sock, SOL_SOCKET, SO_RCVTIMEO,
			(char *)&tv, sizeof(struct timeval)),
				"set info_sock timeout");
	}

	debug("Send to MIH user: %s", buf);
	check(sendto(info_sock, buf, strlen(buf) + 1, 0,
		(struct sockaddr *)&netmgr_addr, addrlen) >= 0,
			"sending to MIH user");

	r = recvfrom(info_sock, buf, IPC_BUF_SIZE, 0,
		(struct sockaddr *)&from_addr, &addrlen);
	check(r >= 0, "No response from MIH user");
	buf[r] = '\0';
	debug("Response from MIH user: %s", buf);

	return;

error:
	buf[0] = '\0';

	if (info_sock > 0) {
		close(info_sock);
		info_sock = -1;
	}
}

/* 0 -- off, 1 -- on, -1 -- error */
int netmgr_interface_on(const char* ifname)
{
	char buf[IPC_BUF_SIZE];

	debug("Query MIH if interface %s exists.", ifname);
	sprintf(buf, "e %s", ifname);	/* TODO: request */
	netmgr_send_info(buf);

	if (!strcmp(buf, "up"))		/* TODO: response */
		return 1;

	return 0;
}

/*
int sine_getCost(const char* ifname)
{
        char buf[20];
        sprintf(buf, "c %s", ifname);
        if (ipc_netmgr(buf, -1) == NULL || buf[0] == '\0')
	{
		return INT_MAX;
	}
	//printf("%d\n", (int)buf[0]);
	return atoi(buf);
}

int sine_getBandwidth(const char* ifname)
{
        char buf[20];
	printf("\nQuery the network information (cost & bandwidth) from MIH.\n");
        sprintf(buf, "b %s", ifname);
        if (ipc_netmgr(buf, -1) == NULL || buf[0] == '\0')
	{
		return -1;
	}
	return atoi(buf);
}

char* sine_getInfo(const char* ifname)
{
        char buf[20];	//TODO: get rid of malloc
        sprintf(buf, "i %s", ifname);
        return ipc_netmgr(buf, -1);
}
*/

/* return: 0 - not hip address, 1 - is hip address */
static inline int is_hip_addr(const struct sockaddr *addr)
{
	int result;

	if (addr->sa_family == AF_INET) {	/* same subnet */
		struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
		debug("%u %u %u", addr4->sin_addr.s_addr,
			hip_mask_v4.s_addr, hip_addr_v4.s_addr);
		result = ((addr4->sin_addr.s_addr & hip_mask_v4.s_addr)
				== hip_addr_v4.s_addr);
	} else if (addr->sa_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
		int i;
		result = 1;
		for (i = 0; i < 16; ++i) {	/* check all bytes */
			//debug("%d %d %d %d", i, addr6->sin6_addr.s6_addr[i],
			//    hip_mask_v6.s6_addr[i], hip_addr_v6.s6_addr[i]);
			if ((addr6->sin6_addr.s6_addr[i]
					& hip_mask_v6.s6_addr[i])
				!= hip_addr_v6.s6_addr[i]) {
				result = 0;
				break;
			}
		}
	} else {
		result = 0;
	}

	return result;
}

int netmgr_set_interface(const struct sockaddr *addr, const char* ifname)
{
	char buf[IPC_BUF_SIZE];

	if (is_hip_addr(addr)) {
		debug("Tell HIP to use interface %s.", ifname);

		sprintf(buf, "h %s", ifname);
		netmgr_send_info(buf);
		check(buf[0], "No response from HIP");

		sprintf(buf, "g %s", ifname);
		netmgr_send_info(buf);
		check(buf[0], "No response from HIP");
	}
	else if (addr->sa_family == AF_INET6) {	/* plain IPv6 */
		debug("Tell MIPv6 to use interface %s.", ifname);

		sprintf(buf, "m %s", ifname);
		netmgr_send_info(buf);
		check(buf[0], "No response from MIPv6");
	}
	else {	/* TODO: plain IPv4, do nothing */
		log_info("Do nothing to plain IPv4");
	}

	return 0;

error:
	return -1;
}

void * main_network_manager(void *arg)
{
	struct sockaddr_in from_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	char buf[IPC_BUF_SIZE];
	int r;

	check(!init_netmgr(), "Failed to initialize Network Manager");

	/* TODO: where to de-register? Also, command of registration */
	check(sendto(event_sock, "r", 2, 0, (struct sockaddr *)&netmgr_addr,
		sizeof(struct sockaddr_in)) >= 0, "sending to MIH user");
	/* TODO: first response message */
	check(recvfrom(event_sock, buf, IPC_BUF_SIZE, 0,
		(struct sockaddr *)&from_addr, &addrlen), "MIH response");

	debug("Network Manager initialized");

	while (1) {
		addrlen = sizeof(struct sockaddr_in);
		r = recvfrom(event_sock, buf, IPC_BUF_SIZE, 0,
			(struct sockaddr *)&from_addr, &addrlen);
		check(r >= 0, "get event from MIH user");

		debug("event: %s", buf);
		trigger_policy_engine();
	}

error:
	if (info_sock >= 0) {
		close(info_sock);
		info_sock = -1;
	}
	if (event_sock >= 0) {
		close(event_sock);
		event_sock = -1;
	}

	return NULL;
}
