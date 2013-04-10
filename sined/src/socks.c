#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

#include <sine.h>
#include "conmgr.h"
#include "socks.h"
#include "sined.h"

#define LISTEN_BACKLOG	64

static int serv_sock = -1;
static fd_set socks_fds;

/* for now, just assume that each socket is owned by only one process */
/* only the main thread of socks server will call this function */
#ifdef SOCKS_IPV6
static pid_t get_pid_by_sockaddr(struct sockaddr_in6 *sa)
#else
static pid_t get_pid_by_sockaddr(struct sockaddr_in *sa)
#endif
{
	/* common variables */
	pid_t pid = -1;

	char inode_desc[LINE_BUF_SIZE];
	char buf[LINE_BUF_SIZE];
	char path[PATH_BUF_SIZE];
	struct dirent *ent;
	DIR *proc = NULL;
	DIR *fd_dir = NULL;
	FILE *fp = NULL;
	char *local, *remote;
	char *saveptr;
	int i;

#ifdef SOCKS_IPV6
	char src[INET6_ADDRSTRLEN] = {0};
	char dst[INET6_ADDRSTRLEN] = {0};

	if (IN6_IS_ADDR_V4MAPPED(sa->sin6_addr.s6_addr)) {
		sprintf(src, "%08X", *((__u32 *)sa->sin6_addr.s6_addr + 3));
		/* sa->sin_addr must be localhost */
		sprintf(dst, "%s:%04X", src, SOCKS_PORT);
		sprintf(src, "%s:%04X", src, ntohs(sa->sin6_port));
		debug("%s %s", src, dst);

		fp = fopen(PROC_TCP_PATH, "r");
		check(fp, "Failed to open %s", PROC_TCP_PATH);
	} else {
		debug("%d", sizeof(__u32));
		for (i = 0; i < IPV6_ADDR_LEN / sizeof(__u32); ++i) {
			sprintf(src + i * 8, "%08X",
				*((int *)sa->sin6_addr.s6_addr + i));
		}
		/* leverage the localhost in src */
		sprintf(dst, "%s:%04X", src, SOCKS_PORT);
		sprintf(src, "%s:%04X", src, ntohs(sa->sin6_port));
		debug("%s %s", src, dst);

		fp = fopen(PROC_TCP6_PATH, "r");
		check(fp, "Failed to open %s", PROC_TCP6_PATH);
	}

#else
	char src[INET6_ADDRSTRLEN] = {0};
	char dst[INET6_ADDRSTRLEN] = {0};

	sprintf(src, "%08X", sa->sin_addr.s_addr);
	/* sa->sin_addr must be localhost */
	sprintf(dst, "%s:%04X", src, SOCKS_PORT);
	sprintf(src, "%s:%04X", src, ntohs(sa->sin_port));
	debug("%s %s", src, dst);

	fp = fopen(PROC_TCP_PATH, "r");
	check(fp, "Failed to open %s", PROC_TCP_PATH);

#endif

	inode_desc[0] = '\0';
	check(fgets(buf, LINE_BUF_SIZE, fp), "%s or %s empty!",
		PROC_TCP_PATH, PROC_TCP6_PATH);		/* title */
	while (fgets(buf, LINE_BUF_SIZE, fp)) {
		strtok_r(buf, " ", &saveptr);		/* sl */
		local = strtok_r(NULL, " ", &saveptr);	/* local_address */
		remote = strtok_r(NULL, " ", &saveptr);	/* rem_address */
		if (strcmp(local, src) || strcmp(remote, dst))
			continue;

		/* st, tx_queue:rx_queue, tr:tm->when */
		/* retrnsmt, uid, timeout, inode */
		for (i = 0; i < 6; ++i)
			strtok_r(NULL, " ", &saveptr);
		sprintf(inode_desc, "socket:[%s]",
			strtok_r(NULL, " ", &saveptr));
		debug("%s", inode_desc);
		break;
	}
	fclose(fp);
	fp = NULL;
	check(inode_desc[0] != '\0',
		"socket (src: %s, dst: %s) not found in %s or %s",
		src, dst, PROC_TCP_PATH, PROC_TCP6_PATH);

	proc = opendir(PROC_PATH);
	check(proc, "Failed to open dir %s", PROC_PATH);

	while ((ent = readdir(proc)) != NULL) {
		pid = atoi(ent->d_name);
		sprintf(buf, "%d", pid);
		if (strcmp(buf, ent->d_name))	/* not a pid */
			continue;

		sprintf(path, "%s/%s/%s", PROC_PATH,
			ent->d_name, PROC_FD_DIRNAME);
		i = strlen(path);	/* for later concatenation */
		fd_dir = opendir(path);
		if (!fd_dir)	/* does not have fd dir */
			continue;

		while ((ent = readdir(fd_dir)) != NULL) {
			sprintf(path + i, "/%s", ent->d_name);	/* concat */
			if (readlink(path, buf, PATH_BUF_SIZE) < 0)
				continue;
			buf[strlen(inode_desc)] = '\0';
			if (!strcmp(inode_desc, buf)) {
				closedir(proc);
				closedir(fd_dir);
				return pid;
			}
		}

		closedir(fd_dir);
	}

error:
	if (fp)
		fclose(fp);
	if (proc)
		closedir(proc);
	if (fd_dir)
		closedir(fd_dir);

	return -1;
}

