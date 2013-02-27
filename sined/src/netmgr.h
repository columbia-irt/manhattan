#ifndef _SINE_NETMGR_H_
#define _SINE_NETMGR_H_

#include "sined.h"

#define HIP_ADDR_V4	"1.0.0.0"
#define HIP_MASK_V4	"255.0.0.0"
#define HIP_ADDR_V6	"2001:10::"
#define HIP_MASK_V6	"ffff:fff0::"

void * main_network_manager(void *arg);

/*
 * Network Manager API
 * @author: Yan Zou
 * @date: Apr 16, 2012
 */

//
int netmgr_interface_on(const char* ifname);

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

#endif
