#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>

#include "sined.h"
#include "netmgr.h"
#include "conmgr.h"
#include "locmgr.h"
#include "pe.h"

int main()
{
	pthread_t thread_netmgr;
	pthread_t thread_conmgr;
	pthread_t thread_locmgr;
	pthread_t thread_pe;

	parse_policies(POLICY_PATH);

	//openlog("sine", LOG_PID | LOG_PERROR, LOG_DAEMON);
	pthread_create(&thread_netmgr, NULL, main_network_manager, NULL);
	pthread_create(&thread_conmgr, NULL, main_connection_manager, NULL);
	pthread_create(&thread_locmgr, NULL, main_location_manager, NULL);
	pthread_create(&thread_pe, NULL, main_policy_engine, NULL);

	//syslog(LOG_INFO, "hello world!\n");
	log_info("hello world!");

	pthread_join(thread_netmgr, NULL);
	pthread_join(thread_conmgr, NULL);
	pthread_join(thread_locmgr, NULL);
	pthread_join(thread_pe, NULL);
	//closelog();

	return 0;
}
