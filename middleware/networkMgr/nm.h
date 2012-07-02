/*
 * Network Manager API
 * @author: Yan Zou
 * @date: Apr 16, 2012
 */
#ifndef _SINE_NM_H_
#define _SINE_NM_H_

//
void sine_getIfStatus(const char* ifname, char* result);

//
int sine_getCost(const char* ifname);

//
int sine_getBandwidth(const char* ifname);

//
char* sine_getInfo(const char* ifname);

//
void sine_setHipIf(int sockfd, const char* ifname);

//
void sine_setMipIf(int sockfd, const char* ifname);

//
bool sine_appRegister(int sockfd);

//
bool sine_appDeregister(int sockfd);

#endif
