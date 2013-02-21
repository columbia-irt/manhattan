#include <sine.h>
#include <dbg.h>
#include <sined.h>
#include <sys/un.h>
#include <unistd.h>

static int sock_conmgr = -1;
static struct sockaddr_un addr_conmgr;

int sine_sendto_conmgr(void *buf, size_t len)
{
	int r;

	if (sock_conmgr < 0) {
		sock_conmgr = socket(AF_UNIX, SOCK_DGRAM, 0);
		if (sock_conmgr < 0)
			return -1;

		addr_conmgr.sun_family = AF_UNIX;
		strcpy(addr_conmgr.sun_path, CONMGR_NAME);
	}

	r = sendto(sock_conmgr, buf, len, 0,
		(struct sockaddr *)&addr_conmgr, sizeof(struct sockaddr_un));
	if (r < 0) {
		close(sock_conmgr);
		sock_conmgr = -1;
	}

	return r;
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
	return 0;
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

int sine_close (int __fd)
{
	return 0;
}
