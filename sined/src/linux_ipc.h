#ifndef _SINE_IPC_H_
#define _SINE_IPC_H_

#define SINE_IPC_PIPE		0
#define SINE_IPC_SOCKETPAIR	1
#define SINE_IPC_UNIX_DGRAM	2
#define SINE_IPC_UNIX_STREAM	3
#define SINE_IPC_INET_DGRAM	4
#define SINE_IPC_INET_STREAM	5

struct ipc_addr {
	int type;
	union {
		int fd;
		struct sockaddr *sa;
	};
};

int ipc_create_pipe(struct ipc_addr *ia);
int ipc_create_unix_dgram(struct ipc_addr *ia, const char *name);
int ipc_create_inet_stream(struct ipc_addr *ia, int version, const char *ip, int port);

int ipc_send(struct ipc_addr *ia);
int ipc_recv(struct ipc_addr *ia);

#endif
