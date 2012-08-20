#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include "nm.h"
#include "lm.h"
#include "sm.h"

#define SINE_SECURITY 0
#define WITH_BROWSER 0
#define WITH_MIPV6 1

using namespace std;

class Interface
{
    public:
    int interfaceEval(char* interface);
    int interfaceStatus(char* interface);
};

int Interface::interfaceEval(char*interface)
{
    char inter[20] = {0};
    int j=0;
    for(int i=0;i<=strlen(interface);i++)
    {
        if(interface[i]!=' ' && interface[i]!='\0')
        {
            inter[j]=interface[i];
            j++;
        }
        else
        {
            inter[j]='\0';
            int ret=interfaceStatus(inter);
            if(!ret)
                return 0;       // make it zero
	    
	    j=0;	
	/* 
	Should uncomment the below comments, if we have multiple interfaces
	and want to test shibboleth on each of them.        
	*/
	/*	if(!strcmp(inter,"wlan0"))
   		if(SINE_SHIBBOLETH && !sine_shib(inter))
			return 0;
	*/
        }
    }
    return 1;
}

int Interface::interfaceStatus(char* interface)
{
    cout<<"Checking interface "<<interface<<endl;
    char buffer[10] = {0};
   	sine_getIfStatus(interface, buffer);
	if(!strcmp(buffer, "up"))
		return 1;
    	else
    	return 0;

	// Below is for testing whether interface exists.
   /* 	if(!strcmp(interface,"wlan0"))
		return 1;
		else
		return 0;
   */
}

class Location
{
    public:
    int resolveLocation(char* loc);
    int locationStatus(char* loc);
};

int Location::resolveLocation(char* location)
{
    char loc[15] = {0};
    int j=0;
    for(int i=0;i<=strlen(location);i++)
    {
        if(location[i]!=' ' && location[i]!='\0')
        {
            loc[j]=location[i];
            j++;
        }
        else
        {
            //j++;
            loc[j]='\0';
		cout << "The current location is " << loc << endl;
            int ret=locationStatus(loc);
		//cout << "Back to resolveLocation" << endl;
            if(ret)
                return 1;       // make it zero
            j=0;
        }
    }
    return 0;
}

int Location::locationStatus(char* location)
{
    char buffer[100] = {0};
	sine_getLocation(buffer);
	cout << "The current location is " << buffer << endl;
	if(!strcmp(buffer, location))
	{
		cout << buffer << endl;
		return 1;
	}
    	else
	{
		cout << buffer << endl;
    		return 0;
	}

// Just to check the validity of the location manager.
/*    	if(!strcmp(location,"columbia"))
		return 1;
		else
		return 0;
*/
}



class Bandwidth
{
    public:
    int resolveBandwidth(const char* inter, int band);
};

int Bandwidth::resolveBandwidth(const char* inter, int band)
{
    int obtBand=510;
    //cout<<"Got - "<<inter<<endl;
    obtBand = sine_getBandwidth(inter);
    if(band<=obtBand)
        return 1;
    else
        return 0;
}

class Cost
{
    public:
    int resolveCost(const char* inter, int cost);
};

int Cost::resolveCost(const char* inter, int cost)
{
    int obtCost=1;
    obtCost = sine_getCost(inter);
    if(cost>=obtCost)
        return 1;
    else
        return 0;
}

class DownloadLimit
{
    int resolvedwnLimit(int dwnlimit);
};

class ConditionList
{
    public:
    Location location;
    Bandwidth bandwidth;
    Cost cost;
    DownloadLimit dwnLimit;
};

class Action
{
    public:
    Action() : encrypted(1) {}
    string interface;
    int encrypted;
};

class Rules     // Similar to Conditions
{
    public:
    ConditionList* condList;
    Action* action;
};

class Policy
{
    public:
    int policyID;
    int appID;
    Rules* rule;
};



/*
Declaring the prototypes and a static variable
*/

char* policyModel(int, int);
int evaluate (Action*, ConditionList*, char*, char*);
int parseCondition(char* tempStore,char* metadata,char* data, char* str,int condOffest,int init2,int dataIndex, int j);
static char retInter[10];

