#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "socks.h"
#include "sined.h"

#define LISTEN_BACKLOG	64

static int serv_sock_v4 = -1;
static int serv_sock_v6 = -1;
static fd_set read_fds;

static int init_server()
{
	const int optval = 1;	/* TODO: what's this ? */
	struct sockaddr_in serv_addr_v4;
	struct sockaddr_in6 serv_addr_v6;

	serv_sock_v4 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	check(serv_sock_v4 >= 0, "Create ipv4 socket");
	check(!setsockopt(serv_sock_v4, SOL_SOCKET, SO_REUSEADDR,
			(char *)&optval, sizeof(optval)),
		"Set socket options for serv_sock_v4");
	serv_addr_v4.sin_addr.s_addr = INADDR_ANY;
	serv_addr_v4.sin_port = htons(SOCKS_PORT);
	check(!bind(serv_sock_v4, (struct sockaddr *)&serv_addr_v4,
		sizeof(serv_addr_v4)), "Bind serv_sock_v4");
	check(!listen(serv_sock_v4, SOCKS_LISTEN), "Set serv_sock_v4 listen");
	
	serv_sock_v6 = socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP);
	check(serv_sock_v6 >= 0, "Create ipv6 socket");
	check(!setsockopt(serv_sock_v6, SOL_SOCKET, SO_REUSEADDR,
			(char *)&optval, sizeof(optval)),
		"Set socket options for serv_sock_v6");
	serv_addr_v6.sin6_addr = in6addr_any;
	serv_addr_v6.sin6_port = htons(SOCKS_PORT + 1);
	check(!bind(serv_sock_v6, (struct sockaddr *)&serv_addr_v6,
		sizeof(serv_addr_v6)), "Bind serv_sock_v6");
	check(!listen(serv_sock_v6, SOCKS_LISTEN), "Set serv_sock_v6 listen");

	FD_ZERO(&read_fds);
	FD_SET(serv_sock_v4, &read_fds);
	FD_SET(serv_sock_v6, &read_fds);

	return 0;

error:
	return -1;
}

void * main_socks_server(void *arg)
{
	int max_fd;
	int req_sock;
	int cli_sock;
	int r, i;
	
	struct sockaddr sa;
	socklen_t len;
	char buf[128];

	check(!init_server(), "Failed to initialize socks server");
	debug("socks server has initialized");

	max_fd = (serv_sock_v4 > serv_sock_v6 ? serv_sock_v4 : serv_sock_v6);

	while (1) {
		/* TODO: except fds */
		select(max_fd + 1, &read_fds, NULL, NULL, NULL);

		req_sock = -1;
		if (FD_ISSET(serv_sock_v4, &read_fds))
			req_sock = serv_sock_v4;
		else if (FD_ISSET(serv_sock_v6, &read_fds))
			req_sock = serv_sock_v6;

		if (req_sock >= 0) {
			cli_sock = accept(req_sock, &sa, &len);
			if (cli_sock >= 0) {
				debug("incomming connection");
				r = recv(cli_sock, buf, sizeof(buf), 0);
				buf[0] = 5;
				buf[1] = 0;
				send(cli_sock, buf, 2, 0);
				r = recv(cli_sock, buf, sizeof(buf), 0);
				buf[1] = 0;
				send(cli_sock, buf, r, 0);
				r = recv(cli_sock, buf, sizeof(buf), 0);
				for (i = 0; i < r; ++i)
					debug("%x %c", buf[i], buf[i]);
				close(cli_sock);
			}
		}
	}

	return 0;

error:
	if (serv_sock_v4 >= 0)
		close(serv_sock_v4);
	if (serv_sock_v6 >= 0)
		close(serv_sock_v6);

	return (void *)-1;	/* TODO: (int)status */
}
