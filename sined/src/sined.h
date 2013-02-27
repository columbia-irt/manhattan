#ifndef _SINED_H_
#define _SINED_H_

#define IPV6_ADDR_LEN	16

#define POLICY_PATH	"sine_policy.conf"
#define LOC_BUF_SIZE	16
#define IPC_BUF_SIZE	32
#define LINE_BUF_SIZE	256
#define PATH_BUF_SIZE	256
#define SOCK_BUF_SIZE	8192

#define SOCKS_IPV4
#define SOCKS_PORT	10808
#define SOCKS_LISTEN	64

#define CONMGR_NAME	"sine_connection_manager"
#define NETMGR_TIMEOUT	10

#define PROC_PATH	"/proc"
#define PROC_TCP6_PATH	"/proc/net/tcp6"
#define PROC_TCP_PATH	"/proc/net/tcp"
#define FDDIR_NAME	"fd"
#define EXEFILE_NAME	"exe"

#include <dbg.h>

#endif
