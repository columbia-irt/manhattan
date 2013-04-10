#ifndef _SINE_SOCKS_H_
#define _SINE_SOCKS_H_

#include <asm/types.h>

#include "sined.h"

int init_socks_server(pthread_t *socks_thread_ptr);

#define SOCKS_V4	4
#define SOCKS_V5	5

#define SOCKS_METHOD_NONE		0x00
#define SOCKS_METHOD_GSSAPI		0x01
#define SOCKS_METHOD_USERPASS		0x02
#define SOCKS_METHOD_NOT_ACCEPTED	0xFF

typedef struct mtdreq {
	__u8	ver;
	__u8	nmethods;
	__u8	methods[256];
} mtdreq_t;

typedef struct mtdres {
	__u8	ver;
	__u8	method;
} mtdres_t;

#define SOCKS_CMD_CONNECT		0x01
#define SOCKS_CMD_BIND			0x02
#define SOCKS_CMD_UDP			0x03

#define SOCKS_ATYP_IPV4			0x01
#define SOCKS_ATYP_DOMAIN		0x03
#define SOCKS_ATYP_IPV6			0x04

#define SOCKS_REP_SUCCEEDED		0x00
#define SOCKS_REP_GENERAL_FAILURE	0x01
#define SOCKS_REP_NOT_ALLOWED		0x02
#define SOCKS_REP_NETWORK_UNREACHABLE	0x03
#define SOCKS_REP_HOST_UNREACHABLE	0x04
#define SOCKS_REP_CONNECTION_REFUSED	0x05
#define SOCKS_REP_TTL_EXPIRED		0x06
#define SOCKS_REP_CMD_NOT_SUPPORTED	0x07
#define SOCKS_REP_ATYP_NOT_SUPPORTED	0x08

struct sock_ipv4 {
	__u32	addr;
	__u16	port;
};

struct sock_domain {
	__u8	nbytes;
	__u8	data[257];	/* max 2^8-1 bytes of name and 2 bytes of port */
};

struct sock_ipv6 {
	__u8	addr[IPV6_ADDR_LEN];
	__u16	port;
};

typedef struct socksreq {
	__u8	ver;
	__u8	cmd;
	__u8	rsv;
	__u8	atyp;	/* address type */
	union {
		struct sock_ipv4 ipv4;
		struct sock_domain domain;
		struct sock_ipv6 ipv6;
	} dst;
} socksreq_t;

#define SOCKS_REQ_BASE_LEN	3

#endif