char* policyModel(int appID, int sockfd)
{
//yan - for mipv6
    if (appID != 35 && !WITH_MIPV6)
	return NULL;

    ifstream in("/etc/sine_policy.conf"); // input
    if(!in) {
        cout << "\nCannot open config policy file.\n";
        return NULL;
    }

    char strAppID[10] = {0};
    char str[150] = {0};
    char metadata[20] = {0};

    while(in)
    {
        char strAppID[10] = {0};
        in.getline(str, 150);
        int dataIndex=0;
        unsigned int i=0;
       	for(i=0;i<strlen(str);i++)
   	{
            if(str[i]!=','){
           		strAppID[dataIndex]=str[i];
       			dataIndex++;
            }
	        else{
	             strAppID[dataIndex]='\0';
	             break;
	        }
    	}
    	int appIDSearch=atoi(strAppID);
    	if(appIDSearch!=appID)
    	{
    	    continue;
    	}

        else
        {
            cout<<endl<<"Working for appID "<<appIDSearch<<endl;
            Policy policy;
		cout << "policy is newed" << endl;
            policy.appID=appIDSearch;
            char rCount[2];
            rCount[0]=str[i+1];
	    rCount[1] = '\0';
            int ruleCount=atoi(rCount);
            Rules rules[ruleCount];      // Set the # of rules.
		cout << "rules is newed" << endl;
            policy.rule=rules;

            for(int j=0;j<ruleCount;j++)        // looping for each condition
            {
                dataIndex=0;
		memset(metadata,0,20);
                in.getline(str, 255);
                char strCondCount[5] = {0};
                for(i=0;i<strlen(str);i++)
                {
                    if(str[i]!=','){
                        strCondCount[dataIndex]=str[i];
                        dataIndex++;
                    }
                    else{
                        strCondCount[dataIndex]='\0';
                        break;
                    }
                }
                int condCount = atoi(strCondCount);
                char tempStore[20];
                char data1[10];      // Do not delete this pointer
                int interIndex=0;
                i++;


                while(i<strlen(str))
                {
                    tempStore[interIndex]=str[i];
                    i++;
                    interIndex++;
                }
                tempStore[interIndex]='\0';
                int init1=0;
                int j=0;
                    for(;init1<=strlen(tempStore);init1++)
                    {
                        if(tempStore[init1]!=' ' && tempStore[init1]!='\0')
                        {
                            metadata[j]=tempStore[init1];
                            j++;
                        }
                        else
                        {
                            metadata[j]='\0';
                            break;
                        }
                    }
                    init1=init1+1;
                    j=0;
                    for(;init1<=strlen(tempStore);init1++)
                    {
                        if(tempStore[init1]!=' ' && tempStore[init1]!='\0')
                        {
                            data1[j]=tempStore[init1];
                            j++;
                        }
                        else
                        {
                            data1[j]='\0';
                            break;
                        }
                    }


                Action act; // Assigning actions to rules and itnerfaces to actions.
			cout << "new actions" << endl;
                ConditionList cl[condCount];
			cout << "new conditionlist" << endl;
                rules[j].action=&act;

                rules[j].condList=cl;

               //cout<<"Got 2 - "<<metadata<<endl;

                if(!strcmp(metadata,"inter"))
                {
                  //  cout<<"In the interface "<<data1<<endl;
                    Interface obj;
                    act.interface=data1;
                    if(!(obj.interfaceEval(data1)))
                    {
                        for(int i=0;i<condCount;i++)
                            in.getline(str, 255);
                        continue;
                    }
                }

                else if(!strcmp(metadata,"loc"))
                {
                    cout<<"\nQuery location from Location Manager.\n";
                    Location obj;
                    if(!(obj.resolveLocation(data1)))
                    {
                        cout<<"Loc does not match "<<data1<<endl;
                        for(int i=0;i<condCount;i++)
                            in.getline(str, 255);
                        continue;

                    }
			cout<<"Loc matched"<<endl;
                }

                else if(!strcmp(metadata,"band"))
                {
                    //act.interface=interfaces;    // we assign the act.interface later on.
                    Bandwidth obj;
                    int band = atoi(data1);
                    /*if(!(obj->resolveBandwidth(band)))
                    {
                        for(int i=0;i<condCount;i++)
                            in.getline(str, 255);
                        continue;
                    }*/
                }

                else if(!strcmp(metadata,"cost"))
                {
                    //act.interface=interfaces;    // we assign the act.interface later on.
                    Cost obj;
                    int cost = atoi(data1);
                    /*if(!(obj->resolveCost(cost)))
                    {
                        for(int i=0;i<condCount;i++)
                            in.getline(str, 255);
                        continue;
                    }*/
                }

                else if(!strcmp(metadata,"limit"))
                {
                    //act.interface=interfaces;    // we assign the act.interface later on.
                    DownloadLimit obj;
                    int limit = atoi(data1);
                    /*if(!(obj->resolvedwnLimit(limit)))
                    {
                        for(int i=0;i<condCount;i++)
                            in.getline(str, 255);
                        continue;
                    }*/
                }

		//yan - begin - encrypt
		else if (!strcmp(metadata, "encrypt"))
		{
			act.encrypted = strcmp(data1, "off");
		}
		//yan - end

                cout<<"\nTrying to check a condition\n";
                // Now loop for condCount
                for(int i=0;i<condCount;i++)
                {
                     in.getline(str, 255);
			
		     // TODO - Change the following to a loop
                     // Working on the first part

			//yan - rewrite the code
			int condOffset = 0;
			int status = 1;
			while (condOffset < strlen(str))
			{
				dataIndex=0;
				memset(tempStore,0,20);
				memset(metadata,0,10);
				char data2[20] = {0};
				int init2=0;
				j=0;
				condOffset=parseCondition(tempStore,metadata,data2,str,condOffset,init2, dataIndex, j) + 1;

				if (!evaluate(&act, cl, metadata, data2))
				{
					status = 0;
					break;
				}
			}
			if (!status)
				continue;
			//yan - end
/*

                    // Work on the second part
                     dataIndex=0;
                     condOffest++;
                     memset(tempStore,0,20);
                     memset(metadata,0,10);
                     init2=0;
                     j=0;
                     char data3[20];
                    condOffest=parseCondition(tempStore,metadata,data3,str,condOffest,init2, dataIndex, j);
                     status = evaluate(&act, cl, metadata, data3);
                     if(!status)
                        continue;


                    // Work on the third part
                     dataIndex=0;
                     condOffest++;
                     memset(tempStore,0,20);
                     memset(metadata,0,10);
                     char data4[20];
                     init2=0;
                     j=0;
                    condOffest=parseCondition(tempStore,metadata,data4,str,condOffest,init2, dataIndex, j);
                     status = evaluate(&act, cl, metadata, data4);
                     if(!status)
                        continue;


                    // Work on the part4
                     dataIndex=0;
                     condOffest++;
                     memset(tempStore,0,20);
                     memset(metadata,0,10);
                     char data5[20];
                     init2=0;
                     j=0;
                     condOffest=parseCondition(tempStore,metadata,data5,str,condOffest,init2, dataIndex, j);
                     status = evaluate(&act, cl, metadata, data5);
                     if(!status)
                        continue;
  */                   
			strcpy(retInter,act.interface.c_str());


			// Need this API from Sabari
			/*
			   if(isHipAddress())
			   {
	                        sine_setHipIf(sockfd, retInter);
        	                if(SINE_SHIBBOLETH && !sine_shib(retInter))
                	                continue;
                        	sine_confirmHipIf(sockfd, retInter);
			   }
			   else
			   {
				sine_setMipIf(sockfd, retInter);	
			   }		
			*/
			
			// Comment out the below 4 lines if you are using the above if-else block	
			//yan - mipv6
			if (appID == 36) {
				sine_setMipIf(sockfd, retInter);
				if(SINE_SECURITY && !sine_shib(retInter))
					continue;
				//sine_confirmMipIf(sockfd, retInter);
			} else {
				sine_hip_encrypt(act.encrypted);
				sine_setHipIf(sockfd, retInter);
				if(SINE_SECURITY) {
					if (WITH_BROWSER && !sine_shib_browser(retInter))
						continue;
					if (!WITH_BROWSER && !sine_shib(retInter))
						continue;
				}
				sine_confirmHipIf(sockfd, retInter);
			}

                     return retInter;

                }   // Condition loop ends
            }
            return NULL;
        }   // else ends (I am checking the right policy)
    }   // while ends
}   // function returns


