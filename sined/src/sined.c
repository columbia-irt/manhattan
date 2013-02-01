#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>

#include "dbg.h"
#include "sined.h"
#include "netmgr.h"
#include "conmgr.h"

int main()
{
	pthread_t thread_netmgr;
	pthread_t thread_conmgr;

	//openlog("sine", LOG_PID | LOG_PERROR, LOG_DAEMON);
	pthread_create(&thread_netmgr, NULL, main_network_manager, NULL);
	pthread_create(&thread_netmgr, NULL, main_connection_manager, NULL);

	//syslog(LOG_INFO, "hello world!\n");
	log_info("hello world!");

	pthread_join(thread_netmgr, NULL);
	pthread_join(thread_conmgr, NULL);
	//closelog();

	return 0;
}
