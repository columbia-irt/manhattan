#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <memory.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define URL1 "https://sp.irtlab.org/secure/irtlab.html"
//#define URL2 "https://idp.irtlab.org/idp/profile/SAML2/Redirect/SSO?SAMLRequest=fZJdb4IwFIb%2FCum9FHAiNkLC9GImbhphu9jNUuBMmpSW9ZR9%2FPuhuE2Txcu278c5TzpH3siWpZ2t1Q7eOkDrfDZSITs%2BxKQzimmOApniDSCzJcvS%2BzULXI%2B1RltdakmcFBGMFVottMKuAZOBeRclPO7WMamtbZFRiq0rjJW8cLXZ06wWRaEl2NpF1PQQGtDtJsuJs%2BynEIof8v7corqw90fa978KCSfvDiphoLQ0yzbEWS1j8lKFEz%2BaVJEfwWQ65bPoppqNQy%2BM%2BuuiCse9DLGDlULLlY1J4PnByAtHnp%2F7EQs85kXPxNme1rwVqhJqf51JMYiQ3eX5djTs8wQGj7v0ApLMD2TZsdicsb4ey38Ak%2BR%2FnPiLc07PCoa2lj30iavlVktRfjmplPpjYYBbiIlPaDJYLr9B8g0%3D&amp;RelayState=ss%3Amem%3A362688277a44664abe93ac1acadb81aa"
#define USERPWD "myself:myself"
//#define WITH_BROWSER 0

int getAuth(CURL* curl, const char* url, char* html);	//return -1 if error
int postAuth(CURL* curl, char* html);	//return 1 if it works, 0 otherwise
void replace(char* str, const char* sub, const char* new_sub);

size_t store_html(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	//char* buf = (char*)userdata;
	//int old_len = strlen(buf);
	//int new_len = size * nmemb;
	//memcpy(buf + len, ptr, new_len);
	//buf[old_len + new_len] = '\0';
	strncat((char*)userdata, ptr, size * nmemb);
	return size * nmemb;
}

int getAuth(CURL* curl, const char* url, char* result)
{
	//CURL *curl;
	CURLcode res;
	//char cookie[2000] = {0};
	//char url[2000];

	//curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // just to start the cookie engine 
		curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);
		curl_easy_setopt(curl, CURLOPT_USERPWD, USERPWD);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, store_html);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);
		//curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
		{
			printf("Curl perform failed: %s\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
			return -1;
		}

		//printf("%s\n", result);	//debug
		curl_easy_cleanup(curl);

		//replace(result, "&#x3a;", ":");
		//replace(result, "&#x2f;", "/");
	}
	return 0;	//succeeded
}

void replace(char* str, const char* sub, const char* new_sub)
{
	int len_str = strlen(str) * 2;
	int len_sub = strlen(sub);
	char* start;
	char* last;
	char* new_str = (char*)malloc(len_str);

	start = strstr(str, sub);
	last = str;
	memset(new_str, 0, len_str);
	while (start != NULL)
	{
		//printf("%s\n", start);
		if (start > last)
		{
			strncat(new_str, last, start - last);
		}
		strcat(new_str, new_sub);
		//printf("%s\n", new_str);
		last = start + len_sub;
		//printf("%s\n", last);
		start = strstr(last, sub);
	}
	//printf("%s %s\n", new_str, last);	//debug
	if (*last != '\0')	//not reached the end
	{
		strcat(new_str, last);
	}

	strcpy(str, new_str);
	free(new_str);
}

int postAuth(CURL* curl, char* html)
{
	//CURL* curl;
	CURLcode res;

	char* posFormStart;
	char* posFormEnd;
	char* posInputStart;
	char* posInputEnd;
	char* posNameStart;
	char* posNameEnd;
	char* posValueStart;
	char* posValueEnd;
	char* posEnd;
	char url[1000] = {0};
	char postFields[100000] = {0};
	char result[100000] = {0};

	if (((posFormStart = strstr(html, "<form action=")) == NULL) ||
		((posFormEnd = strstr(html, "</form>")) == NULL))
	{
		return -1;
	}

	//copy the post url
	posValueStart = posFormStart + 14;	//right after the quote
	posValueEnd = strchr(posValueStart, '\"');
	if (posValueEnd == NULL || posValueEnd > posFormEnd)
	{
		return -1;
	}
	strncpy(url, posValueStart, posValueEnd - posValueStart);
	replace(url, "&#x3a;", ":");
	replace(url, "&#x2f;", "/");
	//printf("url: %s\n", url);

	//construct the post fields
	posInputEnd = posFormStart;	//starting from <form>
	posInputStart = strstr(posInputEnd, "<input");
	while (posInputStart != NULL)
	{
		posInputEnd = strchr(posInputStart, '>');
		posNameStart = strstr(posInputStart, "name=");
		posValueStart = strstr(posInputStart, "value=");
		if (posNameStart == NULL || posValueStart == NULL ||
			posNameStart > posInputEnd || posValueStart > posInputEnd)
		{	//this input doesn't have name or value
			//find the next input
			posInputStart = strstr(posInputEnd, "<input");
			continue;
		}
		
		//get the input name
		posNameStart += 6;
		posNameEnd = strchr(posNameStart, '\"');
		if (posNameEnd == NULL || posNameEnd > posValueStart)
		{
			posInputStart = strstr(posInputEnd, "<input");
			continue;
		}

		//get the input value
		posValueStart += 7;
		posValueEnd = strchr(posValueStart, '\"');
		if (posValueEnd == NULL || posValueEnd > posInputEnd)
		{
			posInputStart = strstr(posInputEnd, "<input");
			continue;
		}

		//add this entry to fields
		if (postFields[0] != 0)	//not the first one
		{
			strcat(postFields, "&");
		}
		strncat(postFields, posNameStart, posNameEnd - posNameStart);
		strcat(postFields, "=");
		strncpy(result, posValueStart, posValueEnd - posValueStart);
		replace(result, "&#x3a;", "\%3A");
		replace(result, "=", "\%3D");
		replace(result, "+", "\%2B");
		strcat(postFields, result);

		//find the next input
		posInputStart = strstr(posInputEnd, "<input");
	}
	//replace(postFields, "&#x3a;", "\%3A");
	//replace(postFields, "=", "\%3D");
	//replace(postFields, "+", "\%2B");
	//printf("here!\n");
	//printf("%s\n", postFields);	//debug
	result[0] = '\0';	//clear result

	//post
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // just to start the cookie engine 
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, store_html);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, result);
		if ((res = curl_easy_perform(curl)) != CURLE_OK)
		{
			printf("Curl perform failed: %s\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
			return -1;	
		}
		//printf("%s\n", result);
		curl_easy_cleanup(curl);
	}

	if (strstr(result, "It works!") == NULL)
	{
		return -1;	//authentication failed
	}
		
	return 0;	//succeeded
}

