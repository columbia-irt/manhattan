#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "conmgr.h"

#define NEW		0
#define SEND		1
#define RECV		2
#define CONNECTING	3
#define ESTABLISHED	4
#define LISTENING	5
#define FIN		6

struct connection *con_tbl_head = NULL;
struct connection *con_tbl_tail = NULL;
pthread_mutex_t con_tbl_mutex;		//sabari
//fpos_t pos;				//sabari

static const char *str_status[FIN+1] = {"NEW", "SEND", "RECV", "CONNECTING" , "ESTABLISHED", "LISTENING", "FIN"};

static inline const char * convert_status(int status) {
	if (status < 0 || status > FIN)
		return "UNKNOWN";
	else
		return str_status[status];
}

static inline void dump_connections() {
	struct connection *conn = con_tbl_head->next;
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
	log_info("Connection: FD\t STAT\t APPID");
	while(conn != NULL) {
		log_info("Connection: %d\t %s\t %d", conn->sockfd, convert_status(conn->status), conn->appID);
		conn = conn->next;
	} 	
	log_info("------------------------");
	/*	sabari
	if (fsetpos(file,&pos) == -1) 
	{ 
		fprintf(file,"Unable to refresh screen\n");
		fclose(file);
	}
	fclose(file); 
	*/
}

static int init_connection_table() {
	pthread_mutex_lock(&con_tbl_mutex);
	if(!con_tbl_head) {
		con_tbl_head = malloc(sizeof(struct connection)); /* head */
		memset(con_tbl_head, 0, sizeof(struct connection));
		check(con_tbl_head, "Failed to allocate space for connection table (%s)", strerror(errno));
		con_tbl_head->next = NULL;
		con_tbl_head->status = -1;
		con_tbl_tail = con_tbl_head;
		log_info("Connection Manager initialized");
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
	conn->sockfd = sockfd;
	conn->parent_sockfd = 0;
	conn->policyID = 0;
	conn->status = ESTABLISHED;
	conn->addr = NULL;
	conn->next = NULL;

	conn->addr = malloc(addrlen);
	check(conn->addr, "malloc %d bytes for conn->addr failure", addrlen);
	memcpy(conn->addr, addr, addrlen);
	conn->addrlen = addrlen;

	conn->app_desc = get_app_path(appID);

	pthread_mutex_lock(&con_tbl_mutex);
	con_tbl_tail->next = conn;
	con_tbl_tail = conn;
	pthread_mutex_unlock(&con_tbl_mutex);

	//dump_connections();
	return 0;

error:
	if (conn) {
		if (conn->addr)
			free(conn->addr);
		free(conn);
	}

	return -1;
} 

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
	//dump_connections();
} 

static int update_connection_bind(int sockfd, const void *addr, int addrlen){
	
	struct connection *conn = con_tbl_head->next;
	while(conn->next != NULL) {
		if (conn->sockfd == sockfd) {
			conn->addr = addr;
			conn->addrlen = addrlen;
			return 0;	
		}
		conn = conn->next;
	} 	

	//dump_connections();
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
	//dump_connections();
	return 1;

}

static void free_connection(struct connection *conn) {
	if (conn->addr)
		free(conn->addr);
	if (conn->app_desc)
		free(conn->app_desc);
	free(conn);
}

static void remove_connection(int sockfd, int appID) {
	struct connection *pred_conn = con_tbl_head;
	struct connection *curr_conn = pred_conn->next;

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
}

/* get application path through pid, return value must be freed */
static inline char * get_app_path(pid_t pid)
{
	struct stat sb;
	char link_path[PATH_BUF_SIZE];
	char *app_path = NULL;
	ssize_t len;

	sprintf(link_path, "%s/%d/%s", PROC_PATH, con_msg.pid, EXEFILE_NAME);
	check(!lstat(link_path, &sb), "lstat %s", link_path);

	len = sb.st_size + 1;
	app_path = malloc(len);
	check(app_path, "malloc failure");

	check(readlink(link_path, app_path, len) > 0,
			"readlink %s", link_path)
	app_path[len] = '\0';
	return;

error:
	if (app_path) {
		free(app_path);
		app_path = NULL;
	}
}

/* sabari
void update_con_tbl_tail(struct connection *conn) {
   pthread_mutex_lock( &mutex );
   con_tbl_tail = conn;
   pthread_mutex_unlock( &mutex );
}
*/

void * main_connection_manager(void *arg)
{
#ifdef DEBUG_CONMGR
	init_connection_table();

	printf("This is a debugging version, please use the following command to control connection manager:\n");
	printf("\ta <sockfd> <appID>\t-\tadd a connection\n");
	printf("\tc <sockfd> <child_sockfd>\t-\tadd a child connection\n");
	printf("\ts <sockfd> <status>\t-\tupdate the status of an existing connection specified by sockfd\n");
	printf("\td <sockfd> <appID>\t-\tdelete an existing connection specified by sockfd and appID\n");

	while (1) {
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
		case 'c':
			debug("child connection");
			add_child_connection(socket, appID);
			break;
		case 's':
		case 'u':
			debug("update status");
			update_connection_status(socket, appID);
			break;
		case 'r':
		case 'd':
			debug("delete connection");
			remove_connection(socket, appID);
		}
		dump_connections();
	}
#else
	int sock;
	struct sockaddr_un name;
	char path[PATH_BUF_SIZE];
	char buf[SOCK_BUF_SIZE];
	conmsg_t con_msg;

	sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	check(sock >= 0, "Failed to create UNIX domain datagram socket");
	name.sun_family = AF_UNIX;
	strcpy(name.sun_path, CONMGR_NAME);
	init_connection_table();
	check(!bind(sock, (struct sockaddr *) &name, sizeof(struct sockaddr_un)),
		"Failed to bind UNIX socket with name %s", CONMGR_NAME);

	/* just one socket, no need to select */
	//while (read(sock, buf, SOCK_BUF_SIZE)) {
	while (read(sock, &con_msg, sizeof(conmsg_t))) {
		debug("message received from pid %d", con_msg.pid);
		switch (con_msg.cmd) {
		case CONMGR_CMD_ADD:
			add_connection(con_msg.pid, con_msg.sockfd,
				(struct sockaddr *)&con_msg.addr,
				con_msg.addrlen);
			break;
		case CONMGR_CMD_REMOVE:
			remove_connection();
			break;
		default:
			log_warn("unknown command");
			break;
		}
	}

error:
	if (sock >= 0)
		close(sock);

#endif

	return NULL;
}
