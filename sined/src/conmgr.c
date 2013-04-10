#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include "pe.h"
#include "conmgr.h"

/*
#define NEW		0
#define SEND		1
#define RECV		2
#define CONNECTING	3
#define ESTABLISHED	4
#define LISTENING	5
#define FIN		6
*/

int conmgr_sock = -1;

struct connection *con_tbl_head = NULL;
struct connection *con_tbl_tail = NULL;
pthread_mutex_t con_tbl_mutex = PTHREAD_MUTEX_INITIALIZER;
//fpos_t pos;				//sabari

//static const char *str_status[FIN+1] = {"NEW", "SEND", "RECV", "CONNECTING" , "ESTABLISHED", "LISTENING", "FIN"};
static const char *str_proto[CONMGR_PROTO_NUM] =
	{"UNSPEC", "IPv4", "HIP", "MIPv6"};

static inline const char * proto_to_str(int protocol)
{
	if (protocol < 0 || protocol >= CONMGR_PROTO_NUM)
		return "INVALID";
	else
		return str_proto[protocol];
}

/* not thread safe, must be called while holding con_tbl_mutex */
static inline void dump_one_connection(struct connection *conn)
{
	struct sockaddr_in *addr4;
	struct sockaddr_in6 *addr6;
	char str_addr[INET6_ADDRSTRLEN + 1];
	char str_addr_port[INET6_ADDRSTRLEN + 1];

	//pthread_mutex_lock(&conn->mutex);	//hold con_tbl_mutex
	if (conn->addr) {
		switch (conn->addr->sa_family) {
		case AF_INET:
			addr4 = (struct sockaddr_in *)conn->addr;
			inet_ntop(AF_INET, &addr4->sin_addr,
					str_addr, INET_ADDRSTRLEN);
			sprintf(str_addr_port, "%s:%d", str_addr,
					ntohs(addr4->sin_port));
			break;
		case AF_INET6:
			addr6 = (struct sockaddr_in6 *)conn->addr;
			inet_ntop(AF_INET6, &addr6->sin6_addr,
					str_addr, INET6_ADDRSTRLEN);
			sprintf(str_addr_port, "[%s]:%d", str_addr,
					ntohs(addr6->sin6_port));
			break;
		default:
			strcpy(str_addr_port, "(unknown address)");
		}
	} else {
		strcpy(str_addr_port, "(null)");
	}
	log_info("    Connection: %-5d %5d %-6s %4d %s\t%s",
		conn->appID, conn->sockfd, proto_to_str(conn->protocol),
		conn->active_rule, str_addr_port, conn->app_desc);
	//pthread_mutex_unlock(&conn->mutex);	//hold con_tbl_mutex
}

/* TODO: to string */
void conmgr_dump_connections()
{
	struct connection *conn;

	pthread_mutex_lock(&con_tbl_mutex);

	// get the current position 
	/*	sabari
	FILE *file = fopen("connections.out","w"); 
	fflush(file);
	if (fgetpos(file,&pos) == -1) 
	{ 
		fprintf(file,"Unable to refresh screen\n");
		fclose(file);
	} 
	*/
	log_info("------------------------");
	log_info("Printing Connection Info");
	log_info("------------------------");
	log_info("Connection: APPID    FD PROTO  RULE REMOTE             \tDESC");
	for (conn = con_tbl_head->next; conn != NULL; conn = conn->next)
		dump_one_connection(conn);
	log_info("------------------------");

	pthread_mutex_unlock(&con_tbl_mutex);
	/*	sabari
	if (fsetpos(file,&pos) == -1) 
	{ 
		fprintf(file,"Unable to refresh screen\n");
		fclose(file);
	}
	fclose(file); 
	*/
}

/* get application path through pid, return value must be freed */
static char * get_app_path(pid_t pid)
{
	char link_path[PATH_BUF_SIZE];
	char buf[LINE_BUF_SIZE];
	char *app_path = NULL;
	int r;

	sprintf(link_path, "%s/%d/%s", PROC_PATH, pid, PROC_EXE_FILENAME);
	debug("link path: %s", link_path);

	r = readlink(link_path, buf, LINE_BUF_SIZE);
	check(r >= 0, "readlink %s", link_path);

	debug("app path length: %d", r);
	app_path = malloc(r + 1);
	check(app_path, "malloc failure");

	if (r < LINE_BUF_SIZE) {
		strncpy(app_path, buf, r);
	} else {
		check(readlink(link_path, app_path, r + 1) >= 0,
			"readlink %s for the second time", link_path);
	}
	app_path[r] = '\0';

	debug("app path for pid %d: %s", pid, app_path);
	return app_path;

error:
	if (app_path) {
		free(app_path);
		app_path = NULL;
	}
	return NULL;
}

