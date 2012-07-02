#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <cstdlib>
using namespace std;

int main()
{

//     This part writes the data.
    ofstream out("configPolicy.txt", ios::out | ios::app);
    char* myApp=(char *) malloc (sizeof(char) * 10);
    char* myAppID=(char *) malloc (sizeof(char) * 20);
    char* myPolicyID=(char *) malloc (sizeof(char) * 10);
    char* costBand=(char *) malloc (sizeof(char) * 20);
    char* rulesNo=(char *) malloc (sizeof(char) * 10);
    char* condNo=(char *) malloc (sizeof(char) * 10);   
    char* conditionNo=(char *) malloc (sizeof(char) * 10);
    char* action=(char *) malloc (sizeof(char) * 10);
    char* location=(char *) malloc (sizeof(char) * 10);
    char* bandwidth=(char *) malloc (sizeof(char) * 10);
    char* cost=(char *) malloc (sizeof(char) * 10);
    char* downloadLimit=(char *) malloc (sizeof(char) * 10);
    int rules, conditions, currRule=0, currCondition=0, appIndex=0, i=0, j=0;
    char** appIDs;// = (char **) malloc (sizeof(char) * 50);


    //allocate memory for rows
    appIDs = (char**)malloc(100 *sizeof(char*));
    //for each row allocate memory for columns
    for(int i=0; i<100; i++) 
    {
   	*(appIDs+i) = (char*)malloc(3 *sizeof(char));
    }	

    char* str=(char *) malloc (sizeof(char) * 500);
    memset (str,0,20);
    char* str2=(char *) malloc (sizeof(char) * 500);
    memset (str2,0,20);
    char* actionStr=(char *) malloc (sizeof(char) * 100);
    memset (actionStr,0,20);
    char temp[10];

    cout<<"Enter the Application(s) - ";
	gets(myApp);
	cout<<"Enter the Application ID(s) - ";
    gets(myAppID);

	/*
    Break the appIDs into individual tokens.
    */
    for(i=0;i<strlen(myAppID);i++)
    {
//        cout<<"Intermediate - "<<str2<<endl<<endl;
	if(myAppID[i]!=' '){
            appIDs[appIndex][j]=myAppID[i];
            j++;
        }
        else{
             appIDs[appIndex][j]='\0';
             appIndex++;
             j=0;
        }
    }
    appIDs[appIndex][j]='\0';

	cout<<"Enter the policyID - ";
	gets(myPolicyID);
	cout<<"Number of rules in this policy - ";
	gets(rulesNo);
	rules = atoi(rulesNo);

    while(currRule<rules)
    {
        cout<<"**********************\n*******Rule #"<<currRule+1<<"********\n";

        cout<<"Action associated with this rule - ";
        gets(action);
        strcat(actionStr,action);
        strcat(actionStr," \n");

        cout<<"Number of conditions this policy is valid for - ";
        gets(conditionNo);
        conditions = atoi(conditionNo);
	strcat(condNo,conditionNo);
	strcat(condNo," \n");
	cout<<"Just pasted "<<conditionNo;
        currCondition=0;
       // strcat(str,"\n");
        while(currCondition<conditions)
        {
            strcat(str,conditionNo);
            strcat(str,",");
            //itoa(currCondition+1,temp,5);
            //strcat(str,temp);
            //strcat(str,",");
            strcat(str,action);
            strcat(str,",");
            cout<<"***************************\n*******Condition #"<<currCondition+1<<"********\n";
            cout<<"Enter the Location(s) - ";
            gets(location);
            strcat(str,location);
            strcat(str,",");

            cout<<"Enter the Bandwidth (Kbps) - ";
            gets(bandwidth);
            strcat(str,bandwidth);
            strcat(str,",");
    	    strcat(costBand,bandwidth);
	    strcat(costBand," "); 	

            cout<<"Enter the cost (KB/$) - ";
            gets(cost);
            strcat(str,cost);
            strcat(str,",");
	    strcat(costBand,cost);
    	    strcat(costBand,"\n");

            cout<<"Enter the Max. Download Limit (MB)- ";
            gets(downloadLimit);
            strcat(str,downloadLimit);
            strcat(str,"\n");
            currCondition++;
        }
        currRule++;
    }
    //cout<<"Finally - "<<str<<endl;

    //cout<<"Now - "<<str<<endl;
    /*
    Add data to the file.
    */
    for(int i=0;i<=appIndex;i++)
    {
     //   cout<<"Here\n";
        out<<myPolicyID<<","<<appIDs[i]<<","<<rules<<"\n";
     //   cout<<myPolicyID<<","<<appIDs[i]<<","<<rules<<"\n";
        out<<str<<"\n";
     //   cout<<str<<"\n";
    }
    out.flush();
    out.close();


    ofstream tempOut("tempData.txt", ios::out | ios::app);
    if(!tempOut)
    {
        cout<<"Problem with I/O";
        return 1;
    }

    //cout<<"Also appending "<<rulesNo<<endl;
    for(int i=0;i<=appIndex;i++)
    {
        tempOut<<"\n"<<appIDs[i]<<"\n";
	tempOut<<rulesNo<<"\n";
	tempOut<<condNo;
        tempOut<<costBand;	
        //cout<<appIDs[i]<<"\n";
        tempOut<<actionStr<<"\n";
    }
    tempOut<<"\n";
    tempOut.flush();
    tempOut.close();

    return 0;
}


