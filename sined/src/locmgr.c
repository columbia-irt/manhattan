#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "locmgr.h"
#include "conmgr.h"
#include "pe.h"

static pthread_mutex_t location_mutex = PTHREAD_MUTEX_INITIALIZER;
static char location[LOC_BUF_SIZE];

int locmgr_get_location(char *loc)
{
	check(loc, "The pointer passed in is NULL");
	pthread_mutex_lock(&location_mutex);
	strcpy(loc, location);
	pthread_mutex_unlock(&location_mutex);
	return 0;

error:
	return -1;
}

static void * locmgr_main_loop()
{
#ifdef DEBUG_CONMGR
	while (!exit_flag);
#else
	fd_set locmgr_fds;
	size_t nbytes = 0;
	char *loc = NULL;
	int max_fd;
	int i;

	max_fd = MAX(STDIN_FILENO, exit_pipe[0]);

	while (!exit_flag) {
		FD_ZERO(&locmgr_fds);
		FD_SET(STDIN_FILENO, &locmgr_fds);
		FD_SET(exit_pipe[0], &locmgr_fds);

		i = select(max_fd + 1, &locmgr_fds, NULL, NULL, NULL);
		check(i >= 0, "Select exception");
		debug("select returns %d", i);

		if (FD_ISSET(exit_pipe[0], &locmgr_fds))
			break;

		if (FD_ISSET(STDIN_FILENO, &locmgr_fds) &&
		    getline(&loc, &nbytes, stdin) > 0) {
			for (i = 0; loc[i] != '\n' && loc[i] != '\0'; ++i);
			loc[i] = '\0';

			if (!strcmp(loc, "l")) {
				conmgr_dump_connections();
				continue;
			}

			debug("input location is: %s", loc);
			pthread_mutex_lock(&location_mutex);
			strncpy(location, loc, LOC_BUF_SIZE);
			location[LOC_BUF_SIZE - 1] = '\0';
			pthread_mutex_unlock(&location_mutex);

			free(loc);
			loc = NULL;
			nbytes = 0;

			trigger_policy_engine();
		}
	}

error:
#endif

	log_info("location manager exiting...");
	return NULL;
}

int init_location_manager(pthread_t *locmgr_thread_ptr)
{
	strcpy(location, "columbia");
	check(!pthread_create(locmgr_thread_ptr, NULL, locmgr_main_loop, NULL),
		"Failed to create thread for location manager");

	log_info("Location Manager initialized");
	return 0;

error:
	return -1;
}
