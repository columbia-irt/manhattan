#ifndef _SINE_PE_H_
#define _SINE_PE_H_

#include "sined.h"

struct connection;

void parse_policies(const char *policy_path);

void trigger_policy_engine();

void evaluate(struct connection *conn);

int init_policy_engine(pthread_t *pe_thread_ptr);

#endif
