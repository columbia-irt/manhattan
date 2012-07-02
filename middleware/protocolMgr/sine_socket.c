#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
//#include <socks.h>
#include "connectionMgr.h"
#include "policy.h"

/* ToDo : Cleanup Variables */

/* Global State Vars */
enum connection_status status;
int policy_engine_start=0;

int sine_socket (int af, int type, int protocol, int app_guid){
    
    int fd = socket(af, type, protocol);

    //ToDo : Should happen only once per sine lifetime
    init_connection_tbl();
    //ToDo : Not the right place to start Policy engine
    if(!policy_engine_start)    
	init_policy_engine();
    if(fd > -1) {
    printf("Adding NEW Socfd %d\n", fd);
    add_connection(fd, app_guid);
    update_connection_status(fd, NEW);
    }
    return fd;
}

int sine_bind (int fd, const void *addr, int  addrlen) {
    update_connection_bind(fd, addr, addrlen);
    return bind(fd, addr, addrlen);
}

int sine_send(int s, void *msg, int len, int flags) {
	update_connection_status(s, SEND);
 	return	send(s, msg, len, flags);
}

int sine_connect(int s, void *addr, int  addrlen) {
	update_connection_status(s, CONNECT);
	return connect(s, addr, addrlen);
}

int sine_getsockopt (int s, int  level, int optname, void *optval, int *optlen) {
	return getsockopt(s, level, optname, optval, optlen);
}

int sine_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout){
	return select(n, readfds, writefds, exceptfds, timeout);
}
int sine_shutdown(int s, int how){ 
	update_connection_status(s, FIN);
	return shutdown(s,how);
}

int sine_close(int s) {
	update_connection_status(s, FIN);
	return close(s);
}

int sine_listen(int s, int backlog){
	update_connection_status(s, LISTEN);
	return listen(s, backlog);
}

int sine_recv(int s, void *buf, int len, int flags) {
	update_connection_status(s, RECV);
	return recv(s, buf, len, flags);
}

int sine_accept(int s, void *addr, int *addrlen){
	int new_fd = accept(s, addr, addrlen);
	if(new_fd > -1) {
    		printf("Adding Child Socfd %d for parent %d\n", new_fd, s);
		add_child_connection(s, new_fd);
	}
	return new_fd;
}

int sine_setsockopt (int         s,
                     int         level,
                     int         optname,
                     void       *optval,
                     int         optlen){
	return setsockopt(s, level, optname, optval, optlen);
}

void sine_kill(void) {
//	mptcp_kill();
}

int sine_fcntl(int fd, int cmd, int arg){
	return fcntl(fd, cmd, arg);
}

int sine_read   (int s, void       *buf, size_t nbyte) {
	update_connection_status(s, RECV);
	return read(s,buf, nbyte);
}

int sine_write(int s, const void *buf, size_t nbyte) {
	update_connection_status(s, SEND);
	return write(s,buf,nbyte);
}


void *policy_thread (void *arg)
{
	sine_policyListener();
}

void init_policy_engine(){
	
	pthread_t policyListener;
	policy_engine_start = 1;
	pthread_create (&policyListener, NULL, policy_thread, NULL);
	printf("Listener Thread created\n");
}


