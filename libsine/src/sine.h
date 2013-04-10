#ifndef _SINE_H_
#define _SINE_H_

#include <sys/socket.h>
#include <sys/un.h>

int sine_create_socket(const char *name);
inline int sine_create_addr(struct sockaddr *target, const char *name);
inline int sine_set_timeout(int sockfd, int timeout);
int sine_send_block(int from_sock, void *msg, size_t len,
			struct sockaddr *to_sa, socklen_t addrlen, int timeout);
int sine_send_nonblock(int from_sock, void *msg, size_t len,
			struct sockaddr *to_sa, socklen_t addrlen);
int sine_recv_ack(int sockfd, void *buf, size_t len, int timeout);
int sine_recv_noack(int sockfd, void *buf, size_t len, int timeout);
inline int sine_recv_from(int sockfd, void *buf, size_t len,
	struct sockaddr *from_sa, socklen_t *addrlen, int timeout);

#endif