static void * forward_loop(void *arg)
{
	int cli_sock = *(int *)arg;
	int rem_sock = *((int *)arg + 1);
	pid_t pid = *((int *)arg + 2);

	conmsg_t con_msg;

	fd_set forward_fds;
	struct timeval tv;
	char buf[SOCK_BUF_SIZE];
	int max_fd;
	int len;
	int r;

	free(arg);	/* solve racing issues */
	debug("cli_sock: %d (pid %d), rem_sock: %d", cli_sock, pid, rem_sock);

	max_fd = MAX(cli_sock, rem_sock);

	while (!exit_flag) {
		FD_ZERO(&forward_fds);
		FD_SET(cli_sock, &forward_fds);
		FD_SET(rem_sock, &forward_fds);
		/* TODO: timeout */
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		r = select(max_fd + 1, &forward_fds, NULL, NULL, NULL);
		if (r == 0) {
			debug("Select time out");
			continue;
		}
		check(r > 0, "Select exception");

		if (FD_ISSET(cli_sock, &forward_fds)) {
			len = recv(cli_sock, buf, sizeof(buf), 0);
			if (len == 0) {
				log_info("client side has closed the socket");
				break;
			}
			check(len > 0, "cli_sock %d recv %d", cli_sock, len);
			//debug("%d bytes from client to remote", len);

			r = send(rem_sock, buf, len, 0);
			check(r > 0, "rem_sock %d send %d", rem_sock, r);
			//debug("%d bytes forwarded", r);
			if (r != len)
				log_warn("sending exception - %d/%d", r, len);

		}

		if (FD_ISSET(rem_sock, &forward_fds)) {
			len = recv(rem_sock, buf, sizeof(buf), 0);
			if (len == 0) {
				log_info("remote side has closed the socket");
				break;
			}
			check(len > 0, "rem_sock %d recv %d", rem_sock, r);
			//debug("%d bytes from remote to client", len);

			r = send(cli_sock, buf, len, 0);
			check(r > 0, "cli_sock %d send %d", rem_sock, r);
			//debug("%d bytes forwarded", r);
			if (r != len)
				log_warn("sending exception - %d/%d", r, len);
		}
	}

error:
	close(cli_sock);
	close(rem_sock);

	/* remove connection from conneciton manager */
	con_msg.cmd = CONMGR_CMD_REMOVE;
	con_msg.pid = pid;
	con_msg.sockfd = rem_sock;
	if (sine_sendto_conmgr(&con_msg, sizeof(conmsg_t)) < 0)
		log_warn("Failed to contact connection manager");

	debug("thread exiting");
	return NULL;
}

