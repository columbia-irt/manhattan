#ifndef _SINE_CONMGR_H_
#define _SINE_CONMGR_H_

#include <netinet/in.h>
#include "sined.h"

#define CONMGR_CMD_ADD		0
#define CONMGR_CMD_REMOVE	1

#define CONMGR_PROTO_UNSPEC	0
#define CONMGR_PROTO_IPV4	1
#define CONMGR_PROTO_HIP	2
#define CONMGR_PROTO_MIPV6	3
#define CONMGR_PROTO_NUM	4

struct connection {
	int appID;
	char *app_desc;

	int sockfd;
	socklen_t addrlen;
	struct sockaddr *addr;
	int protocol;
	int active_rule;

	//pthread_mutex_t mutex;	//hold con_tbl_mutex
	struct connection *next;
};

typedef struct conmsg {
	int cmd;
	pid_t pid;
	int sockfd;
	socklen_t addrlen;
	struct sockaddr_storage addr;
} conmsg_t;
	
int init_connection_manager(pthread_t *conmgr_thread_ptr);
void conmgr_dump_connections();

extern int conmgr_sock;

extern struct connection *con_tbl_head;
extern struct connection *con_tbl_tail;
extern pthread_mutex_t con_tbl_mutex;

#endif
