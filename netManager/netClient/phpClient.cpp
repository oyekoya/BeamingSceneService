/*!
 *	phpClient.cpp 
 *	This client acts as a gateway, in that it simply connects to the server and sends information to the php webpage
 *
 *	Contact Wole Oyekoya <w.oyekoya@cs.ucl.ac.uk> for any queries
 * 
 *  Include client.h
 *
 */
#include "TCPInterface.h"
#include "HTTPConnection.h"
#include "PHPConnections.h"
#include "RakSleep.h"
#include "RakString.h"
#include "GetTime.h"
#include "DS_Table.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <stdio.h>
#include "Kbhit.h"
#include "Getche.h"
#include "client.h"
#include <stdlib.h>
#include <time.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include<unistd.h>
//#include<cstdlib>
#endif
#include "TCPInterface.h"
#include "HTTPConnection.h"
#include "PHPConnections.h"

// Allocate rather than create on the stack or the RakString mutex crashes on shutdown
TCPInterface *tcp;
HTTPConnection *httpConnection;
PHPConnections *phpConnections;

enum ReadResultEnum
{
	RR_EMPTY_TABLE,
	RR_READ_TABLE,
	RR_TIMEOUT,
};

ReadResultEnum ReadResult(RakNet::RakString &httpResult)
{
	RakNetTimeMS endTime=RakNet::GetTimeMS()+10000;
	httpResult.Clear();
	while (RakNet::GetTimeMS()<endTime)
	{
		Packet *packet = tcp->Receive();
		if(packet)
		{
			httpConnection->ProcessTCPPacket(packet);
			tcp->DeallocatePacket(packet);
		}

		if (httpConnection->HasRead())
		{
			httpResult = httpConnection->Read();
			// Good response, let the PHPConnections class handle the data
			// If resultCode is not an empty string, then we got something other than a table
			// (such as delete row success notification, or the message is for HTTP only and not for this class).
			HTTPReadResult readResult = phpConnections->ProcessHTTPRead(httpResult);

			if (readResult==HTTP_RESULT_GOT_TABLE)
			{
				//printf("RR_READ_TABLE\n");
				return RR_READ_TABLE;
			}
			else if (readResult==HTTP_RESULT_EMPTY)
			{
				//printf("HTTP_RESULT_EMPTY\n");
				return RR_EMPTY_TABLE;
			}
		}

		// Update our two classes so they can do time-based updates
		httpConnection->Update();
		phpConnections->Update();

		// Prevent 100% cpu usage
		RakSleep(1);
	}

	return RR_TIMEOUT;
}

bool HaltOnUnexpectedResult(ReadResultEnum result, ReadResultEnum expected)
{
	if (result!=expected)
	{
		printf("TEST FAILED. Expected ");

		switch (expected)
		{

		case RR_EMPTY_TABLE:
			printf("no results");
			break;
		case RR_TIMEOUT:
			printf("timeout");
			break;
		case RR_READ_TABLE:
			printf("to download result");
			break;
		}
		
		switch (result)
		{
		case RR_EMPTY_TABLE:
			printf(". No results were downloaded");
			break;
		case RR_READ_TABLE:
			printf(". Got a result");
			break;
		case RR_TIMEOUT:
			printf(". Timeout");
			break;
		}
		printf("\n");
		return true;
	}

	return false;
}

