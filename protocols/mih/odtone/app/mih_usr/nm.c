/*
 * Network Manager API
 * @author: Yan Zou
 * @date: Apr 16, 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "nm.h"

char* ipc_nm(char* buf)
{
	int sockfd;
	struct sockaddr_in nm_addr;
	struct timeval tv;
	unsigned int addrlen;
	unsigned int buflen;

	//char* buf = malloc(20);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("failed to create datagram socket");
		buf[0] = '\0';
		return NULL;
	}

	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&tv, sizeof(struct timeval));

	nm_addr.sin_family = AF_INET;
	//serv_addr.sin_addr.s_addr = ;
	inet_aton("127.0.0.1", &nm_addr.sin_addr);
	nm_addr.sin_port = htons(18752);
	addrlen = sizeof(nm_addr);

	sendto(sockfd, buf, strlen(buf) + 1, 0,
		(struct sockaddr *)&nm_addr, addrlen);

	if ((buflen = recvfrom(sockfd, buf, 1000, 0,
		(struct sockaddr *)&nm_addr, &addrlen)) < 0)
	{
		perror("failed to get response from Network Manager");
		buf[0] = '\0';
		close(sockfd);
		return NULL;
	}

	close(sockfd);
	buf[buflen] = '\0';
	return buf;
}

void sine_getIfStatus(const char* ifname, char* result)
{
	//char* buf = malloc(20);		//TODO: get rid of malloc
	sprintf(result, "e %s", ifname);
	if (ipc_nm(result) == NULL)
	{
		result[0] = '\0';
	}
}

int sine_getCost(const char* ifname)
{
        char buf[20];
        sprintf(buf, "c %s", ifname);
        if (ipc_nm(buf) == NULL)
	{
		return -1;
	}
	return atoi(buf);
}

int sine_getBandwidth(const char* ifname)
{
        char buf[20];
        sprintf(buf, "b %s", ifname);
        if (ipc_nm(buf) == NULL)
	{
		return -1;
	}
	return atoi(buf);
}

char* sine_getInfo(const char* ifname)
{
        char* buf = malloc(1024);		//TODO: get rid of malloc
        sprintf(buf, "i %s", ifname);
        return ipc_nm(buf);
}

void sine_setHipIf(int sockfd, const char* ifname)
{
        char buf[20];
        sprintf(buf, "h %s", ifname);
        ipc_nm(buf);
}

void sine_setMipIf(int sockfd, const char* ifname)
{
        char buf[20];
        sprintf(buf, "m %s", ifname);
        ipc_nm(buf);
}

//* for test
int main()
{
	char buf[21];
	char result[20];
	char* r;
	scanf("%s", result);
	printf("%s\n", result);
	scanf("%s", buf);
	printf("%s\n", buf);

	switch(result[0])
	{
	case 'e':
		sine_getIfStatus(buf, result);
		printf("%s\n", result);
		break;
	case 'i':
		r = sine_getInfo(buf);
		printf("%s\n", r);
		free(r);
		break;
	case 'c':
		printf("%d\n", sine_getCost(buf));
		break;
	case 'b':
		printf("%d\n", sine_getBandwidth(buf));
		break;
	case 'h':
		sine_setHipIf(0, buf);
		break;
	case 'm':
		sine_setMipIf(0, buf);
		break;
	default:
		break;
	}
	return 0;
}

