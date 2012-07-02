#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "connectionMgr.h"

struct connection ** connection_tbl = NULL;
struct connection *last_connection;
pthread_mutex_t mutex;
int init_connection_tbl_status = 0;
fpos_t pos; 
const char str_status[6][7] = {"NEW", "SEND", "RECV", "CONNECT" , "LISTEN", "FIN"};

void init_connection_tbl() {

 /* ToDo : Ideally connection_tbl should be initiallized only once */
 if(!init_connection_tbl_status) {
	connection_tbl = calloc(1, sizeof(struct connection *));
	connection_tbl[0] = calloc(1, sizeof(struct connection));
	connection_tbl[0]->next = NULL;
	last_connection = connection_tbl[0];
	if(!connection_tbl) {
		printf("Error init_connection_tbl");
		return;
	}
	printf("Memory Loction of table %u\n", connection_tbl);
	printf("Memory Loction of init record %u\n", connection_tbl[0]);
	init_connection_tbl_status = 1;
	printf("Size of connection : %d %d\n", sizeof(struct connection), sizeof(size_t));
	printf("------------------------------\n");
	printf("Initialized Connection Manager\n");
	printf("------------------------------\n\n\n");
 }

} 

void add_connection(int sockfd, int app_guid){
	struct connection *record;
	record = calloc(1, sizeof(struct connection));
	last_connection->next = record;
	if(!record) {
		printf("Error add_connection");
		return;
	}
	printf("Memory Loction is %u\n", record);
	record->sockfd = sockfd;
	record->parent_sockfd = 0;
	record->app_guid = app_guid;
	record->policy_guid = 0;
	record->status = NEW;
	record->next = NULL;
	update_last_connection(record);
	//print_connections();
} 

void add_child_connection(int sockfd, int new_sockfd){
	struct connection *record;
	record = calloc(1, sizeof(struct connection));
	last_connection->next = record;
	if(!record) {
		printf("Error add_connection");
		return;
	}
	printf("Memory Loction is %u\n", record);
	record->sockfd = new_sockfd;
	record->parent_sockfd = sockfd;
	record->app_guid = 31;
	record->policy_guid = 0;
	record->status = NEW;
	record->next = NULL;
	update_last_connection(record);
	//print_connections();
} 

int update_connection_bind(int sockfd, const void *addr, int addrlen){
	
	struct connection *node = connection_tbl[0]->next;
	while(node->next != NULL) {
		if (node->sockfd == sockfd) {
			node->addr = addr;
			node->addrlen = addrlen;
			return 0;	
		}
		node = node->next;
	} 	

	//print_connections();
	return 1;

}

int update_connection_status(int sockfd, enum connection_status status){
	
	struct connection *node = connection_tbl[0]->next;
	while(node->next != NULL) {
		if (node->sockfd == sockfd) {
			node->status = status;
			return 0;	
		}
		node = node->next;
	} 	
	//print_connections();
	return 1;

}

void print_connections() {
	struct connection *node = connection_tbl[0]->next;
	// get the current position 
	FILE *file = fopen("connections.out","w"); 
	fflush(file);
	if (fgetpos(file,&pos) == -1) 
	{ 
		fprintf(file,"Unable to refresh screen\n");
		fclose(file);
	} 
	fprintf(file,"------------------------\n");
	fprintf(file,"Printing Connection Info\n");
	fprintf(file,"------------------------\n");
	fprintf(file,"Connection: FD \t STAT \t APPID \n");
	while(node->next != NULL) {
		fprintf(file,"Connection: %d \t %s \t %d \n", node->sockfd,  convert_status(node->status), node->app_guid);
		node = node->next;
	} 	
	fprintf(file, "------------------------\n");
	if (fsetpos(file,&pos) == -1) 
	{ 
		fprintf(file,"Unable to refresh screen\n");
		fclose(file);
	}
	fclose(file); 
}

char * convert_status(enum connection_status status) {
	return str_status[status];
}

void update_last_connection(struct connection *record) {
   pthread_mutex_lock( &mutex );
   last_connection = record;
   pthread_mutex_unlock( &mutex );
}
