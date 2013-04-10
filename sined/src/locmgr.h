#ifndef _SINE_LOCMGR_H_
#define _SINE_LOCMGR_H_

#include "sined.h"

int locmgr_get_location(char *loc);

int init_location_manager(pthread_t *locmgr_thread_ptr);

#endif
