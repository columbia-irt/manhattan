#ifndef _CONMGR_H_
#define _CONMGR_H_

#include <netinet/in.h>

struct connection {
	int sockfd;
	int parent_sockfd;
	int appID;
	int policyID;
	int status;
	socklen_t addrlen;
	const struct sockaddr *addr;
	struct connection *next;
};

void * main_connection_manager(void *arg);

extern struct connection *con_tbl_head;
extern struct connection *con_tbl_tail;

#endif
