
/* A wee progam to test the spangling sockets. */

/* #define _P __P */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include "defines.h"
#include "libusernet.h"
#include <time.h>
#include "nd.h"
#include "policy.h"
#include "connectionMgr.h"
#define MAX_CONNS 300
#define SOCKBUF 400000 //(64*1024)

#ifdef NO_THREADS
extern void *my_upcall_worker(void *arg); 
#endif

int my_port; 
#ifdef USER_STACK
    pthread_t threads[MAX_CONNS];
#endif
    int       i;
    int ii; 
    int nd,fd; 
    int rr, ss; 
    char * intF[10];

extern struct net_device** ultcp_interface;
extern char ultcp_cnt;

int mptcp_socket (int af, int type, int protocol, int port){
    my_port = port;	
    user_init();
    fd = user_socket(af, type, protocol);
    //if(port != 9001)
	    user_fcntl(fd, F_SETFL, O_NONBLOCK);
    //Update Connection Manager
    //Ideally should be with SINE Net API
    //To be moved
    init_connection_tbl();    
    add_connection(fd, 1000);
    return fd;
}

int mptcp_bind (int fd, const void *addr, int  addrlen, char * ifNames[], int ifCount, int offset) {
 //Bind with interfaces obtained from policy settings

 for (i=offset;i<ifCount;i++){
      struct sockaddr_in in_addr;
      int j;
      //intF[0] = malloc (sizeof(char) * 10);
      //intF[1] = malloc (sizeof(char) * 10);
      //printf("Here\n");
      //policy(123);
      //printf("Here\n");
      printf("Inames is %s\n", ifNames[i]);
      for (j=0;j<ultcp_cnt;j++)
        if (!strcmp(ifNames[i],ultcp_interface[j]->name))
          break;

      if (j==ultcp_cnt){
        printf("Can't find interface %s\n",ifNames[i]);
        exit(1);
      }

      in_addr.sin_port = htons(my_port);
      in_addr.sin_family = AF_INET;
      in_addr.sin_addr.s_addr = ultcp_interface[j]->ip_addr;
      //bind!
      if (user_bind(fd,(struct sockaddr*)&in_addr,sizeof(struct sockaddr_in))<0){
        printf("Bind error!");exit(1);
      }
    }
    return fd;
}

int mptcp_send(int s, void *msg, int len, int flags) {
	return user_send(s, msg, len, flags);
}

int mptcp_connect(int s, void *addr, int  addrlen) {
	return user_connect(s, addr, addrlen);
}

int mptcp_getsockopt (int s, int  level, int optname, void *optval, int *optlen) {
	return user_getsockopt(s, level, optname, optval, optlen);
}

int mptcp_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout){
	return user_select(n, readfds, writefds, exceptfds, timeout);
}
int mptcp_shutdown(int s, int how){ 
	return user_shutdown(s,how);
}

int mptcp_close(int s) {
	return user_close(s);
}

int mptcp_listen(int s, int backlog){
	return user_listen(s, backlog);
}

int mptcp_recv(int s, void *buf, int len, int flags) {
	return user_recv(s, buf, len, flags);
}

int mptcp_accept(int s, void *addr, int *addrlen){
	return user_accept(s, addr, addrlen);
}

int mptcp_setsockopt (int         s,
                     int         level,
                     int         optname,
                     void       *optval,
                     int         optlen){
	return user_setsockopt(s, level, optname, optval, optlen);
}

void mptcp_kill(void) {
	user_kill();
}

int mptcp_fcntl      (int fd, int cmd, int arg){
	return user_fcntl(fd, F_SETFL, O_NONBLOCK);
}