int parseCondition(char* tempStore,char* metadata,char* data2, char* str,int condOffest,int init2,int dataIndex, int j)
{
                     for(;condOffest<strlen(str);condOffest++)
                     {
                        if(str[condOffest]!=','){
                            tempStore[dataIndex]=str[condOffest];
                            dataIndex++;
                        }
                        else{
                            tempStore[dataIndex]='\0';
                            break;
                        }
                     }


                    for(;init2<=strlen(tempStore);init2++)
                    {
                        if(tempStore[init2]!=' ' && tempStore[init2]!='\0')
                        {
                            metadata[j]=tempStore[init2];
                            j++;
                        }
                        else
                        {
                            //j++;
                            metadata[j]='\0';
                            break;
                        }
                    }
                    init2=init2+1;
                    j=0;
                    for(;init2<=strlen(tempStore);init2++)
                    {
                        if(tempStore[init2]!=' ' && tempStore[init2]!='\0')
                        {
                            data2[j]=tempStore[init2];
                            j++;
                        }
                        else
                        {
                            data2[j]='\0';
                            break;
                        }
                    }
		return condOffest;
}



int evaluate (Action* act, ConditionList* cl, char* metadata, char* data2)
{
		//cout<<"Metadata - "<<metadata<<endl;
                if(!strcmp(metadata,"inter"))
                {
                    act->interface=data2;
                    cout<<"intermediate - "<<act->interface<<" "<<data2<<endl;

                    Interface obj;
                    if(!(obj.interfaceEval(data2)))
                        return 0;

                }

                else if(!strcmp(metadata,"loc"))
                {
                      Location locObj;
                       cl->location=locObj;                   // Do not delete strLocation -> memory leak.
                      if(!((cl->location).resolveLocation(data2)))
			{
				cout << "Location not matched 2" << endl;
                        	return 0;
			}	
			cout << "Location matched 2" << endl;

                }

                else if(!strcmp(metadata,"band"))
                {
                    char dataBand[5];
                    char strOffset[5];
                    int offset=0;
                    int init1=0;

                    int j=0;
                    for(;init1<=strlen(data2);init1++)
                    {
                        if(data2[init1]!='-' && data2[init1]!='\0')
                        {
                            dataBand[j]=data2[init1];
                            j++;
                        }
                        else
                        {
                            dataBand[j]='\0';
                            break;
                        }
                    }
                    init1=init1+1;
                    j=0;
                    for(;init1<=strlen(data2);init1++)
                    {
                        if(data2[init1]!='-' && data2[init1]!='\0')
                        {
                                strOffset[j]=data2[init1];
                                j++;
                        }
                        else
                        {
                                strOffset[j]='\0';
                                break;
                        }
                     }


                     int bandwidth = atoi(dataBand);
                     Bandwidth bandObj;
                     cl->bandwidth=bandObj;
                     offset = atoi(strOffset);
                     if(!((cl->bandwidth).resolveBandwidth(act->interface.c_str(), bandwidth - offset)))
                        return 0;

                }

                else if(!strcmp(metadata,"cost"))
                {
                    char dataCost[5];
                    char strOffset[5];
                    int offset=0;
                    int init1=0;

                    int j=0;
                    for(;init1<=strlen(data2);init1++)
                    {
                        if(data2[init1]!='-' && data2[init1]!='\0')
                        {
                            dataCost[j]=data2[init1];
                            j++;
                        }
                        else
                        {
                            dataCost[j]='\0';
                            break;
                        }
                    }
                    init1=init1+1;
                    j=0;
                    for(;init1<=strlen(data2);init1++)
                    {
                        if(data2[init1]!='-' && data2[init1]!='\0')
                        {
                                strOffset[j]=data2[init1];
                                j++;
                        }
                        else
                        {
                                strOffset[j]='\0';
                                break;
                        }
                     }

                    int cost = atoi(dataCost);
                    Cost costObj;
                    cl->cost=costObj;
                    offset = atoi(strOffset);
                    if(!((cl->cost).resolveCost(act->interface.c_str(), cost+offset)))
                       return 0;

                    //delete cl->cost;
                }

                else if(!strcmp(metadata,"limit"))
                {
                    int download = atoi(data2);
                    DownloadLimit dwnObj;
                    cl->dwnLimit=dwnObj;
           /*          if(!((cl->dwnLimit).resolvedwnLimit(download)))
                        return 0;
            */
                }

		else if (!strcmp(metadata, "encrypt"))
		{	//yan - hip encryption
			act->encrypted = strcmp(data2, "off");
		}

                return 1;
}
