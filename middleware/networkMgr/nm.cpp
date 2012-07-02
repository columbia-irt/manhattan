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

#include "nm.h"

using namespace std;

char* ipc_nm(char* buf, int listener_sock)
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

	tv.tv_sec = 100;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
		(char *)&tv, sizeof(struct timeval));

	nm_addr.sin_family = AF_INET;
	//serv_addr.sin_addr.s_addr = ;
	inet_aton("127.0.0.1", &nm_addr.sin_addr);
	nm_addr.sin_port = htons(18752);
	addrlen = sizeof(nm_addr);

	printf("Sending: %s\n", buf);
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

void sine_getIfStatus(const char* ifname, char* result)
{
	//char* buf = malloc(20);		//TODO: get rid of malloc
	printf("\nQuery MIH if interface %s exists.\n", ifname);
	sprintf(result, "e %s", ifname);
	if (ipc_nm(result, -1) == NULL)
	{
		result[0] = '\0';
	}
}

int sine_getCost(const char* ifname)
{
        char buf[20];
        sprintf(buf, "c %s", ifname);
        if (ipc_nm(buf, -1) == NULL || buf[0] == '\0')
	{
		return INT_MAX;
	}
	//printf("%d\n", (int)buf[0]);
	return atoi(buf);
}

int sine_getBandwidth(const char* ifname)
{
        char buf[20];
	printf("\nQuery the network information (cost & bandwidth) from MIH.\n");
        sprintf(buf, "b %s", ifname);
        if (ipc_nm(buf, -1) == NULL || buf[0] == '\0')
	{
		return -1;
	}
	return atoi(buf);
}

char* sine_getInfo(const char* ifname)
{
        char buf[20];	//TODO: get rid of malloc
        sprintf(buf, "i %s", ifname);
        return ipc_nm(buf, -1);
}

void sine_setHipIf(int sockfd, const char* ifname)
{
        char buf[20];
	printf("\nTell HIP to use interface %s.\n", ifname);
        sprintf(buf, "h %s", ifname);
        ipc_nm(buf, -1);
}

void sine_confirmHipIf(int sockfd, const char* ifname)
{
	char buf[20];
	sprintf(buf, "g %s", ifname);
	ipc_nm(buf, -1);
}

void sine_setMipIf(int sockfd, const char* ifname)
{
        char buf[20];
	printf("\nTell MIPv6 to use interface %s.\n", ifname);
        sprintf(buf, "m %s", ifname);
        ipc_nm(buf, -1);
}

bool sine_appRegister(int sockfd)
{
	char buf[20];
	//sprintf(buf, "r %d", portno);
	sprintf(buf, "r");
	ipc_nm(buf, sockfd);
	if (strcmp(buf, "ack") == 0)
	{
		return true;
	}
	return false;
}

