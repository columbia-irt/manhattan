#include <iostream>
#include "policy.h"

using namespace std;

int sine_getBandwidth(const char*interface)
{
	return 450;
}

int sine_getCost(const char*interface)
{
	return 1;
}

void sine_setHipIf(int sockfd, const char* ifname)
{

}

void sine_getIfStatus(const char* ifname, char* result)
{
	//return "up";
}

void sine_appRegister(int portno)
{

}
