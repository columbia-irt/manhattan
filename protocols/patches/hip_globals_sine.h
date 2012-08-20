//yan-begin

#ifndef _HIP_GLOBALS_SINE_
#define _HIP_GLOBALS_SINE_

#define IP_VERSION 6
#if (IP_VERSION == 6)
#define DEST_ADDR "2001:468:904:16:221:86ff:fe52:7b3e"
//#define DEST_ADDR "2001:470:1f06:415::2"
#else
#define DEST_ADDR "128.59.16.0/21"
#endif

#define IPV6_ADDR_PREFERENCES   72

#define IPV6_PREFER_SRC_TMP             0x0001
#define IPV6_PREFER_SRC_PUBLIC          0x0002
#define IPV6_PREFER_SRC_COA             0x0004
#define IPV6_PREFER_SRC_HOME            0x0400

#include <pthread.h>

extern pthread_t pref_thread;// = -1;

extern char hip_preferred_ifname[20];// = {0};    //04/09/2012 - hard code max length of ifname
extern char hip_current_ifname[20];// = {0};      //05/06/2012 - change routing table in readdress
extern volatile int prefer_changed;// = 0;        //04/20/2012

extern pthread_mutex_t pref_mutex;// = PTHREAD_MUTEX_INITIALIZER;

extern volatile int encrypt_off;

void hip_sine_init();

void hip_sine_cleanup();

void hip_handoff(sockaddr_list *l);

void hip_route_change(int ip_version, const char *dest_addr,
		const char *old_ifname, const char *new_ifname);

void *hip_pref_listener(void *arg);

char* get_router_str(const char* ifname);

#endif

//yan-end