static int init_connection_table() {
	pthread_mutex_lock(&con_tbl_mutex);
	if(!con_tbl_head) {
		con_tbl_head = malloc(sizeof(struct connection)); /* head */
		memset(con_tbl_head, 0, sizeof(struct connection));
		check(con_tbl_head, "Failed to allocate space for connection table (%s)", strerror(errno));
		con_tbl_head->next = NULL;
		con_tbl_tail = con_tbl_head;
	}
	pthread_mutex_unlock(&con_tbl_mutex);
	return 0;

error:
	pthread_mutex_unlock(&con_tbl_mutex);
	return -1;
} 

static int add_connection(int appID, int sockfd,
			struct sockaddr *addr, socklen_t addrlen)
{
	struct connection *conn = NULL;

	conn = malloc(sizeof(struct connection));
	check(conn, "malloc for connection failed");
	memset(conn, 0, sizeof(struct connection));

	conn->appID = appID;
	conn->app_desc = get_app_path(appID);

	conn->sockfd = sockfd;
	conn->active_rule = 0;
	conn->protocol = CONMGR_PROTO_UNSPEC;

	conn->addr = NULL;
	debug("addrlen: %d", addrlen);
	conn->addr = malloc(addrlen);
	check(conn->addr, "malloc %d bytes for conn->addr failure", addrlen);
	memcpy(conn->addr, addr, addrlen);
	conn->addrlen = addrlen;

	//pthread_mutex_init(&conn->mutex, NULL);	//hold con_tbl_mutex
	conn->next = NULL;
	dump_one_connection(conn);

	pthread_mutex_lock(&con_tbl_mutex);
	evaluate(conn);
	con_tbl_tail->next = conn;
	con_tbl_tail = conn;
	pthread_mutex_unlock(&con_tbl_mutex);

	//conmgr_dump_connections();
	return 0;

error:
	if (conn) {
		if (conn->addr)
			free(conn->addr);
		free(conn);
	}

	return -1;
} 

/* sabari
static void add_child_connection(int sockfd, int new_sockfd){
	struct connection *conn;
	conn = calloc(1, sizeof(struct connection));
	con_tbl_tail->next = conn;
	if(!conn) {
		printf("Error add_connection");
		return;
	}
	conn->sockfd = new_sockfd;
	conn->parent_sockfd = sockfd;
	conn->appID = 31;
	conn->policyID = 0;
	conn->status = ESTABLISHED;
	conn->next = NULL;
	//update_con_tbl_tail(conn);
	//conmgr_dump_connections();
} 

static int update_connection_bind(int sockfd, struct sockaddr *addr,
					socklen_t addrlen) {
	
	struct connection *conn = con_tbl_head->next;
	while(conn->next != NULL) {
		if (conn->sockfd == sockfd) {
			conn->addr = addr;
			conn->addrlen = addrlen;
			return 0;	
		}
		conn = conn->next;
	} 	

	//conmgr_dump_connections();
	return 1;

}

static int update_connection_status(int sockfd, int status){
	
	struct connection *conn = con_tbl_head->next;
	while(conn->next != NULL) {
		if (conn->sockfd == sockfd) {
			conn->status = status;
			return 0;	
		}
		conn = conn->next;
	} 	
	//conmgr_dump_connections();
	return 1;

}
*/

static void free_connection(struct connection *conn) {
	//pthread_mutex_lock(&conn->mutex);	//hold con_tbl_mutex

	if (conn->addr)
		free(conn->addr);
	if (conn->app_desc)
		free(conn->app_desc);

	//pthread_mutex_unlock(&conn->mutex);	//hold con_tbl_mutex
	//pthread_mutex_destroy(&conn->mutex);	//hold con_tbl_mutex

	free(conn);
}

static void remove_connection(int appID, int sockfd) {
	struct connection *pred_conn;
	struct connection *curr_conn;

	pthread_mutex_lock(&con_tbl_mutex);
	pred_conn = con_tbl_head;
	curr_conn = pred_conn->next;

	while (curr_conn != NULL) {
		if (curr_conn->sockfd == sockfd &&
		    curr_conn->appID == appID) {
			pred_conn->next = curr_conn->next;
			free_connection(curr_conn);
			curr_conn = pred_conn->next;
			/* update tail when the last connection is removed */
			if (curr_conn == NULL) {
				con_tbl_tail = pred_conn;
				break;
			}
		} else {
			pred_conn = curr_conn;
			curr_conn = pred_conn->next;
		}
	} 	

	pthread_mutex_unlock(&con_tbl_mutex);
}

/* sabari
void update_con_tbl_tail(struct connection *conn) {
   pthread_mutex_lock( &mutex );
   con_tbl_tail = conn;
   pthread_mutex_unlock( &mutex );
}
*/

