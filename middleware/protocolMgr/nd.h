#ifndef NDEV
#define NDEV
  //huge hack by costin
#define MAX_IFNAME_LEN 7
char* sprint_ip(int ip);

struct net_device
{
  char name[MAX_IFNAME_LEN+1];
  struct net_device *next;

  int   ifindex;
  int mtu;               /* interface MTU value */
  short hard_header_len; /* hardware header length */
  char dev_addr[6];
  char addr_len;
  // XXX only used in a test for NULL -- might be able to fake                                                                                                                             
  int (*h)();
  /*int                 (*hard_header) (struct sk_buff *skb,
				      struct net_device *dev,
				      unsigned short type,
				      void *daddr,
				      void *saddr,
				      unsigned len);
  */
  /*                                                                                                                                                                                       
   * NEW FIELDS FOLLOW...                                                                                                                                                                  
   */
  int   ip_addr;           /* layer violation! never mind :) */
  struct rtable *rt_list;
};


#endif