static void socks_establish(int cli_sock, pid_t pid)
{
	mtdreq_t method_req;
	mtdres_t method_res;
	socksreq_t socks_req;
	socksreq_t socks_res;	/* socks_req and socks_res has the same structure */

	struct sockaddr *rem_addr_ptr;
	struct sockaddr_in rem_addr_v4;
	struct sockaddr_in6 rem_addr_v6;
	socklen_t addrlen;

	char node[LINE_BUF_SIZE];
	char service[LINE_BUF_SIZE];
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	int rem_sock = -1;
	int socket_family;
	int len;
	int i;

	pthread_t forward_thread;
	conmsg_t con_msg;
	int *arg;

	log_info("SOCKS connection received (PID: %d)", pid);
	len = recv(cli_sock, &method_req, sizeof(mtdreq_t), 0);
	debug("%d bytes received", len);
	check(len > 0, "Failed to receive method selection message");
	check(method_req.ver == SOCKS_V5, "We only support SOCKS5");

	method_res.ver = SOCKS_V5;
	for (i = 0; i < method_req.nmethods; ++i) {
		if (method_req.methods[i] == SOCKS_METHOD_NONE)
			break;
	}
	if (i < method_req.nmethods) {
		method_res.method = SOCKS_METHOD_NONE;
		len = send(cli_sock, &method_res, sizeof(mtdres_t), 0);
		debug("%d bytes sent", len);
		check(len == sizeof(mtdres_t),
			"Failed to send method selection");
	} else {
		method_res.method = SOCKS_METHOD_NOT_ACCEPTED;
		send(cli_sock, &method_res, sizeof(mtdres_t), 0);
		close(cli_sock);
		return;
	}

	len = recv(cli_sock, &socks_req, sizeof(socksreq_t), 0);
	debug("%d bytes received", len);
	check(len > 0, "Failed to receive SOCKS request");
	check(socks_req.ver == SOCKS_V5, "We only support SOCKS5");

	memcpy(&socks_res, &socks_req, len);

	/* pick out the address type */
	socket_family = -1;
	rem_addr_ptr = NULL;
	addrlen = 0;
	debug("address type: %d", socks_req.atyp);

	switch (socks_req.atyp) {
	case SOCKS_ATYP_IPV4:
		socket_family = AF_INET;
		rem_addr_v4.sin_family = AF_INET;
		rem_addr_v4.sin_addr.s_addr = socks_req.dst.ipv4.addr;
		rem_addr_v4.sin_port = socks_req.dst.ipv4.port;
		rem_addr_ptr = (struct sockaddr *)&rem_addr_v4;
		addrlen = sizeof(rem_addr_v4);

		debug("remote addr: %s, port: %d",
			inet_ntoa(rem_addr_v4.sin_addr),
			ntohs(rem_addr_v4.sin_port));
		break;

	case SOCKS_ATYP_DOMAIN:
		/* TODO fix hard code */
		addrlen = socks_req.dst.domain.nbytes;
		debug("domain received (%d bytes): %s", addrlen,
			socks_req.dst.domain.data);

		sprintf(service, "%u", ntohs(*(unsigned short *)
				(socks_req.dst.domain.data + addrlen)));
		if (*socks_req.dst.domain.data == '[') {
			addrlen -= 2;
			strncpy(node, (char *)socks_req.dst.domain.data + 1,
				addrlen);
			node[addrlen] = '\0';
		} else {
			strncpy(node, (char *)socks_req.dst.domain.data,
				addrlen);
			node[addrlen] = '\0';
		}
		debug("node: %s, service: %s", node, service);

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;	/* any protocol */
		check(!getaddrinfo(node, service, &hints, &result),
			"getaddrinfo");

		for (rp = result; rp != NULL; rp = rp->ai_next) {
			if (rp->ai_family == AF_INET) {
				memcpy(&rem_addr_v4, rp->ai_addr,
					rp->ai_addrlen);
				socket_family = AF_INET;
				addrlen = rp->ai_addrlen;
				rem_addr_ptr = (struct sockaddr*)&rem_addr_v4;

				debug("remote addr: %s, port: %d",
					inet_ntoa(rem_addr_v4.sin_addr),
					ntohs(rem_addr_v4.sin_port));
				break;
			} else if (rp->ai_family == AF_INET6) {
				memcpy(&rem_addr_v6, rp->ai_addr,
					rp->ai_addrlen);
				socket_family = AF_INET6;
				addrlen = rp->ai_addrlen;
				rem_addr_ptr = (struct sockaddr*)&rem_addr_v6;

				debug("remote addr: , port: %d",
					/* TODO: output ipv6 address */
					ntohs(rem_addr_v6.sin6_port));
				break;
			}
		}
		break;

	case SOCKS_ATYP_IPV6:
		socket_family = AF_INET6;
		rem_addr_v6.sin6_family = AF_INET6;
		memcpy(rem_addr_v6.sin6_addr.s6_addr,
			socks_req.dst.ipv6.addr, IPV6_ADDR_LEN);
		rem_addr_v6.sin6_port = socks_req.dst.ipv6.port;
		rem_addr_ptr = (struct sockaddr *)&rem_addr_v6;
		addrlen = sizeof(rem_addr_v6);
		debug("remote addr: , port: %d",
			/* TODO: output ipv6 address */
			ntohs(rem_addr_v6.sin6_port));
		break;

	default:
		break;
	}

	/* set reply */
	if (!rem_addr_ptr) {
		log_err("Address type not supported yet");
		socks_res.cmd = SOCKS_REP_ATYP_NOT_SUPPORTED;
	} else if (socks_req.cmd != SOCKS_CMD_CONNECT) {
		log_err("Command not supported yet");	/* TODO */
		socks_res.cmd = SOCKS_REP_CMD_NOT_SUPPORTED;
	} else {
		rem_sock = socket(socket_family, SOCK_STREAM, IPPROTO_TCP);
		if (rem_sock < 0) {
			log_err("Create remote socket");
			socks_res.cmd = SOCKS_REP_GENERAL_FAILURE;
		} else if (connect(rem_sock, rem_addr_ptr, addrlen) < 0) {
			switch (errno) {
			case ENETUNREACH:
				socks_res.cmd = SOCKS_REP_NETWORK_UNREACHABLE;
				break;
			case ETIMEDOUT:
				socks_res.cmd = SOCKS_REP_HOST_UNREACHABLE;
				break;
			case ECONNREFUSED:
				socks_res.cmd = SOCKS_REP_CONNECTION_REFUSED;
				break;
			default:
				socks_res.cmd = SOCKS_REP_GENERAL_FAILURE;
				break;
			}
			log_err("Failed to connect to the destination");
		} else {
			socks_res.cmd = SOCKS_REP_SUCCEEDED;
		}
	}

	/* send reply */
	i = send(cli_sock, &socks_res, len, 0);
	debug("%d bytes sent", i);
	check(i == len, "Failed to send SOCKS response");

	/* create new thread */
	if (socks_res.cmd == SOCKS_REP_SUCCEEDED) {
		arg = malloc(3 * sizeof(int));	/* solve racing issues */
		arg[0] = cli_sock;
		arg[1] = rem_sock;
		arg[2] = pid;

		/* add connection to conneciton manager */
		con_msg.cmd = CONMGR_CMD_ADD;
		con_msg.pid = pid;
		con_msg.sockfd = rem_sock;
		memcpy(&con_msg.addr, rem_addr_ptr, addrlen);
		con_msg.addrlen = addrlen;
		if (sine_sendto_conmgr(&con_msg, sizeof(conmsg_t)) < 0)
			log_warn("Failed to contact connection manager");

		pthread_create(&forward_thread, NULL, forward_loop, arg);
		log_info("new thread created for the new connection");

		return;
	}

error:
	if (rem_sock >= 0)
		close(rem_sock);

	close(cli_sock);
}