void DownloadTable()
{
	phpConnections->DownloadTable("beammedown");
}
void UploadTable(RakNet::RakString clientName, unsigned short clientPort)
{
	phpConnections->UploadTable("beammeup", clientName, clientPort, false);
}
void UploadAndDownloadTable(RakNet::RakString clientName, unsigned short clientPort)
{
	phpConnections->UploadAndDownloadTable("beammeup", "beammedown", clientName, clientPort, false);
}
bool PassTestOnEmptyDownloadedTable()
{
	const DataStructures::Table *clients = phpConnections->GetLastDownloadedTable();

	if (clients->GetRowCount()==0)
	{
		printf("Test passed.\n");
		return true;
	}
	printf("TEST FAILED. Empty table should have been downloaded.\n");
	return false;
}
bool VerifyDownloadMatchesUpload(int requiredRowCount, int testRowIndex)
{
	const DataStructures::Table *clients = phpConnections->GetLastDownloadedTable();
	if (clients->GetRowCount()!=(unsigned int)requiredRowCount)
	{
		printf("TEST FAILED. Expected %i result rows, got %i\n", requiredRowCount, clients->GetRowCount());
		return false;
	}
	RakNet::RakString columnName;
	RakNet::RakString value;
	unsigned int i;
	DataStructures::Table::Row *row = clients->GetRowByIndex(testRowIndex,NULL);
	const DataStructures::List<DataStructures::Table::ColumnDescriptor>& columns = clients->GetColumns();
	unsigned int colIndex;
	// +4 comes from automatic fields
	// _CLIENT_PORT
	// _CLIENT_NAME
	// _SYSTEM_ADDRESS
	// __SEC_AFTER_EPOCH_SINCE_LAST_UPDATE
	if (phpConnections->GetFieldCount()+4!=clients->GetColumnCount())
	{
		printf("TEST FAILED. Expected %i columns, got %i\n", phpConnections->GetFieldCount()+4, clients->GetColumnCount());
		printf("Expected columns:\n");
		for (colIndex=0; colIndex < phpConnections->GetFieldCount(); colIndex++)
		{
			phpConnections->GetField(colIndex, columnName, value);
			printf("%i. %s\n", colIndex+1, columnName.C_String());
		}
		printf("%i. _CLIENT_PORT\n", colIndex++);
		printf("%i. _CLIENT_NAME\n", colIndex++);
		printf("%i. _System_Address\n", colIndex++);
		printf("%i. __SEC_AFTER_EPOCH_SINCE_LAST_UPDATE\n", colIndex++);

		printf("Got columns:\n");
		for (colIndex=0; colIndex < columns.Size(); colIndex++)
		{
			printf("%i. %s\n", colIndex+1, columns[colIndex].columnName);
		}

		return false;
	}
	for (i=0; i < phpConnections->GetFieldCount(); i++)
	{
		phpConnections->GetField(i, columnName, value);
		for (colIndex=0; colIndex < columns.Size(); colIndex++)
		{
			if (strcmp(columnName.C_String(), columns[colIndex].columnName)==0)
				break;
		}
		if (colIndex==columns.Size())
		{
			printf("TEST FAILED. Expected column with name %s\n", columnName.C_String());
			return false;
		}

		if (strcmp(value.C_String(), row->cells[colIndex]->c)!=0)
		{
			printf("TEST FAILED. Expected row with value '%s' at index %i for column %s. Got '%s'.\n", value.C_String(), i, columnName.C_String(), row->cells[colIndex]->c);
			return false;
		}
	}

	printf("Test passed.\n");
	return true;
}
void PrintHttpResult(RakNet::RakString httpResult)
{
	printf("--- Last result read ---\n");
	printf("%s", httpResult.C_String());
}
void PrintFieldColumns(void)
{
	unsigned int colIndex;
	RakNet::RakString columnName;
	RakNet::RakString value;
	for (colIndex=0; colIndex < phpConnections->GetFieldCount(); colIndex++)
	{
		phpConnections->GetField(colIndex, columnName, value);
		printf("%i. %s\n", colIndex+1, columnName.C_String());
	}
}
bool RunTest()
{
	RakNet::RakString httpResult;
	ReadResultEnum rr;
	char ch[32];
	printf("Warning, table must be clear before starting the test.\n");
	printf("Press enter to start\n");
	gets(ch);

	printf("*** Testing initial table is empty.\n");
	// Table should start empty
	DownloadTable();
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
		{PrintHttpResult(httpResult); return false;}
	if (PassTestOnEmptyDownloadedTable()==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Downloading again, to ensure download does not modify the table.\n");
	// Downloading should not modify the table
	DownloadTable();
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
		{PrintHttpResult(httpResult); return false;}
	if (PassTestOnEmptyDownloadedTable()==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing upload.\n");
	// Upload values likely to mess up PHP
	phpConnections->SetField("TestField1","0");
	phpConnections->SetField("TestField2","");
	phpConnections->SetField("TestField3"," ");
	phpConnections->SetField("TestField4","!@#$%^&*(");
	phpConnections->SetField("TestField5","A somewhat big long string as these things typically go.\nIt even has a linebreak!");
	phpConnections->SetField("TestField6","=");
	phpConnections->UploadTable("beammeup", "FirstClientUpload", 80, false);
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
		{PrintHttpResult(httpResult); return false;}
	if (PassTestOnEmptyDownloadedTable()==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing download, should match upload exactly.\n");
	// Download what we just uploaded
	DownloadTable();
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_READ_TABLE))
		{PrintHttpResult(httpResult); return false;}
	// Check results
	if (VerifyDownloadMatchesUpload(1,0)==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing that download works twice in a row.\n");
	// Make sure download works twice
	DownloadTable();
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_READ_TABLE))
		{PrintHttpResult(httpResult); return false;}
	// Check results
	if (VerifyDownloadMatchesUpload(1,0)==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing reuploading a client to modify fields.\n");
	// Modify fields
	phpConnections->SetField("TestField1","zero");
	phpConnections->SetField("TestField2","empty");
	phpConnections->SetField("TestField3","space");
	phpConnections->SetField("TestField4","characters");
	phpConnections->SetField("TestField5","A shorter string");
	phpConnections->SetField("TestField6","Test field 6");
	phpConnections->UploadTable("beammeup", "FirstClientUpload", 80, false);
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
	{PrintHttpResult(httpResult); return false;}
	if (PassTestOnEmptyDownloadedTable()==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing that downloading returns modified fields.\n");
	// Download what we just uploaded
	DownloadTable();
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_READ_TABLE))
		{PrintHttpResult(httpResult); return false;}
	// Check results
	if (VerifyDownloadMatchesUpload(1,0)==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing that downloading works twice.\n");
	// Make sure download works twice
	DownloadTable();
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_READ_TABLE))
		{PrintHttpResult(httpResult); return false;}
	// Check results
	if (VerifyDownloadMatchesUpload(1,0)==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing upload of a second client.\n");
	// Upload another client
	phpConnections->SetField("TestField1","0");
	phpConnections->SetField("TestField2","");
	phpConnections->SetField("TestField3"," ");
	phpConnections->SetField("TestField4","Client two characters !@#$%^&*(");
	phpConnections->UploadTable("beammeup", "SecondClientUpload", 80, false);
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
	{PrintHttpResult(httpResult); return false;}
	if (PassTestOnEmptyDownloadedTable()==false)
		{PrintHttpResult(httpResult); return false;}
	/*RakNetTimeMS startTime = RakNet::GetTimeMS();

	printf("*** Testing 20 repeated downloads.\n");
	printf("Field columns\n");
	PrintFieldColumns();

	// Download repeatedly
	unsigned int downloadCount=0;
	while (downloadCount < 20)
	{
		printf("*** (%i) Downloading 'FirstClientUpload'\n", downloadCount+1);
		// Download again (First client)
		DownloadTable();
		if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_READ_TABLE))
			{PrintHttpResult(httpResult); return false;}
		// Check results
		// DOn't have this stored anymore
//		if (VerifyDownloadMatchesUpload(2,0)==false)
//			{PrintHttpResult(httpResult); return false;}

		printf("*** (%i) Downloading 'SecondClientUpload'\n", downloadCount+1);
		// Download again (second client)
		DownloadTable();
		if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_READ_TABLE))
			{PrintHttpResult(httpResult); return false;}
		// Check results
		if (VerifyDownloadMatchesUpload(2,1)==false)
			{PrintHttpResult(httpResult); return false;}

		downloadCount++;

		RakSleep(1000);
	}

	printf("*** Waiting for 70 seconds to have elapsed...\n");
	RakSleep(70000 - (RakNet::GetTimeMS()-startTime));


	printf("*** Testing that table is now clear.\n");
	// Table should be cleared
	DownloadTable();
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
		{PrintHttpResult(httpResult); return false;}
	if (PassTestOnEmptyDownloadedTable()==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing upload and download. No clients should be downloaded.\n");
	phpConnections->ClearFields();
	phpConnections->SetField("TestField1","NULL");
	UploadAndDownloadTable("FirstClientUpload", 80);
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
		{PrintHttpResult(httpResult); return false;}
	if (PassTestOnEmptyDownloadedTable()==false)
		{PrintHttpResult(httpResult); return false;}

	printf("*** Testing upload and download. One client should be downloaded.\n");
	UploadAndDownloadTable("ThirdClientUpload", 80);
	if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_READ_TABLE))
		{PrintHttpResult(httpResult); return false;}
	if (VerifyDownloadMatchesUpload(1,0)==false)
		{PrintHttpResult(httpResult); return false;}*/

	return true;
}

