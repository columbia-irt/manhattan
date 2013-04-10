#ifndef _SINE_NETMGR_H_
#define _SINE_NETMGR_H_

#include <sys/socket.h>

#include "sined.h"

#define HIP_ADDR_V4	"1.0.0.0"
#define HIP_MASK_V4	"255.0.0.0"
#define HIP_ADDR_V6	"2001:10::"
#define HIP_MASK_V6	"ffff:fff0::"

/* TODO: port number to communicate with MIH_USR */
#define NETMGR_PORT	18752

#define LINK_TYPE_ETHERNET	0
#define LINK_TYPE_WIFI		1
#define LINK_TYPE_WIMAX		2
#define LINK_TYPE_LTE		3
#define LINK_TYPE_NUM		4

struct link
{
	char *name;
	int type;
	struct link *next;
};

int init_network_manager(pthread_t *netmgr_thread_ptr);

/*
 * Network Manager API
 * @author: Yan Zou
 * @date: Apr 16, 2012
 */

//
int netmgr_interface_on(const char* ifname);
int netmgr_link_up(const char* name);

/*
int netmgr_get_cost(const char* ifname);

//
int netmgr_get_bandwidth(const char* ifname);

//
char* netmgr_get_info(const char* ifname);
*/

//
int netmgr_set_interface(const struct sockaddr *addr, const char* ifname);

//
//int netmgr_set_hip(const char* ifname);
//int netmgr_set_mipv6(const char* ifname);

/*
//
int sine_appRegister(int sockfd);

//
int sine_appDeregister(int sockfd);
*/

void netmgr_dump_links();

#endif