static void * socks_main_loop()
{
	int max_fd;
	int cli_sock;
	int r;

#ifdef SOCKS_IPV6
	struct sockaddr_in6 sa;
#else
	struct sockaddr_in sa;
#endif
	socklen_t len;
	pid_t pid;

	max_fd = MAX(serv_sock, exit_pipe[0]);

	while (!exit_flag) {
		FD_ZERO(&socks_fds);
		FD_SET(serv_sock, &socks_fds);
		FD_SET(exit_pipe[0], &socks_fds);

		/* TODO: except fds */
		r = select(max_fd + 1, &socks_fds, NULL, NULL, NULL);
		check(r >= 0, "Select exception");
		debug("select returns %d", r);

		if (FD_ISSET(exit_pipe[0], &socks_fds))
			break;

		if (FD_ISSET(serv_sock, &socks_fds)) {
			cli_sock = accept(serv_sock,
				(struct sockaddr *)&sa, &len);
			if (cli_sock < 0)
				continue;

			pid = get_pid_by_sockaddr(&sa);
			if (pid < 0) {
				log_warn("PID not found");
				close(cli_sock);
			} else {
				socks_establish(cli_sock, pid);
			}
		}
	}

error:
	if (serv_sock >= 0) {
		close(serv_sock);
		serv_sock = -1;
	}

	libsine_exit();

	//return (void *)-1;	/* TODO: (int)status */
	log_info("socks server exiting...");
	return NULL;
}

int init_socks_server(pthread_t *socks_thread_ptr)
{
	const int optval = 1;	/* TODO: what's this ? */
	sa_family_t sa_family;
#ifdef SOCKS_IPV6
	struct sockaddr_in6 serv_addr;
	sa_family = PF_INET6;
#else
	struct sockaddr_in serv_addr;
	sa_family = PF_INET;
#endif

	serv_sock = socket(sa_family, SOCK_STREAM, IPPROTO_TCP);
	check(serv_sock >= 0, "Create ipv6 socket");
	check(!setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&optval, sizeof(optval)),
		"Set socket options for serv_sock");
#ifdef SOCKS_IPV6
	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_addr = in6addr_any;
	serv_addr.sin6_port = htons(SOCKS_PORT);
#else
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(SOCKS_PORT);
#endif
	check(!bind(serv_sock, (struct sockaddr *)&serv_addr,
		sizeof(serv_addr)), "Bind serv_sock");
	check(!listen(serv_sock, SOCKS_LISTEN), "Set serv_sock to listen");

	check(!pthread_create(socks_thread_ptr, NULL, socks_main_loop, NULL),
		"Failed to create thread for socks server");

	debug("SOCKS server initialized");
	return 0;

error:
	if (serv_sock >= 0) {
		close(serv_sock);
		serv_sock = -1;
	}

	return -1;
}