static void free_connection_table()
{
	struct connection *curr_conn;

	if (!con_tbl_head) return;

	pthread_mutex_lock(&con_tbl_mutex);
	while (con_tbl_head->next != NULL) {
		curr_conn = con_tbl_head->next;
		con_tbl_head->next = curr_conn->next;
		free_connection(curr_conn);
	}
	free_connection(con_tbl_head);
	con_tbl_head = NULL;
	con_tbl_tail = NULL;
	pthread_mutex_unlock(&con_tbl_mutex);
}

static void * conmgr_main_loop()
{
#ifdef DEBUG_CONMGR
	printf("This is a debugging version, please use the following command to control connection manager:\n");
	printf("\ta <sockfd> <appID>\t-\tadd a connection\n");
	printf("\tc <sockfd> <child_sockfd>\t-\tadd a child connection\n");
	printf("\ts <sockfd> <status>\t-\tupdate the status of an existing connection specified by sockfd\n");
	printf("\td <sockfd> <appID>\t-\tdelete an existing connection specified by sockfd and appID\n");

	while (!exit_flag) {
		char choice;
		int socket;
		int appID;

		scanf("%c %d %d", &choice, &socket, &appID);
		debug("%c %d %d\n", choice, socket, appID);
		switch (choice) {
		case 'a':
		case 'n':
			debug("new connection");
			add_connection(appID, socket, "", 0);
			break;
		/*case 'c':
			debug("child connection");
			add_child_connection(socket, appID);
			break;
		case 's':
		case 'u':
			debug("update status");
			update_connection_status(socket, appID);
			break; */
		case 'r':
		case 'd':
			debug("remove connection");
			remove_connection(appID, socket);
		}
		conmgr_dump_connections();
	}
#else
	//char buf[SOCK_BUF_SIZE];
	fd_set conmgr_fds;
	struct sockaddr_un from_addr;
	socklen_t addrlen;
	conmsg_t con_msg;
	int max_fd;
	int r;

	max_fd = MAX(conmgr_sock, exit_pipe[0]);

	while (!exit_flag) {
		FD_ZERO(&conmgr_fds);
		FD_SET(conmgr_sock, &conmgr_fds);
		FD_SET(exit_pipe[0], &conmgr_fds);

		r = select(max_fd + 1, &conmgr_fds, NULL, NULL, NULL);
		check(r >= 0, "Select exception");
		debug("select returns %d", r);

		if (FD_ISSET(exit_pipe[0], &conmgr_fds))
			break;

		if (FD_ISSET(conmgr_sock, &conmgr_fds)) {
			addrlen = sizeof(struct sockaddr_un);
			r = recvfrom(conmgr_sock, &con_msg, sizeof(conmsg_t),
				0, (struct sockaddr *)&from_addr, &addrlen);
			//r = read(conmgr_sock, &con_msg, sizeof(conmsg_t));
			check(r > 0, "read connection message returns %d", r);

			debug("message received from pid %d", con_msg.pid);
			switch (con_msg.cmd) {
			case CONMGR_CMD_ADD:
				add_connection(con_msg.pid, con_msg.sockfd,
					(struct sockaddr *)&con_msg.addr,
					con_msg.addrlen);
				break;
			case CONMGR_CMD_REMOVE:
				remove_connection(con_msg.pid, con_msg.sockfd);
				break;
			default:
				log_warn("unknown command");
				break;
			}

			if (sendto(conmgr_sock, &r, sizeof(int), 0,
				(struct sockaddr *)&from_addr, addrlen) < 0)
				log_warn("failed to respond");
		}
	}

error:
	free_connection_table();
	if (conmgr_sock >= 0) {
		close(conmgr_sock);
		conmgr_sock = -1;
		unlink(CONMGR_SOCKET_NAME);
	}
#endif

	log_info("connection manager exiting");

	return NULL;
}

int init_connection_manager(pthread_t *conmgr_thread_ptr)
{
#ifdef DEBUG_CONMGR
	init_connection_table();
	pthread_create(conmgr_thread_ptr, NULL, conmgr_main_loop, NULL);
	return 0;

#else
	struct sockaddr_un name;

	check(!init_connection_table(), "Initializing connection table");

	conmgr_sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	check(conmgr_sock >= 0, "Failed to create UNIX datagram socket");

	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, CONMGR_SOCKET_NAME);
	unlink(name.sun_path);
	check(!bind(conmgr_sock, (struct sockaddr *)&name,
			sizeof(struct sockaddr_un)),
		"Failed to bind socket with name %s", CONMGR_SOCKET_NAME);

	check(!pthread_create(conmgr_thread_ptr, 0, conmgr_main_loop, 0),
		"Failed to create thread for connection manager");

	debug("Connection Manager initialized");
	return 0;

error:
	free_connection_table();
	if (conmgr_sock >= 0) {
		close(conmgr_sock);
		conmgr_sock = -1;
		unlink(CONMGR_SOCKET_NAME);
	}

	return -1;
#endif
}
