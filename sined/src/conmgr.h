#ifndef _SINE_CONMGR_H_
#define _SINE_CONMGR_H_

#include <netinet/in.h>
#include "sined.h"

#define CONMGR_CMD_ADD		0
#define CONMGR_CMD_REMOVE	1

struct connection {
	int appID;
	int sockfd;
	int parent_sockfd;
	int policyID;
	int status;
	socklen_t addrlen;
	const struct sockaddr *addr;
	char *app_desc;

	struct connection *next;
};

typedef struct conmsg {
	int cmd;
	pid_t pid;
	int sockfd;
	socklen_t addrlen;
	struct sockaddr_storage addr;
} conmsg_t;
	

void * main_connection_manager(void *arg);

extern struct connection *con_tbl_head;
extern struct connection *con_tbl_tail;
extern pthread_mutex_t con_tbl_mutex;

#endif
