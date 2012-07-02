/*
 * Network Manager API
 * @author: Yan Zou
 * @date: Apr 16, 2012
 */

#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#include "nm.h"

using namespace std;

char* ipc_lm(char* buf, int listener_sock)
{
	int sockfd;
	struct sockaddr_in nm_addr;
	struct timeval tv;
	unsigned int addrlen;
	unsigned int buflen;

	//char* buf = malloc(20);

	if (listener_sock < 0)	//register and deregister already have a sockfd
	{
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	}
	else
	{
		sockfd = listener_sock;
	}
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
	nm_addr.sin_port = htons(18753);	//hard code the port number
	addrlen = sizeof(nm_addr);

	sendto(sockfd, buf, strlen(buf) + 1, 0,
		(struct sockaddr *)&nm_addr, addrlen);

	if ((buflen = recvfrom(sockfd, buf, 1000, 0,
		(struct sockaddr *)&nm_addr, &addrlen)) < 0)
	{
		perror("failed to get response from Location Manager");
		buf[0] = '\0';
		close(sockfd);
		return NULL;
	}

	if (listener_sock < 0)
	{
		close(sockfd);
	}
	else //set timeout as infinity
	{
		tv.tv_sec = 0;
		tv.tv_sec = 0;
		setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
			(char *) &tv, sizeof(struct timeval));
	}

	buf[buflen] = '\0';
	return buf;
}

void sine_getLocation(char* result)
{
	//char* buf = malloc(20);		//TODO: get rid of malloc
	sprintf(result, "q");
	if (ipc_lm(result, -1) == NULL)
	{
		result[0] = '\0';
	}
}

bool sine_locRegister(int sockfd)
{
	char buf[20];
	//sprintf(buf, "r %d", portno);
	sprintf(buf, "r");
	ipc_lm(buf, sockfd);
	if (strcmp(buf, "ack") == 0)
	{
		return true;
	}
	return false;
}

bool sine_locDeregister(int sockfd)
{
	char buf[20];
	//sprintf(buf, "d %d", portno);
	sprintf(buf, "d");
	ipc_lm(buf, sockfd);
	if (strcmp(buf, "ack") == 0)
	{
		return true;
	}
	return false;
}

/* for test
int main()
{
	char buf[21];
	char result[20];
	char* r;
	int sockfd;

	scanf("%s", result);

	switch(result[0])
	{
	case 'q':
		sine_getLocation(result);
		break;
	case 'r':
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		sine_locRegister(sockfd);
		break;
	case 'd':
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		sine_locDeregister(sockfd);
		break;
	default:
		break;
	}

	printf("%s\n", result);

	return 0;
}
*/

