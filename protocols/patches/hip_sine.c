//yan-begin
//#include <hip/hip_globals.h>

//volatile int encrypt_off = 0;   //06/18/2012 - control encryption

void hip_sine_cleanup()
{
	int r;

	//if (pref_thread >= 0) pthread_cancel(pref_thread);	//for hip_sine_init

        r = system("sudo ip rule del table 200");       //hip
        r = system("sudo ip rule del table 201");       //eth
        r = system("sudo ip rule del table 202");       //wlan
        r = system("sudo ip rule del table 203");       //ppp
        r = system("sudo ip route flush table 200");
        r = system("sudo ip route flush table 201");
        r = system("sudo ip route flush table 202");
        r = system("sudo ip route flush table 203");

        r = system("sudo ip -6 rule del table 200");       //hip
        r = system("sudo ip -6 rule del table 201");       //eth
        r = system("sudo ip -6 rule del table 202");       //wlan
        r = system("sudo ip -6 rule del table 204");       //sixxs
        r = system("sudo ip -6 rule del table 205");       //he-ipv6
        r = system("sudo ip -6 route flush table 200");
        r = system("sudo ip -6 route flush table 201");
        r = system("sudo ip -6 route flush table 202");
        r = system("sudo ip -6 route flush table 204");
        r = system("sudo ip -6 route flush table 205");
}

void hip_sine_init()
{
	int r;
	char cmd[100];

	hi_node* my_host_id = get_preferred_hi(my_hi_head);

	hip_sine_cleanup();

	r = system("sudo ip rule add table 200 pref 200");
	r = system("sudo ip rule add table 201 pref 201");
	r = system("sudo ip rule add table 202 pref 202");
	r = system("sudo ip rule add table 203 pref 203");

	r = system("sudo ip -6 rule add table 200 pref 200");
	r = system("sudo ip -6 rule add table 201 pref 201");
	r = system("sudo ip -6 rule add table 202 pref 202");
	r = system("sudo ip -6 rule add table 204 pref 204");
	r = system("sudo ip -6 rule add table 205 pref 205");

	//r = system("sudo ip route add table 200 1.0.0.0/8 dev hip0 proto kernel scope link src 1.11.30.38");
	sprintf(cmd, "sudo ip route add table hip0 1.0.0.0/8 dev hip0 proto kernel scope link src %s",
			logaddr(SA(&my_host_id->lsi)));
	r = system(cmd);
	sprintf(cmd, "sudo ip -6 route add table hip0 2001:10::/28 dev hip0 proto kernel scope link");
	r = system(cmd);
}

void hip_handoff(sockaddr_list *l)
{
	pthread_mutex_lock(&pref_mutex);
	if (prefer_changed)     //this second prefer_changed is used to avoid multithread conflict
	{
		prefer_changed = 0;

		sockaddr_list *l2;
		for (l2 = my_addr_head; l2; l2=l2->next)
		{
		        l2->preferred=FALSE;
		}

		struct sockaddr *oldaddr;
		struct sockaddr *newaddr;
		hip_assoc *hip_a;
		int i;
		//char cmd[256];

		newaddr=SA(&l->addr);   //l is selected by the 

		//added, change the routing table here
		//sprintf(cmd, "sudo ip route add table %s 128.59.20.0/24 dev %s src %s",
		//      cmd, hip_preferred_ifname, hip_preferred_ifname, logaddr(newaddr))
		//log_(NORM, "route change: %s", cmd);
		//i = system(cmd);

		for (i = 0; i < max_hip_assoc; i++)
		{
		        hip_a = &hip_assoc_table[i];
		        oldaddr = HIPA_SRC(hip_a);
		        if (oldaddr->sa_family == AF_INET) {
		                ((struct sockaddr_in*)newaddr)->sin_port =
					((struct sockaddr_in*)oldaddr)->sin_port;
		        }

			readdress_association(hip_a, newaddr, l->if_index);
		}

		l->preferred = TRUE;
	}
	pthread_mutex_unlock(&pref_mutex);
}

