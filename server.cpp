/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

void* auth_status(void* arg) {
    int old_val, new_val;
	int newsockfd;
	char cmd[256];
	char buf[256];
	FILE *fp;

	newsockfd = (int)arg;

	cout << "New authentication starts " << newsockfd << endl;
	sprintf(cmd, "cat /opt/shibboleth-sp/var/log/shibboleth/shibd.log | grep \"new session created:\" | wc -l");
    	fp = popen(cmd, "r");
	if (fp < 0) {
		cout << "There is problem reading the log" << endl;
	}
	fgets(buf, 255, fp);
	old_val = atoi(buf);
	cout << "Old value is " << old_val << endl;
	fclose(fp);

    //This loop run for 10seconds to check for the new value (authentication)
    //for (j=0;j < 100; j++) {
        cout << "Waiting for authentication ends" << endl;
        //sleep(2);
	buf[0] = '\0';
	while (recv(newsockfd, buf, 255, 0) == 0);
	//printf("recv: %s\n", buf);
	//recv(newsockfd, buf, 255, 0);
	printf("recv: %s\n", buf);
        fp = popen(cmd, "r");
        
        if (fp < 0) {
		cout << "There is problem reading the log" << endl;
                return NULL;
        }
	fgets(buf, 255, fp);
	new_val = atoi(buf);
	cout << "New value is " << new_val << endl;
        fclose(fp);

        if(new_val > old_val) {
		buf[0] = '1';
        } else {
		buf[0] = '0';
	}
	buf[1] = '\0';
	printf("send: %s\n", buf);
	if (send(newsockfd, buf, 2, 0) < 0) {
		cout << "ERROR when sending back response" << endl;
	}
    //}
    close(newsockfd);

    return NULL;
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     int cur_lc;
	pthread_t thread_auth;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
	 while(1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
		if (newsockfd < 0) {
          		error("ERROR on accept");
			continue;
		}
		  //call auth status function
		//cout << "The new port sockfd is " << newsockfd << endl;
		pthread_create(&thread_auth, NULL, auth_status, (void *)newsockfd);
	}
     error("Closing Socket\n");
     close(sockfd);
     return 0; 
}

