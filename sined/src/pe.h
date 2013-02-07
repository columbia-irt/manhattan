#ifndef _SINE_PE_H_
#define _SINE_PE_H_

#include "sined.h"

void parse_policies(const char *policy_path);

void trigger_policy_engine();

void * main_policy_engine();

#endif
