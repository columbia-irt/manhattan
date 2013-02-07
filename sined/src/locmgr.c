#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "locmgr.h"
#include "pe.h"

static pthread_mutex_t location_mutex;
static char location[LOC_BUF_SIZE];

int get_location(char *loc)
{
	check(loc, "The pointer passed in is NULL");
	pthread_mutex_lock(&location_mutex);
	strcpy(loc, location);
	pthread_mutex_unlock(&location_mutex);
	return 0;

error:
	return -1;
}

void * main_location_manager(void *arg)
{
#ifdef DEBUG_CONMGR
	while (1);
#else
	int i;
	size_t nbytes = 0;
	char *loc = NULL;

	while (1) {
		if (getline(&loc, &nbytes, stdin) > 0) {
			for (i = 0; loc[i] != '\n' && loc[i] != '\0'; ++i);
			loc[i] = '\0';
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
#endif
}
