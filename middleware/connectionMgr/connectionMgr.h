
enum connection_status {
   NEW,
   SEND,
   RECV,
   CONNECT,
   LISTEN,
   FIN
};

struct connection{
   int sockfd;
   int parent_sockfd;
   int app_guid;
   int policy_guid;
   enum connection_status status;
   socklen_t addrlen;
   struct sockaddr *addr;
   struct connection *next;
};

void init_connection_tbl();
int update_connection_bind(int sockfd, const void *addr, int addrlen);
int update_connection_status(int sockfd, enum connection_status status);
void add_connection(int sockfd, int app_guid);
void add_child_connection(int sockfd, int new_sockfd);
void print_connections();
char * convert_status(enum connection_status status);