void add_fw_rules(const char* ifname)
{
/*
	system("sudo iptables -A INPUT -i hip0 -j DROP &> /dev/null");
	system("sudo iptables -A OUTPUT -o hip0 -j DROP &> /dev/null");
	system("sudo ip6tables -A INPUT -i hip0 -j DROP &> /dev/null");
	system("sudo ip6tables -A OUTPUT -o hip0 -j DROP &> /dev/null");
*/
}

void del_fw_rules(const char* ifname)
{
/*
	system("sudo iptables -D INPUT -i hip0 -j DROP &> /dev/null");
	system("sudo iptables -D OUTPUT -o hip0 -j DROP &> /dev/null");
	system("sudo ip6tables -D INPUT -i hip0 -j DROP &> /dev/null");
	system("sudo ip6tables -D OUTPUT -o hip0 -j DROP &> /dev/null");
*/
}

bool sine_shib_browser(char* ifname)
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

bool sine_shib(char* ifname)
{
	CURL* curl = curl_easy_init();
	char html[100000] = {0};

	printf("\nSecurity part: Shibboleth authentication.\n");
	printf("Block normal traffic\n");
	del_fw_rules(ifname);
	add_fw_rules(ifname);

	//printf("Username: myself\nPassword: myself\n");
	printf("\nStart to authenticate...\n");
	printf("Connect to Service Provider: 128.59.20.188\n");
	printf("Redirect to Identity Provider: 128.59.20.83\n");
	if (getAuth(curl, URL1, html) < 0)
	{
		printf("Failed to connect to Shibboleth Server!\n");
		return 0;
	}
	printf("Sending Credentials:\n");
	printf("Username: myself\nPassword: myself\n");
	//getWebpage(curl, URL2, html);
	if (postAuth(curl, html) < 0)
	{
		printf("Denied by Shibboleth Server!\n");
		return 0;
	}

	printf("Successfully authenticated! Allow traffic.\n\n");
	del_fw_rules(ifname);

	return 1;
}

//yan-start-10/04/2012
bool sine_hip_encrypt(int encrypt_enable)
{
        int sockfd;
        int n;
        int i;
        unsigned int len;
        struct sockaddr_in serv_addr;
        struct timeval tv;
        //odtone::mih::octet_string cmd;
        char buf[256];

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
        {
                printf("HIP: Failed to create socket\n");
                return -1;
        }

        tv.tv_sec = 10;
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                (char *)&tv, sizeof(struct timeval)) < 0)
        {
                printf("HIP: Failed to set timeout\n");
                return -1;
        }

        serv_addr.sin_family = AF_INET;
        inet_aton("127.0.0.1", &serv_addr.sin_addr);
        serv_addr.sin_port = htons(7776);
	if (encrypt_enable)
	{
        	n = sendto(sockfd, "eoff 0", 7, 0,
                	(struct sockaddr *)&serv_addr, sizeof(serv_addr));
	}
	else
	{
        	n = sendto(sockfd, "eoff 1", 7, 0,
                	(struct sockaddr *)&serv_addr, sizeof(serv_addr));
	}
        if (n < 0)
        {
                printf("HIP: Failed to send command via socket\n");
                return -1;
        }

        len = sizeof(serv_addr);
        n = recvfrom(sockfd, buf, 255, 0, (struct sockaddr *)&serv_addr, &len);
        if (n < 0)
        {
                printf(0, "HIP daemon not responding\n");
                return -1;
        }
        for (i = 0; i < n && buf[i] != '\n'; ++i);
        buf[i] = 0;
        printf("HIP: %s\n", buf);

        close(sockfd);
        return 0;
}

