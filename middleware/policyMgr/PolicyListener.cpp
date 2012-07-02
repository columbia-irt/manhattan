#include <iostream>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "policy.h"
#include "nm.h"
#include "lm.h"

using namespace std;

extern "C" void sine_policyListener()
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n = 0;	//yan

     //sockfd = socket(AF_INET, SOCK_STREAM, 0);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);	//yan - Apr 19, 2012
     if (sockfd < 0) 
     {
	cout<<"ERROR opening socket\n";
     }
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = rand() % 64510 + 1024;
//     cout<<portno<<endl;

     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	{	////yan - Apr 19, 2012
		cout << "ERROR on binding";
	}
	
	sine_appRegister(sockfd);	// Registering the port with the NM
	sine_locRegister(sockfd);
	cout << "b" << endl;
     while(1)
     {
	//yan - begin - Apr 19, 2012
	if (n < 0) {cout << "ERROR reading from socket"; exit(0);}
	else policyListenerController();
     	//if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) { cout << "ERROR on binding"; }
	cout << "Listening for Network Events "<<endl;
	//listen(sockfd,5);
     	clilen = sizeof(cli_addr);
     	//newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	//cout << "Got a connection : Policy Listener";
     	//if (newsockfd < 0) {}//error("ERROR on accept");
        bzero(buffer,256);
        //n = read(newsockfd,buffer,255);
	n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr *) &cli_addr, &clilen);
        //if (n < 0) {cout<<"ERROR reading from socket"; exit(0);}
	//else	//else added
        //policyListenerController();
//	cout<<"Listening to the listener again\n";
        //close(newsockfd);
	//yan - end
     }
     close(sockfd);	
     return;
}