//yan - start- 04/09/2012
// for network manager to change the preferred address of hip
//extern char hip_preferred_ifname[20];	
void *hip_pref_listener(void *arg)
{
	int sockfd;
	struct sockaddr_in serv_addr, cli_addr;

	fd_set rfds;
	int len;
	unsigned int addrlen = sizeof(cli_addr);
	char buf[256];
	char result[256];

	//revert 4 lines
	sockaddr_list *l;
	sockaddr_list *l_new = NULL;
	int preferred_iface_index;
	__u32 ip;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		log_(ERR, "Failed to create datagram socket\n");
		return NULL;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(7776);	//TODO: eliminate the hard code

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		log_(ERR, "Failed to bind socket\n");
		return NULL;
	}

	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);

	while (1)
	{
		if (select(sockfd + 1, &rfds, 0, 0, 0) < 0)
		{
			log_(ERR, "Failed to select\n");
			break;
		}

		if (FD_ISSET(sockfd, &rfds))
		{
			//log_(NORM, "Control message received\n");
			if ((len = recvfrom(sockfd, buf, 255, 0, (struct sockaddr *)&cli_addr, &addrlen)) < 6
				|| buf[4] != ' ')
			{
				sprintf(result, "Ignore bad message.");
			}
			else
			{
				buf[4] = 0;	//divide the message into two parts
				if (strcmp(buf, "pref") == 0)
				{
					log_(NORM, "Preference message received.\n");
					if (strcmp(buf + 5, hip_preferred_ifname) == 0)
					{
						sprintf(result, "Already on %s", hip_preferred_ifname);
						pthread_mutex_lock(&pref_mutex); 
						prefer_changed = 1;
						pthread_mutex_unlock(&pref_mutex);
					}
					else
					{
						char cmd[1000] = {0};
						int r;
						//sprintf(cmd, "sudo ip route change default dev %s", buf + 5);
						//sprintf(result, "Interface changed to %s", buf + 5);
						//r = system(cmd);
					
						if (hip_current_ifname[0] != 0)
						{
						
							//sprintf(cmd, "sudo ip -%d route del table %s %s",
							//	IP_VERSION, hip_preferred_ifname, DEST_ADDR);
							sprintf(cmd, "sudo ip -%d route flush table %s",
								IP_VERSION, hip_preferred_ifname);
							//log_(NORM, "change routing table: %s\n", cmd);
							//r = system(cmd);
						}

						sprintf(hip_preferred_ifname, "%s", buf + 5);
						preferred_iface_index = devname_to_index(hip_preferred_ifname,
								 NULL);
						//int temp = devname_to_index("ppp0", NULL);
						//log_(NORM, "index: ppp0 %d current %d\n", temp, devname_to_index);

						//sprintf(cmd, "%ssudo ip route add table %s 128.59.20.0/24 dev %s",
						//		cmd, hip_preferred_ifname, hip_preferred_ifname);
						//log_(NORM, "change routing table %d: %s\n", preferred_iface_index, cmd);
						//r = system(cmd);	//move it to the last
	
						pthread_mutex_lock(&pref_mutex);	//yan - Apr 11 2012
						prefer_changed = 1;
	
						for (l = my_addr_head; l; l=l->next) {
						    //l->preferred = FALSE;	//refresh
						    //log_(NORM, "%s %d\n", logaddr(SA(&l->addr)), l->if_index);
						    if (preferred_iface_index == l->if_index) {
#if (IP_VERSION == 6)	
							if ((l->addr.ss_family != AF_INET6) ||
								(IN6_IS_ADDR_LINKLOCAL(SA2IP6(&l->addr)) ||
								 IN6_IS_ADDR_SITELOCAL(SA2IP6(&l->addr)) ||
								 IN6_IS_ADDR_MULTICAST(SA2IP6(&l->addr))))
								continue;
#else
							if (l->addr.ss_family != AF_INET)
								continue;
#endif	
							if (IN_LOOP(&l->addr))
								continue;
							ip = ((struct sockaddr_in*)&l->addr)->sin_addr.s_addr;
							if (IS_LSI32(ip))
								continue;
							l_new = l;
							//l->preferred = TRUE;	//set it after handover
				    			//log_(NORM, "%s selected as the ",logaddr(SA(&l->addr)));
				    			//log_(NORM, "preferred address (pref iface).\n");
						    }
						}

						if (l_new != NULL)
						{/*
							struct sockaddr *oldaddr;
				    			struct sockaddr *newaddr;
			    				hip_assoc *hip_a;
	
							log_(NORM, "I'm also trying to readdress\n");
				    			newaddr = SA(&l_new->addr);
	
				    			for (i = 0; i < max_hip_assoc; i++)
				    			{
				    				hip_a = &hip_assoc_table[i];
			    					oldaddr = HIPA_SRC(hip_a);
								log_(NORM, "Old address: %s\n", logaddr(SA(oldaddr)));
			    					if (oldaddr->sa_family == AF_INET) {
			    						((struct sockaddr_in*)newaddr)->sin_port =
				    						((struct sockaddr_in*)oldaddr)->sin_port;
				    				}
	
				    				readdress_association(hip_a, newaddr, l_new->if_index);
				    			}
	
							l_new->preferred = TRUE;*/
	
							if (cmd[0] != 0)
								strcat(cmd, " && ");
							sprintf(cmd, "%ssudo ip -%d route add table %s %s%s dev %s", // src %s",
									cmd, IP_VERSION, hip_preferred_ifname, DEST_ADDR,
									get_router_str(hip_preferred_ifname),
									hip_preferred_ifname);//, logaddr(SA(&l_new->addr)));
							sprintf(result, "Interface changed to %s", hip_preferred_ifname);
							//hipcfg_setUnderlayIpAddress(logaddr(SA(&l_new->addr)));
						}
						else
						{
							sprintf(result, "Interface %s not found!", hip_preferred_ifname);
							hip_preferred_ifname[0] = 0;
						}
						//pthread_mutex_unlock(&pref_mutex);	//yan - Apr 11 2012
	
						log_(NORM, "change routing table: %s\n", cmd);
#if IP_VERSION == 6
						sprintf(cmd, "%s && sudo ip route add table eth0 default dev eth0 && sudo ip route flush table eth0", cmd);	//trigger route change
#endif
						r = system(cmd);
						sprintf(hip_current_ifname, "%s", hip_preferred_ifname);
						pthread_mutex_unlock(&pref_mutex);	//yan - Apr 11 2012
					//*/
					}
				}
				else if (strcmp(buf, "eoff") == 0)
				{
					log_(NORM, "Encryptiong control message received.\n");
					encrypt_off = atoi(buf + 5);
					if (encrypt_off) {
						sprintf(result, "Encryption is turned off!!");
					} else {
						sprintf(result, "Encryption is turned on!!");
					}
				}
				else
				{
					sprintf(result, "Bad message format!");
				}
			}
			log_(NORM, "%s\n", result);

			if (sendto(sockfd, result, strlen(result), 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
			{
				log_(ERR, "Failed to send back result!");
			}
		}
	}

	close(sockfd);

	//std::cout << "end" << std::endl;
	return NULL;
}

//TODO: get router from netlink
char* get_router_str(const char* ifname)
{
	if (strcmp(ifname, "eth0") == 0) {
#if IP_VERSION == 4
		return " via 128.59.16.1";
#else
		return " via fe80::2d0:6ff:fe26:9c00";
#endif
	} else if (strcmp(ifname, "wlan0") == 0) {
#if IP_VERSION == 4
		return " via 192.168.1.1";
#else
		return " via fe80::21f:3bff:fe05:6d5b";
#endif
	} else if (strcmp(ifname, "sixxs") == 0) {
		return "";//" via 2001:4830:1100:1ef::1";
	} else if (strcmp(ifname, "he-ipv6") == 0) {
		return "";//" via 2001:470:1f06:20c::1";
	} else {
		return "";
	}
}

//yan-end

