#ifndef COMMON_H
#define COMMON_H

char* policyModel(int appID, int sockfd);
#ifdef __cplusplus
extern "C" void sine_policyListener();
#endif
void policyListenerController();

#define SINE_SHIBBOLETH 1
#define WITH_BROWSER 1

#endif
