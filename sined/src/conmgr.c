#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "dbg.h"
#include "conmgr.h"

#define NEW	0
#define SEND	1
#define RECV	2
#define CONNECT	3
#define LISTEN	4
#define FIN	5

struct connection *con_tbl_head = NULL;
struct connection *con_tbl_tail = NULL;
pthread_mutex_t con_tbl_mutex;		//sabari
//fpos_t pos;				//sabari

#ifndef NDEBUG
static const char *str_status[FIN+1] = {"NEW", "SEND", "RECV", "CONNECT" , "LISTEN", "FIN"};

static inline const char * convert_status(int status) {
	if (status < 0 || status > FIN)
		return "UNKNOWN";
	else
		return str_status[status];
}

static inline void print_connections() {
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
	debug("------------------------");
	debug("Printing Connection Info");
	debug("------------------------");
	debug("Connection: FD\t STAT\t APPID");
	while(conn != NULL) {
		debug("Connection: %d\t %s\t %d", conn->sockfd, convert_status(conn->status), conn->appID);
		conn = conn->next;
	} 	
	debug("------------------------");
	/*	sabari
	if (fsetpos(file,&pos) == -1) 
	{ 
		fprintf(file,"Unable to refresh screen\n");
		fclose(file);
	}
	fclose(file); 
	*/
}
#else
static inline void print_connections() { }
#endif

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

static int add_connection(int sockfd, int appID){
	struct connection *conn;

	conn = malloc(sizeof(struct connection));
	check(conn, "Failed to allocate space for new connection record (%s)", strerror(errno));
	memset(conn, 0, sizeof(struct connection));

	conn->sockfd = sockfd;
	conn->parent_sockfd = 0;
	conn->appID = appID;
	conn->policyID = 0;
	conn->status = NEW;
	conn->next = NULL;

	pthread_mutex_lock(&con_tbl_mutex);
	con_tbl_tail->next = conn;
	con_tbl_tail = conn;
	pthread_mutex_unlock(&con_tbl_mutex);

	//print_connections();
	return 0;

error:
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
	conn->status = NEW;
	conn->next = NULL;
	//update_con_tbl_tail(conn);
	//print_connections();
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

	//print_connections();
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
	//print_connections();
	return 1;

}

static void remove_connection(int sockfd, int appID) {
	struct connection *pred_conn = con_tbl_head;
	struct connection *curr_conn = pred_conn->next;

	while (curr_conn != NULL) {
		if (curr_conn->sockfd == sockfd && curr_conn->appID == appID) {
			pred_conn->next = curr_conn->next;
			free(curr_conn);
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

/* sabari
void update_con_tbl_tail(struct connection *conn) {
   pthread_mutex_lock( &mutex );
   con_tbl_tail = conn;
   pthread_mutex_unlock( &mutex );
}
*/

void * main_connection_manager(void *arg)
{
	char choice;
	int socket;
	int appID;

	init_connection_table();

	printf("This is a debugging version, please use the following command to control connection manager:\n");
	printf("\ta <sockfd> <appID>\t-\tadd a connection\n");
	printf("\tc <sockfd> <child_sockfd>\t-\tadd a child connection\n");
	printf("\ts <sockfd> <status>\t-\tupdate the status of an existing connection specified by sockfd\n");
	printf("\td <sockfd> <appID>\t-\tdelete an existing connection specified by sockfd and appID\n");

	while (1) {
		scanf("%c %d %d", &choice, &socket, &appID);
		debug("%c %d %d\n", choice, socket, appID);
		switch (choice) {
		case 'a':
		case 'n':
			debug("new connection");
			add_connection(socket, appID);
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
		print_connections();
	}

	return NULL;
}
