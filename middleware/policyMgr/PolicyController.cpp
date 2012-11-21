/**
	PolicyController.cpp
	Purpose: Invokes the Policy Model
*/


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


/**
Invokes the Policy Model
@param - Application ID
@return
*/
void sine_policyModelController(int appID)
{
	int sockfd=0;
	char* interfaces=policyModel(appID, sockfd);
	if(interfaces==NULL)
        cout<<"No available interfaces\n";
    else
        cout<<"Connecting to "<<interfaces<<endl;
}

/**
API used by the Policy Listener to invoke Policy Engine when an event occurs
*/
void policyListenerController()
{
	int appID=35;
	sine_policyModelController(appID);
	appID = 36;	//yan - for MIPv6
	sine_policyModelController(appID);
}