void OutputBody(HTTPConnection& http, const char *path, const char *data, TCPInterface& tcp);

int main(int argc, char**argv)
{
	char connectedclients[1024];
	char *guid, *node, *tmpid, *nodeid;
	char allnodes[2048], node_info[256];
	int loop = 0, index = 1;
	char nodes[512];
	bool quit = false;
	char ch;
	char iddata[128];
	char website[256];
	char pathToPHP[256];
	RakNet::RakString httpResult;
	ReadResultEnum rr;
	char ipdetails[128];

	srand((unsigned)time(0));
	//connect using startclient
	int connected = startclient("128.16.7.66", 12050/*"193.205.82.196", 50000*/,"UCL","VISITOR","m016.cfg",false);
#if defined(_WIN32)
	Sleep(100);
#else
	sleep(2);
#endif
	if (!connected)
	{ exit(1); }

	printf("Starts streaming to php webpage: http://web4.cs.ucl.ac.uk/research/vr/Projects/BEAMING/wole/BeamingSceneService/List/Connections.php\n");
	printf("Commands:\n(Q)uit\n");
	//printf("connected = %i\n", connected);


	strcpy(iddata,"");

	printf("Streaming to http://web4.cs.ucl.ac.uk/research/vr/Projects/BEAMING/wole/BeamingSceneService/List/Connections.php ...\n");

	tcp = RakNet::OP_NEW<TCPInterface>(__FILE__,__LINE__);
	httpConnection = RakNet::OP_NEW<HTTPConnection>(__FILE__,__LINE__);
	phpConnections = RakNet::OP_NEW<PHPConnections>(__FILE__,__LINE__);
//	RakNetTime lastTouched = 0;
	// Start the TCP thread. This is used for general TCP communication, whether it is for webpages, sending emails, or telnet
	tcp->Start(0, 64);

	strcpy(website, "web4.cs.ucl.ac.uk");
	strcpy(pathToPHP, "research/vr/Projects/BEAMING/wole/BeamingSceneService/List/Connections.php");

	if (website[strlen(website)-1]!='/' && pathToPHP[0]!='/')
	{
		memmove(pathToPHP+1, pathToPHP, strlen(pathToPHP)+1);
		pathToPHP[0]='/';
	}

	// This creates an HTTP connection using TCPInterface. It allows you to Post messages to and parse messages from webservers.
	// The connection attempt is asynchronous, and is handled automatically as HTTPConnection::Update() is called
	httpConnection->Init(tcp, website);
   
	// This adds specific parsing functionality to HTTPConnection, in order to communicate with Connections.php
	phpConnections->Init(httpConnection, pathToPHP);

	while (!quit)
	{
		//check incoming packets
		check();
		//get information on all connected clients. 
		//printf("number of connected clients %i\n",getPeersID(connectedclients));
		getPeersID(connectedclients);
		//printf("%s\n",connectedclients);
		for (guid = strtok (connectedclients, ";"); guid != NULL;
		   guid = strtok (guid + strlen (guid) + 1, ";"))
		{
			strncpy (iddata, guid, sizeof (iddata));
			//for each connected guid in the database, get the nodes information.
			getNodesInfo(iddata,allnodes);
			//printf("%s\n",allnodes);
			for (node = strtok (allnodes, ";"); node != NULL;
			   node = strtok (node + strlen (node) + 1, ";"))
			{
				strncpy (node_info, node, sizeof (node_info));
				//phpConnections->SetField(RakNet::RakString("Node ") + RakNet::RakString("%d",loop),RakNet::RakString(node));
				for (tmpid = strtok (node_info, ","); tmpid != NULL;
				   tmpid = strtok (tmpid + strlen (tmpid) + 1, ","))
				{
					switch (loop) 
					{
					case 0:// first token is the client name (in the case of AVATAR type, avatar id)
						phpConnections->SetField("Client Name",tmpid);
						break;
					case 1:// second token is the node_id 
						phpConnections->SetField("Node ID",tmpid);
						break;
					case 2: //third token is the TYPE
						phpConnections->SetField("Node Type",tmpid);
						break;
					case 3: //fouth token is the config
						phpConnections->SetField("Config",tmpid);
						break;
					}
					loop++;
				} //end for
				loop=0; //reset
				getIPinfo(iddata,ipdetails);
				phpConnections->SetField("_System_Address",ipdetails);
				phpConnections->UploadTable("beammeup", RakNet::RakString("%s Node %d",guid,index), 80, false);
				if (HaltOnUnexpectedResult(rr=ReadResult(httpResult), RR_EMPTY_TABLE))
				{
					PrintHttpResult(httpResult); 
					//return false;
				}
				index++;
				RakSleep(10);
			} //end for
			index = 1;
		} //end for
		RakSleep(1000); //sleep for 2 seconds before uploading next set of records
		if (kbhit())
		{
			ch=getch();
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				quit=true;
			}
		}// end if kbhit()
	}//end while

	// The destructor of each of these references the other, so delete in this order
	RakNet::OP_DELETE(phpConnections,__FILE__,__LINE__);
	RakNet::OP_DELETE(httpConnection,__FILE__,__LINE__);
	RakNet::OP_DELETE(tcp,__FILE__,__LINE__);
	printf("Web streaming disabled.\n");

	//remove all nodes and clean up on exit 
	removeAllNodes();
	stop();
}
