#ifndef _SINE_H_ 
#define _SINE_H_

#include <sys/socket.h>

extern int sine_socket (int __domain, int __type, int __protocol,
			pid_t __pid) __THROW;

extern int sine_bind (int __fd, __CONST_SOCKADDR_ARG __addr,
			socklen_t __len) __THROW;

extern int sine_connect (int __fd, __CONST_SOCKADDR_ARG __addr,
			socklen_t __len);

extern int sine_listen (int __fd, int __n) __THROW;

extern int sine_accept (int __fd, __SOCKADDR_ARG __addr,
			socklen_t *__restrict __addr_len);

extern int sine_close (int __fd);

int sine_sendto_conmgr(void *buf, size_t len);

#endif
