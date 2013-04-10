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
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
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

struct link *link_tbl_head = NULL;
struct link *link_tbl_tail = NULL;
pthread_mutex_t link_tbl_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *str_link_type[LINK_TYPE_NUM] =
	{"Ethernet", "Wi-Fi", "WiMAX", "LTE"};

static inline const char * lt_to_str(int type)
{
	if (type < 0 || type >= LINK_TYPE_NUM)
		return "INVALID";
	else
		return str_link_type[type];
}

static inline int str_to_lt(const char *name)
{
	int i;
	for (i = 0; i < LINK_TYPE_NUM; ++i)
		if (!strcmp(name, str_link_type[i]))
			return i;
	return -1;
}

/* not thread safe, must be called while holding link_tbl_mutex */
static inline void dump_one_link(struct link *lnk)
{
	log_info("    Link: %8s %s", lt_to_str(lnk->type), lnk->name);
}

/* TODO: to string */
void netmgr_dump_links() {
	struct link *lnk;

	pthread_mutex_lock(&link_tbl_mutex);

	log_info("------------------------");
	log_info("Printing Link Info");
	log_info("------------------------");
	log_info("Link:TYPE      NAME");
	for (lnk = link_tbl_head->next; lnk != NULL; lnk = lnk->next)
		dump_one_link(lnk);
	log_info("------------------------");

	pthread_mutex_unlock(&link_tbl_mutex);
}

static int init_link_table()
{
	pthread_mutex_lock(&link_tbl_mutex);
	if(!link_tbl_head) {
		link_tbl_head = malloc(sizeof(struct link)); /* head */
		memset(link_tbl_head, 0, sizeof(struct link));
		check(link_tbl_head, "Failed to allocate space for link table (%s)", strerror(errno));
		link_tbl_head->next = NULL;
		link_tbl_tail = link_tbl_head;
	}
	pthread_mutex_unlock(&link_tbl_mutex);
	return 0;

error:
	pthread_mutex_unlock(&link_tbl_mutex);
	return -1;
} 

static inline void free_link(struct link *lnk) {
	if (lnk) {
		if (lnk->name)
			free(lnk->name);
		free(lnk);
	}
}

static int add_link(const char *type, const char *name)
{
	struct link *lnk = NULL;

	lnk = malloc(sizeof(struct link));
	check(lnk, "malloc for link failed");
	memset(lnk, 0, sizeof(struct link));

	lnk->type = str_to_lt(type);

	lnk->name = malloc(strlen(name));
	check(lnk->name, "malloc for link->name failed");
	strcpy(lnk->name, name);
	dump_one_link(lnk);

	return 0;

error:
	free_link(lnk);
	return -1;
} 

/* TODO: for now, lnk->name is unique */
static void remove_link(const char *name) {
	struct link *pred_lnk;
	struct link *curr_lnk;

	pthread_mutex_lock(&link_tbl_mutex);
	pred_lnk = link_tbl_head;
	curr_lnk = pred_lnk->next;

	while (curr_lnk != NULL) {
		if (!strcmp(curr_lnk->name, name)) {
			pred_lnk->next = curr_lnk->next;
			free_link(curr_lnk);
			curr_lnk = pred_lnk->next;
			/* update tail when the last link is removed */
			if (curr_lnk == NULL) {
				link_tbl_tail = pred_lnk;
				break;
			}
		} else {
			pred_lnk = curr_lnk;
			curr_lnk = pred_lnk->next;
		}
	} 	

	pthread_mutex_unlock(&link_tbl_mutex);
}

