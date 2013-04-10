#include <dbg.h>
#include <conmgr.h>
#include <sine.h>
#include <sined.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <unistd.h>

static int lib_sock = -1;
static struct sockaddr_un conmgr_addr;

int sine_sendto_conmgr(void *buf, size_t len)
{
	struct sockaddr_un name;
	socklen_t addrlen = sizeof(struct sockaddr_un);
	int r;

	if (lib_sock < 0) {
		struct timeval tv;

		lib_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
		check(lib_sock >= 0, "create lib_sock");

		name.sun_family = AF_UNIX;
		sprintf(name.sun_path, "%s%u", LIB_SOCKET_NAME, getpid());
		unlink(name.sun_path);
		check(!bind(lib_sock, (struct sockaddr *)&name,
				sizeof(struct sockaddr_un)),
			"Failed to bind socket with name %s", name.sun_path);

		tv.tv_sec = SINE_TIMEOUT;
		tv.tv_usec = 0;
		check(!setsockopt(lib_sock, SOL_SOCKET, SO_RCVTIMEO,
					(char *)&tv, sizeof(struct timeval)),
			"set recv timeout for lib_sock");

		conmgr_addr.sun_family = AF_UNIX;
		strcpy(conmgr_addr.sun_path, CONMGR_SOCKET_NAME);
	}

	check(sendto(lib_sock, buf, len, 0,
		(struct sockaddr *)&conmgr_addr, addrlen) >= 0, "sendto");
	check(recvfrom(lib_sock, &r, sizeof(int), 0,
		(struct sockaddr *)&name, &addrlen) >= 0, "recvfrom");

	return r;

error:
	if (lib_sock >= 0) {
		close(lib_sock);
		lib_sock = -1;
	}

	return -1;
}

int libsine_exit()
{
	if (lib_sock >= 0) {
		char path[UNIX_PATH_MAX];
		sprintf(path, "%s%u", LIB_SOCKET_NAME, getpid());
		unlink(path);
		return close(lib_sock);
	}

	return 0;
}

int sine_socket(int __domain, int __type, int __protocol, pid_t __pid)
{
	return 0;
}

int sine_bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
	return 0;
}

int sine_connect(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len)
{
	conmsg_t con_msg;

	con_msg.cmd = CONMGR_CMD_ADD;
	con_msg.pid = getpid();
	con_msg.sockfd = __fd;
	if (sine_sendto_conmgr(&con_msg, sizeof(conmsg_t)) < 0)
		log_warn("Failed to contact connection manager");

	return connect(__fd, __addr, __len);
}

int sine_listen(int __fd, int __n)
{
	return 0;
}

int sine_accept(int __fd, __SOCKADDR_ARG __addr,
			socklen_t *__restrict __addr_len)
{
	return 0;
}

int sine_close(int __fd)
{
	conmsg_t con_msg;

	con_msg.cmd = CONMGR_CMD_REMOVE;
	con_msg.pid = getpid();
	con_msg.sockfd = __fd;
	if (sine_sendto_conmgr(&con_msg, sizeof(conmsg_t)) < 0)
		log_warn("Failed to contact connection manager");

	return close(__fd);
}