bool sine_appDeregister(int sockfd)
{
	char buf[20];
	//sprintf(buf, "d %d", portno);
	sprintf(buf, "d");
	ipc_nm(buf, sockfd);
	if (strcmp(buf, "ack") == 0)
	{
		return true;
	}
	return false;
}
/*
void add_fw_rules(const char* ifname)
{
	char cmd[256];

	sprintf(cmd, "sudo iptables -A INPUT -p udp --sport 10500 -i %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -A INPUT -p tcp -s idp.irtlab.org --sport 443 -i %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -A INPUT -p tcp -s sp.irtlab.org --sport 443 -i %s -j ACCEPT", ifname);
	system(cmd);
        sprintf(cmd, "sudo iptables -A INPUT -p tcp -s sp.irtlab.org --sport 18754 -i %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -A INPUT -i %s -j DROP", ifname);
	system(cmd);

	sprintf(cmd, "sudo iptables -A OUTPUT -p udp --dport 10500 -o %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -A OUTPUT -p tcp -d idp.irtlab.org --dport 443 -o %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -A OUTPUT -p tcp -d sp.irtlab.org --dport 443 -o %s -j ACCEPT", ifname);
	system(cmd);
        sprintf(cmd, "sudo iptables -A OUTPUT -p tcp -d sp.irtlab.org --dport 18754 -o %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -A OUTPUT -o %s -j DROP", ifname);
	system(cmd);

	system("sudo iptables -A INPUT -i hip0 -j DROP");
	system("sudo iptables -A OUTPUT -o hip0 -j DROP");
}

void del_fw_rules(const char* ifname)
{
	char cmd[256];

	sprintf(cmd, "sudo iptables -D INPUT -i %s -j DROP", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -D INPUT -p tcp -s idp.irtlab.org --sport 443 -i %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -D INPUT -p tcp -s sp.irtlab.org --sport 443 -i %s -j ACCEPT", ifname);
	system(cmd);
        sprintf(cmd, "sudo iptables -D INPUT -p tcp -s sp.irtlab.org --sport 18754 -i %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -D INPUT -p udp --sport 10500 -i %s -j ACCEPT", ifname);
	system(cmd);

        sprintf(cmd, "sudo iptables -D OUTPUT -o %s -j DROP", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -D OUTPUT -p tcp -d idp.irtlab.org --dport 443 -o %s -j ACCEPT", ifname);
	system(cmd);
        sprintf(cmd, "sudo iptables -D OUTPUT -p tcp -d sp.irtlab.org --dport 443 -o %s -j ACCEPT", ifname);
	system(cmd);
        sprintf(cmd, "sudo iptables -D OUTPUT -p tcp -d sp.irtlab.org --dport 18754 -o %s -j ACCEPT", ifname);
	system(cmd);
	sprintf(cmd, "sudo iptables -D OUTPUT -p udp --dport 10500 -o %s -j ACCEPT", ifname);
	system(cmd);

	system("sudo iptables -D INPUT -i hip0 -j DROP");
	system("sudo iptables -D OUTPUT -o hip0 -j DROP");
}

bool sine_shibboleth(char* ifname)
{
	int sockfd;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	int len;
	char buf[2];
	struct timeval tv;
	int i;

	printf("Set iptables to only allow the authentication packets.\n");
	del_fw_rules(ifname);
	add_fw_rules(ifname);

	printf("Start chrome to authenticate...\n");
	for (i = 0; i < 3; ++i) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			printf("failed to create socket\n");
			continue;
		}

		tv.tv_sec = 10;
		tv.tv_usec = 0;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
			(char *) &tv, sizeof(struct timeval)) < 0) {
			printf("failed to set timeout\n");
		}

		bzero((char *)&serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		inet_aton("128.59.20.188", &serv_addr.sin_addr);
		serv_addr.sin_port = htons(18754);
		if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			printf("failed to connect shib server\n");
			close(sockfd);
			continue;
		}
		close(sockfd);

		system("ps -ef | grep chrome | grep -v grep | awk '{print $2}' | xargs kill -9");
		system("google-chrome https://sp.irtlab.org/secure/irtlab.html --app --incognito");
		
		printf("waiting for authentication response\n");
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			printf("failed to create socket\n");
			continue;
		}
		if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			printf("failed to connect shib server\n");
			close(sockfd);
			continue;
		}

		len = send(sockfd, "start", 6, 0);
		//printf("Wrote 6 bytes\n");
		if (len < 0) {
			printf("Failed to send authentication ending message\n");
			close(sockfd);
			continue;
		}

		len = recv(sockfd, buf, 255, 0);
		//printf("buf: %s\n", buf);
		if (buf[0] == '1') {
			printf("\nAuthenticated by Shib Deamon\n");
			close(sockfd);
			printf("Clear iptables to allow traffic.\n");
			del_fw_rules(ifname);
			return 1;
		}
		else {
			printf("\nDenied by Shib Deamon\n");
		}
		close(sockfd);
	}
	return 0;
}
*/

/* for test
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
*/

