#ifndef _SINED_H_
#define _SINED_H_

#include <dbg.h>

#define IPV6_ADDR_LEN		16

#define CMD_MIHF		"odtone-mihf"
#define CMD_MIIS_SERVER		"miis_rdf_server"
#define CMD_LINK_SAP		"link_sap"
#define CMD_MIH_USR		"mih_usr"
#define CMD_MIH_USR_ARG		"--dest=mihf1"
#define CMD_HIP_DAEMON		"hip"
#define CMD_MIPV6_DAEMON	"mip6d"

#define CONF_ROOT		"/usr/local/etc/"
#define CONF_POLICY_PATH	CONF_ROOT"sine_policy.conf"
#define CONF_MIHF_PATH		CONF_ROOT"mih/odtone.conf"
#define CONF_MIIS_DATABASE	CONF_ROOT"mih/miis_database"
#define CONF_LINK_ETH_PATH	CONF_ROOT"mih/ethernet.conf"
#define CONF_LINK_WIFI_PATH	CONF_ROOT"mih/wifi.conf"
#define CONF_LINK_LTE_PATH	CONF_ROOT"mih/lte.conf"
#define CONF_MIH_USR_PATH	CONF_ROOT"mih/mih_usr.conf"
#define CONF_MIPV6_PATH		CONF_ROOT"mip6d.conf"

#define LOG_ROOT		"/var/log/"
#define LOG_MIHF_PATH		LOG_ROOT"mihf.log"
#define LOG_MIIS_SERVER_PATH	LOG_ROOT"miis_server.log"
#define LOG_LINK_ETH_PATH	LOG_ROOT"link_eth.log"
#define LOG_LINK_WIFI_PATH	LOG_ROOT"link_wifi.log"
#define LOG_LINK_LTE_PATH	LOG_ROOT"link_lte.log"
#define LOG_MIH_USR_PATH	LOG_ROOT"mih_usr.log"
#define LOG_HIP_PATH		LOG_ROOT"hip.log"
#define LOG_MIPV6_PATH		LOG_ROOT"mip6d.log"

#define PID_MIHF		0
#define PID_MIIS_SERVER		1
#define PID_LINK_ETH		2
#define PID_LINK_WIFI		3
#define PID_LINK_LTE		4
#define PID_MIH_USR		5
#define PID_HIP_DAEMON		6
#define PID_MIPV6_DAEMON	7
#define PID_NUM			8

extern int pids[PID_NUM];

#define WAIT_TIME		1
#define SINE_TIMEOUT		5

#define LOC_BUF_SIZE		16
#define IPC_BUF_SIZE		32
#define LINE_BUF_SIZE		256
#define PATH_BUF_SIZE		256
#define NAME_BUF_SIZE		256
#define SOCK_BUF_SIZE		8192

#define CONMGR_SOCKET_NAME	"/tmp/sine_connection_manager"
#define LIB_SOCKET_NAME		"/tmp/libsine_"
#define SOCKS_IPV4
#define SOCKS_PORT		10808
#define SOCKS_LISTEN		64

#define PROC_PATH		"/proc"
#define PROC_TCP6_PATH		"/proc/net/tcp6"
#define PROC_TCP_PATH		"/proc/net/tcp"
#define PROC_FD_DIRNAME		"fd"
#define PROC_EXE_FILENAME	"exe"

extern volatile int exit_flag;
extern int exit_pipe[2];

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#endif