/* only called by pe during evaluation, don't need locks */
static void netmgr_send_info(char *buf)
{
	struct sockaddr_in from_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int r;

	if (info_sock < 0) {
		struct timeval tv;
		info_sock = socket(AF_INET, SOCK_DGRAM, 0);
		check(info_sock >= 0, "new info_sock");

		tv.tv_sec = SINE_TIMEOUT;
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
	sprintf(buf, "e %s", ifname);	// TODO: request
	netmgr_send_info(buf);

	if (!strcmp(buf, "up"))		// TODO: response
		return 1;

	return 0;
}

static void free_link_table()
{
	struct link *curr_lnk;

	if (!link_tbl_head) return;

	pthread_mutex_lock(&link_tbl_mutex);
	while (link_tbl_head->next != NULL) {
		curr_lnk = link_tbl_head->next;
		link_tbl_head->next = curr_lnk->next;
		free_link(curr_lnk);
	}
	free_link(link_tbl_head);
	link_tbl_head = NULL;
	link_tbl_tail = NULL;
	pthread_mutex_unlock(&link_tbl_mutex);
}

int netmgr_link_up(const char* name)
{
	struct link *curr_lnk = NULL;

	pthread_mutex_lock(&link_tbl_mutex);
	curr_lnk = link_tbl_head->next;
	while (curr_lnk != NULL && strcmp(curr_lnk->name, name))
		curr_lnk = curr_lnk->next;
	pthread_mutex_unlock(&link_tbl_mutex);

	return (curr_lnk != NULL);
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

/* this works, but not recommended
static void netmgr_sig_handler(int signum)
{
	debug("signal %d received", signum);
}
*/

static void * netmgr_main_loop()
{
	struct sockaddr_in from_addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	fd_set netmgr_fds;
	char buf[IPC_BUF_SIZE];
	int max_fd;
	int r;

	/* this works, but not recommended
	struct sigaction sa;
	sa.sa_handler = netmgr_sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP;
	check(!sigaction(SIGUSR1, &sa, NULL), "set handler for SIGUSR1");
	*/

	max_fd = MAX(event_sock, exit_pipe[0]);

	while (!exit_flag) {
		FD_ZERO(&netmgr_fds);
		FD_SET(event_sock, &netmgr_fds);
		FD_SET(exit_pipe[0], &netmgr_fds);

		r = select(max_fd + 1, &netmgr_fds, NULL, NULL, NULL);
		check(r >= 0, "Select exception");
		debug("select returns %d", r);

		if (FD_ISSET(exit_pipe[0], &netmgr_fds))
			break;

		if (FD_ISSET(event_sock, &netmgr_fds)) {
			debug("getting response from MIH user");
			addrlen = sizeof(struct sockaddr_in);
			r = recvfrom(event_sock, buf, IPC_BUF_SIZE, 0,
				(struct sockaddr *)&from_addr, &addrlen);
			check(r > 0, "get event from MIH user");

			debug("event: %s", buf);
			trigger_policy_engine();
		}
	}

error:
	free_link_table();

	if (info_sock >= 0) {
		close(info_sock);
		info_sock = -1;
	}
	if (event_sock >= 0) {
		close(event_sock);
		event_sock = -1;
	}

	log_info("network manager exiting...");

	return NULL;
}

int init_network_manager(pthread_t *netmgr_thread_ptr)
{
	struct hostent *he;
	struct sockaddr_in from_addr;
	struct timeval tv;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	char buf[IPC_BUF_SIZE];

	check(!init_link_table(), "Initializing link table");

	inet_pton(AF_INET, HIP_ADDR_V4, &hip_addr_v4);
	inet_pton(AF_INET, HIP_MASK_V4, &hip_mask_v4);
	inet_pton(AF_INET6, HIP_ADDR_V6, &hip_addr_v6);
	inet_pton(AF_INET6, HIP_MASK_V6, &hip_mask_v6);

	he = gethostbyname("localhost");
	check(he, "get addr by localhost");

	memcpy(&netmgr_addr.sin_addr, he->h_addr_list[0], he->h_length);
	netmgr_addr.sin_family = AF_INET;
	netmgr_addr.sin_port = htons(NETMGR_PORT);

	event_sock = socket(AF_INET, SOCK_DGRAM, 0);
	check(event_sock >= 0, "new event_sock");

	tv.tv_sec = SINE_TIMEOUT;
	tv.tv_usec = 0;
	check(!setsockopt(event_sock, SOL_SOCKET, SO_RCVTIMEO,
				(char *)&tv, sizeof(struct timeval)),
		"set recv timeout for event_sock");

	log_info("Try to register to MIH user...");
	/* TODO: where to de-register? Also, command of registration */
	check(sendto(event_sock, "r", 2, 0, (struct sockaddr *)&netmgr_addr,
		sizeof(struct sockaddr_in)) > 0, "register to MIH user");
	/* TODO: first response message */
	check(recvfrom(event_sock, buf, IPC_BUF_SIZE, 0,
		(struct sockaddr *)&from_addr, &addrlen) > 0, "MIH response");

	/* set event_sock back to no timeout */
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	check(!setsockopt(event_sock, SOL_SOCKET, SO_RCVTIMEO,
				(char *)&tv, sizeof(struct timeval)),
		"clear recv timeout for event_sock");

	check(!pthread_create(netmgr_thread_ptr, 0, netmgr_main_loop, 0),
		"Failed to create thread for network manager");

	debug("Network Manager initialized");
	return 0;

error:
	free_link_table();
	if (event_sock >= 0)
		close(event_sock);

	return -1;
}
