#ifndef COMMON_H
#define COMMON_H


char* policyModel(int appID, int sockfd);
void sine_policyListener();
void sine_appRegister(int portno);
void policyListenerController();
void sine_policyModelController(int appID);
char* sine_getIfStatus(const char* ifname);
int sine_getCost(const char* ifname);
int sine_getBandwidth(const char* ifname);
void sine_setHipIf(int sockfd, const char* ifname);
int sine_getsockfdCM();
int sine_getappIDCM();

#endif
