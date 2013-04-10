#include <unistd.h>

#include <dbg.h>
#include <sine.h>

int sine_create_socket(const char *name)
{
	int sockfd = -1;
	struct sockaddr_un sa;

	sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	check(sockfd >= 0, "create UNIX domain datagram socket");

	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, name);
	unlink(name);
	check(!bind(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_un)),
		"bind %s", name);

	return sockfd;

error:
	if (sockfd >= 0)
		close(sockfd);
	return -1;
}

inline int sine_create_addr(struct sockaddr *target, const char *name)
{
	struct sockaddr_un *sa = (struct sockaddr_un *)target;
	sa->sun_family = AF_UNIX;
	strcpy(sa->sun_path, name);
	return 0;
}

/* if timeout < 0, use the last timeout */
inline int sine_set_timeout(int sockfd, int timeout)
{
	struct timeval tv;

	if (timeout < 0)
		return 0;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
				(char *)&tv, sizeof(struct timeval));
}

int sine_send_block(int from_sock, void *msg, size_t len,
			struct sockaddr *to_sa, socklen_t addrlen, int timeout)
{
	int res;
	int sockfd = from_sock;
	char *name = ((struct sockaddr_un *)to_sa)->sun_path;

	if (sockfd < 0)
		sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	check(sockfd >= 0, "create UNIX domain datagram socket");

	check(sendto(sockfd, msg, len, 0, to_sa, addrlen) >= 0,
		"send message to %s", name);

	check(!sine_set_timeout(sockfd, timeout),
		"set response timeout to %d for message sent to %s", timeout, name);
	check(recvfrom(sockfd, &res, sizeof(int), 0, to_sa, &addrlen) >= 0,
		"get response for message sent to %s", name);

	if (from_sock < 0)
		close(sockfd);
	return res;

error:
	if (sockfd >= 0 && from_sock < 0)
		close(sockfd);
	return -1;
}

int sine_send_nonblock(int from_sock, void *msg, size_t len,
			struct sockaddr *to_sa, socklen_t addrlen)
{
	int sockfd = from_sock;

	if (sockfd < 0)
		sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	check(sockfd >= 0, "create UNIX domain datagram socket");

	check(sendto(sockfd, msg, len, 0, to_sa, addrlen) >= 0,
		"send message to %s", ((struct sockaddr_un *)to_sa)->sun_path);

	if (from_sock < 0)
		close(sockfd);
	return 0;

error:
	if (sockfd >= 0 && from_sock < 0)
		close(sockfd);
	return -1;
}

int sine_recv_ack(int sockfd, void *buf, size_t len, int timeout)
{
	int r;
	struct sockaddr_un from_sa;
	socklen_t addrlen = sizeof(struct sockaddr_un);

	check(!sine_set_timeout(sockfd, timeout),
		"set receiving timeout to %d for sockfd %d", timeout, sockfd);

	r = recvfrom(sockfd, buf, len, 0, (struct sockaddr *)&from_sa, &addrlen);
	check(r >= 0, "receive message");

	if (sendto(sockfd, &r, sizeof(int), 0,
			(struct sockaddr *)&from_sa, addrlen) < 0)
		log_warn("failed to send ack to %s", from_sa.sun_path);

	return r;

error:
	return -1;
}

int sine_recv_noack(int sockfd, void *buf, size_t len, int timeout)
{
	int r;
	struct sockaddr_un from_sa;
	socklen_t addrlen = sizeof(struct sockaddr_un);

	check(!sine_set_timeout(sockfd, timeout),
		"set receiving timeout to %d for sockfd %d", timeout, sockfd);

	r = recvfrom(sockfd, buf, len, 0, (struct sockaddr *)&from_sa, &addrlen);
	check(r >= 0, "receive message");
	return r;

error:
	return -1;
}

inline int sine_recv_from(int sockfd, void *buf, size_t len, struct sockaddr *from_sa,
			socklen_t *addrlen, int timeout)
{
	int r;

	*addrlen = sizeof(struct sockaddr_un);
	check(!sine_set_timeout(sockfd, timeout),
		"set receiving timeout to %d for sockfd %d", timeout, sockfd);

	r = recvfrom(sockfd, buf, len, 0, from_sa, addrlen);
	check(r >= 0, "receive message");
	return r;

error:
	return -1;
}
