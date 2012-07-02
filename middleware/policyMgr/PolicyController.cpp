#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include "policy.h"
#include "nm.h"

using namespace std;

static char** interfaces;

void sine_policyModelController(int appID);


/*int main()
{
	sine_policyModelController(35);

}*/

void sine_policyModelController(int appID)
{
	int sockfd=0;
//	sockfd = sine_getsockfdCM();
	char* interfaces=policyModel(appID, sockfd);
	if(interfaces==NULL)
        cout<<"No available interfaces\n";
    else
        cout<<"Connecting to "<<interfaces<<endl;
}

void policyListenerController()
{
	int appID=35;
//	int appID=sine_getappIDCM();
	sine_policyModelController(appID);
	appID = 36;	//yan - for MIPv6
	sine_policyModelController(appID);
}
