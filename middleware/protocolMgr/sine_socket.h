
int sine_socket (int af, int type, int protocol, int app_guid);
int sine_bind (int s, const void *addr, int  addrlen);
int sine_send(int s, void *msg, int len, int flags);
int sine_connect(int s, void *addr, int  addrlen);
int sine_getsockopt (int s, int  level, int optname, void *optval, int *optlen);
int sine_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int sine_shutdown(int s, int how);
int sine_close(int s);
int sine_listen(int s, int backlog);
int sine_recv(int s, void *buf, int len, int flags) ;
int sine_accept(int s, void *addr, int *addrlen);
int sine_setsockopt (int         s,
                     int         level,
                     int         optname,
                     void       *optval,
                     int         optlen);

void sine_kill(void);
int sine_fcntl      (int fd);
void init_policy_engine();
