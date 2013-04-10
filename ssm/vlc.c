#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vlc/vlc.h>
#include <dbg.h>

#define BUF_SIZE 256
#define SSM_PORT 18888
#define LOCAL_MRL "http://127.0.0.1:8888"
#define BROADCAST_NAME "ssm_broadcast"

libvlc_instance_t *inst;
libvlc_media_player_t *mp;

int open_media(const char *mrl)
{
	/* broadcast to Internet */
	debug("libvlc_vlm_add_broadcast from input %s", mrl);
	check(!libvlc_vlm_add_broadcast(inst, BROADCAST_NAME, mrl,
		"#std{access=http,mux=ts,dst=0.0.0.0:8888}", 0, NULL, 1, 0),
		"libvlc_vlm_add_broadcast");

	libvlc_vlm_play_media(inst, BROADCAST_NAME);
	return 0;

error:
	return -1;
}

void local_display(const char *mrl)
{
	libvlc_media_t *m;

	debug("local display from input %s", mrl);
	m = libvlc_media_new_path(inst, mrl);
	mp = libvlc_media_player_new_from_media(m);
	libvlc_media_release(m);

	libvlc_media_player_play(mp);
}

void close_display()
{
	if (mp == NULL)
		return;

	debug("close_display");
	libvlc_media_player_stop(mp);
	libvlc_media_player_release(mp);
	mp = NULL;
}

int init_ssm_socket()
{
	int sockfd;
	struct sockaddr_in sa;
	int on = 1;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	debug("sockfd: %d", sockfd);
	check(sockfd >= 0, "creat socket");

	check(!setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)),
		"setsockopt SO_REUSEADDR");

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(SSM_PORT);
	check(!bind(sockfd, (struct sockaddr *)&sa,
		sizeof(struct sockaddr_in)), "bind");

	check(!listen(sockfd, 0), "listen");

	return sockfd;

error:
	if (sockfd >= 0)
		close(sockfd);
	return -1;
}

void handle_user_input()
{
	char buf[BUF_SIZE];
	char *cmd;
	char *arg;
	char *saveptr = NULL;
	int i;

	check(fgets(buf, BUF_SIZE, stdin), "fgets");
	for (i = 0; buf[i] != '\n' && buf[i] != '\0'; ++i);
	buf[i] = '\0';

	cmd = strtok_r(buf, " ", &saveptr);
	arg = strtok_r(NULL, " ", &saveptr);

	if (!strcmp(cmd, "open") && arg != NULL) {
		/* TODO: open again? */
		if (!open_media(arg))
			local_display(LOCAL_MRL);
	}
	else if (!strcmp(cmd, "forward") && arg != NULL) {
		int sockfd;
		struct sockaddr_in sa;

		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		check(sockfd >= 0, "creat socket");

		sa.sin_family = AF_INET;
		check(inet_aton(arg, &sa.sin_addr), "invalid IP address");
		sa.sin_port = htons(SSM_PORT);
		check(!connect(sockfd, (struct sockaddr *)&sa,
			sizeof(struct sockaddr_in)), "connect to %s", arg);

		/* must wait for response, if succeeded (1), close the local display */
		strcpy(buf, "open");
		send(sockfd, buf, strlen(buf), 0);
		check(recv(sockfd, buf, BUF_SIZE, 0) > 0,
			"failed to get response");

		if (buf[0] == '1')
			close_display();
	}
	else if (!strcmp(cmd, "stop")) {
		close_display();
		debug("stop & delete media");
		libvlc_vlm_stop_media(inst, BROADCAST_NAME);
		libvlc_vlm_del_media(inst, BROADCAST_NAME);
	}

error:
	return;
}

void handle_network_input(int sockfd)
{
	int new_sockfd;
	struct sockaddr_in sa;
	socklen_t addrlen;
	char buf[BUF_SIZE];
	char addr[INET_ADDRSTRLEN];
	int r;

	addrlen = sizeof(struct sockaddr_in);
	new_sockfd = accept(sockfd, (struct sockaddr *)&sa, &addrlen);
	check(new_sockfd >= 0, "accept");

	r = recv(new_sockfd, buf, BUF_SIZE, 0);
	check(r > 0, "recv");
	buf[r] = '\0';
	debug("network message received: %s", buf);

	if (!strcmp(buf, "open")) {
		char *ip = inet_ntoa(sa.sin_addr);
		sprintf(buf, "http://%s:8888", ip);
		local_display(buf);
	}
	else if (!strcmp(buf, "stop")) {
		close_display();
	}

	strcpy(buf, "1");
	send(new_sockfd, buf, strlen(buf), 0);
	close(new_sockfd);

error:
	if (new_sockfd >= 0)
		close(new_sockfd);
}

int main()
{
	fd_set rfds;
	int sockfd;
	int maxfd;
	int r;

	inst = libvlc_new(0, NULL);
	mp = NULL;
	sockfd = init_ssm_socket();
	check(sockfd >= 0, "failed to create socket for ssm");

	maxfd = ((sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO) + 1;

	while (1) {
		FD_ZERO(&rfds);
		FD_SET(STDIN_FILENO, &rfds);
		FD_SET(sockfd, &rfds);
		r = select(maxfd, &rfds, NULL, NULL, NULL);

		check(r > 0, "select() returns %d", r);

		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			handle_user_input();
		}
		else if (FD_ISSET(sockfd, &rfds)) {
			handle_network_input(sockfd);
		}
	}

error:
	if (sockfd > 0)
		close(sockfd);
	libvlc_release(inst);

	return 0;
}
