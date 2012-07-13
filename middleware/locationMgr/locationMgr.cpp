#include <iostream>
#include <cstring>
#include <vector>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int sockfd;
std::string loc;
pthread_mutex_t loc_mutex = PTHREAD_MUTEX_INITIALIZER;

class client_info       //info of clients that wait for the information response
{
public:
        client_info() {}
        client_info(struct sockaddr_in addr, size_t addrlen)//, int appid)
                : cli_addr(addr), addr_len(addrlen)//, app_id(appid)
        {}
        //int get_app_id();
        int get_port();
        void send_msg(const char *msg);
private:
        //cli_addr needs to be copied into client_info
        //in case it is changed elsewhere or deleted after that function
        struct sockaddr_in cli_addr;
        size_t addr_len;
        //int app_id;   //the registration app_id
};

int client_info::get_port()
{
        return ntohs(cli_addr.sin_port);
}

void client_info::send_msg(const char *msg)
{
        std::cout << "Message sent to port " << get_port() << std::endl;
        if (sendto(sockfd, msg, strlen(msg) + 1, 0, (struct sockaddr *)&cli_addr, addr_len) < 0)
        {
                std::cout << "client_info failed to send msg" << std::endl;
        }
}

void *query_handler (void *arg)
{
	//int sockfd;
        struct sockaddr_in serv_addr, cli_addr;
	std::vector<client_info>* p_applist = (std::vector<client_info>*)arg;

        int len;
        unsigned int addrlen = sizeof(cli_addr);
        char buf[256];

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
        {
                std::cout << "failed to create datagram socket" << std::endl;
                return NULL;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(18753);	//hard code the port number

        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
                std::cout << "failed to bind socket" << std::endl;
                return NULL;
        }

        while (true)
        {
		if ((len = recvfrom(sockfd, buf, 255, 0, (struct sockaddr *)&cli_addr, &addrlen)) < 1)
		{
			std::cout << "Query handler: failed to receive msg" << std::endl;
			continue;
		}
		buf[len] = '\0';

		bool existed;
		client_info* p_client;
		int portno;

		switch (buf[0])
		{
		case 'r':
			existed = false;
			p_client = new client_info(cli_addr, addrlen);

			for (std::vector<client_info>::iterator it = p_applist->begin();
				it != p_applist->end(); ++it)
			{
				if (it->get_port() == p_client->get_port())
				{
					existed = true;
					break;
				}
			}

			if (existed)
			{
				std::cout << "Application (Listening port: " <<
					p_client->get_port() << ") has re-registered" << std::endl;
				if (sendto(sockfd, "nak", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
				{
					std::cout << "Query handler: failed to send nak" << std::endl;
				}
			}
			else
			{
				p_applist->push_back(*p_client);
				std::cout << "Application (Listening port: " <<
					p_client->get_port() << ") has registered" << std::endl;
				if (sendto(sockfd, "ack", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
				{
					std::cout << "Query handler: failed to send ack" << std::endl;
				}
			}

			break;
		case 'd':
			portno = ntohs(cli_addr.sin_port);
			for (std::vector<client_info>::iterator it = p_applist->begin();
				it != p_applist->end(); ++it)
			{
				if (it->get_port() == portno)
				{
					p_applist->erase(it);
					break;
				}
			}

			std::cout << "Application (Listening port: " <<
				portno << ") has de-registered" << std::endl;
			if (sendto(sockfd, "ack", 4, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
			{
				std::cout << "Query handler: failed to send ack" << std::endl;
			}
			break;
		case 'q':
			//if (len < 3)
			//	std::cout << "Query handler: bad msg format" << std::endl;
			//std::cout << "Query received" << std::endl;
			pthread_mutex_lock(&loc_mutex);/*
			if (strcmp(loc.c_str(), buf + 2) == 0)
			{
				buf[0] = '1';
				buf[1] = '\0';
			}
			else
			{
				buf[0] = '0';
				buf[1] = '\0';
			}*/
			//pthread_mutex_unlock(&loc_mutex);
			//if (sendto(sockfd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
			if (sendto(sockfd, loc.c_str(), loc.length() + 1, 0, (struct sockaddr *)&cli_addr, addrlen) < 0)
                        {
                        	std::cout << "Query handler: failed to send query response" << std::endl;
                        }
			pthread_mutex_unlock(&loc_mutex);
			break;
		default:
			std::cout << "Query handler: bad msg format" << std::endl;
			break;
		}
	}

	close(sockfd);
	return NULL;
}

int main()
{
	pthread_t query_thread;
	std::vector<client_info> applist;
	std::string input_buf;

	loc = "columbia";
	std::cout << "The current location is " << loc << std::endl;

	pthread_create(&query_thread, NULL, query_handler, (void *)&applist);
	sleep(1);
	std::cout << "Thread initiated" << std::endl;

	while (true)
	{
		std::cin >> input_buf;
		//std::cout << input_buf << std::endl;
		pthread_mutex_lock(&loc_mutex);
		loc = input_buf;
		std::cout << "The current location has been changed to " << loc << std::endl;
		pthread_mutex_unlock(&loc_mutex);

		//broadcast the change
		for (std::vector<client_info>::iterator it = applist.begin();
			it != applist.end(); ++it)
		{
			it->send_msg("");
			//std::cout << "Trigger port number " << it->get_port() << std::endl;
		}
	}

	return 0;
}
