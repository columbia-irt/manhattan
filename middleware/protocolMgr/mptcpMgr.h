
int mptcp_socket (int af, int type, int protocol, int port);
int mptcp_bind (int s, const void *addr, int  addrlen, char * ifNames[], int ifCount, int offset);
int mptcp_send(int s, void *msg, int len, int flags);
int mptcp_connect(int s, void *addr, int  addrlen);
int mptcp_getsockopt (int s, int  level, int optname, void *optval, int *optlen);
int mptcp_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int mptcp_shutdown(int s, int how);
int mptcp_close(int s);
int mptcp_listen(int s, int backlog);
int mptcp_recv(int s, void *buf, int len, int flags) ;
int mptcp_accept(int s, void *addr, int *addrlen);
int mptcp_setsockopt (int         s,
                     int         level,
                     int         optname,
                     void       *optval,
                     int         optlen);

void mptcp_kill(void);
int mptcp_fcntl      (int fd);
