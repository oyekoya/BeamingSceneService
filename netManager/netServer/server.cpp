//Server Application - Raknet link and logfile recording

#include "ClientServer.h"
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#endif

int main(int argc, char *argv[])
{
	char ch;
	SocketDescriptor sd;
	//char ip[128];
	static int SERVER_PORT=12050;

	// ReplicaManager3 requires NetworkIDManager to lookup pointers from numbers.
	NetworkIDManager networkIdManager;
	// Each application has one instance of RakPeerInterface
	RakPeerInterface *rakPeer;
	// The system that performs most of our functionality for this server
	ReplicaManager3Beaming replicaManager;

	printf("NetServer handles objects creation and destruction and automatic serialization of data members.\n");
	ch='s'; 

	rakPeer = RakNetworkFactory::GetRakPeerInterface();
	topology=SERVER;
	if (argc > 1)
		SERVER_PORT = atoi(argv[1]);
	else
		printf("To specify an alternative port (default=12050), run: server port_no\n");
	sd.port=SERVER_PORT;
	printf("press key 'l' - Starts logging.\n");

	// ObjectMemberRPC, AutoRPC for objects, and ReplicaManager3 require that you call SetNetworkIDManager()
	rakPeer->SetNetworkIDManager(&networkIdManager);
	// The network ID authority is the system that creates the common numerical identifier used to lookup pointers.
	// For client/server this is the server
	// For peer to peer this would be true on every system, and NETWORK_ID_SUPPORTS_PEER_TO_PEER should be defined in RakNetDefines.h
	networkIdManager.SetIsNetworkIDAuthority(true);
	// Start RakNet, up to 32 connections if the server
	rakPeer->Startup(32,10,&sd,1);
	rakPeer->AttachPlugin(&replicaManager);
	rakPeer->SetMaximumIncomingConnections(32);

	printf("Started server on port %i\n", SERVER_PORT);
	printf("Server GUID is %s \n",rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString());

		//printf("\n");

#ifdef WIN32	
	STARTUPINFO si;					//startup handle for audio process
	PROCESS_INFORMATION pi;			//process handle for audio process
#endif

	// Enter infinite loop to run the system
	Packet *packet;
	bool quit=false;
	//std::map<std::string, std::vector<node_info*> >::iterator it;
	while (!quit)
	{
		for (packet = rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet = rakPeer->Receive())
		{
			switch (packet->data[0])
			{
			case ID_CONNECTION_ATTEMPT_FAILED:
				printf("ID_CONNECTION_ATTEMPT_FAILED\n");
				quit=true;
				break;
			case ID_NO_FREE_INCOMING_CONNECTIONS:
				printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
				quit=true;
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
				break;
			case ID_NEW_INCOMING_CONNECTION:
				printf("ID_NEW_INCOMING_CONNECTION from %s, guid %s\n", packet->systemAddress.ToString(),packet->guid.ToString());
				/*it = nodes_map.find(packet->guid.ToString());
				if (it != nodes_map.end()) //if id exists in node database, clear out
					nodes_map[packet->guid.ToString()].clear();
				//avatar_joint_replicas.clear();*/
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				printf("ID_DISCONNECTION_NOTIFICATION\n");
				break;
			case ID_CONNECTION_LOST:
				printf("ID_CONNECTION_LOST\n");
				break;
			}
		}

		if (kbhit())
		{
			ch=getch();
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				quit=true;
			}
			//for debugging
			if (ch=='n' || ch=='N')
			{
				for ( std::map<std::string, std::vector<node_info*> >::iterator cIter = nodes_map.begin(); cIter!=nodes_map.end(); cIter++ )
					printf("%d, ",cIter->second.size());
				printf("\n");
			}
			if (ch=='l' || ch=='L')
			{
			#ifdef WIN32	
				logging=!logging;
				if (logging==true)
				{
					printf("Logging Enabled.\n");
					
					int logstartTime = (int) RakNet::GetTime();		//get time for log name
										
					char commandline[256];							//set command line for audio record
					sprintf(commandline, "Harddisk.exe -preset beaming.hdp -filter beaming.hfs -output BeamingLog_%i.ogg -minimize -record",logstartTime);
					
					ZeroMemory( &si, sizeof(si) );		//prepare for audio recording process
					si.cb = sizeof(si);					//..
					ZeroMemory( &pi, sizeof(pi) );		//..

					if( !CreateProcess( NULL,   		// No module name (use command line)
						commandline,					// Command line for audio client exe
						NULL,           				// Process handle not inheritable
						NULL,           				// Thread handle not inheritable
						FALSE,          				// Set handle inheritance to FALSE
						0,              				// No creation flags
						NULL,           				// Use parent's environment block
						NULL,           				// Use parent's starting directory 
						&si,            				// Pointer to STARTUPINFO structure
						&pi )           				// Pointer to PROCESS_INFORMATION structure
						) 
					{									//warn on fail
						printf("Create Audio Process failed (%d).\n", GetLastError() );	
					}
					
					rakPeer->AttachPlugin(&logfileHandler);
					logfileHandler.StartLog("BeamingLog", logstartTime); //to test -  standard output should be automatically written
					char starttime[128];
					char startstr[128];
					logfileHandler.GetLocalTime(starttime);
					sprintf(startstr,"S;%s",starttime);
					logfileHandler.WriteLog(startstr);
					//logfileHandler.WriteLog("Wole is testing"); //writes additional lines
					//logfileHandler.WriteMiscellaneous("audiotime","234678"); //should enable writing additional information - not on the same line?
	
					char localtime[128];
					char logstr[1024];
					logfileHandler.GetLocalTime(localtime);
					RakNetTimeMS time = RakNet::GetTime();
					//NOTE: This is out of date and doesn't work anymore. Need to update, as it only logged avatars. 
					for ( std::map<RakNetGUID, std::vector<BeamingAvatarJointReplica*> >::iterator cIter = avatar_joint_replicas.begin(); cIter!=avatar_joint_replicas.end(); cIter++ )
					{
						for (int i=0; i<=cIter->second.size();i++)
						{
							sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%i;%i",(unsigned long long)time,localtime,cIter->first.ToString(),i,avatar_joint_replicas.size());
							logfileHandler.WriteLog(logstr);
						}
					}
				}
				else
				{
					rakPeer->DetachPlugin(&logfileHandler);
					printf("Logging Disabled.\n");
					DWORD exitCode=0;								//kill the audio recording process
					GetExitCodeProcess(pi.hProcess, &exitCode);		//..
					TerminateProcess(pi.hProcess, exitCode);		//..
					CloseHandle(pi.hProcess);						//..
					char endtime[128];
					char endstr[128];
					logfileHandler.GetLocalTime(endtime);
					sprintf(endstr,"E;%s",endtime);
					logfileHandler.WriteLog(endstr);
				}
			#endif
			}
		}

		RakSleep(1);
	}

	#ifdef WIN32	
	if (logging==true)
	{
		rakPeer->DetachPlugin(&logfileHandler);
		printf("Logging Disabled.\n");
		DWORD exitCode=0;								//kill the audio recording process
		GetExitCodeProcess(pi.hProcess, &exitCode);		//..
		TerminateProcess(pi.hProcess, exitCode);		//..
		CloseHandle(pi.hProcess);						//..
		char endtime[128];
		char endstr[128];
		logfileHandler.GetLocalTime(endtime);
		sprintf(endstr,"E;%s",endtime);
		logfileHandler.WriteLog(endstr);
	}
	#endif

	//Tells clients to destroy local copies when server exits.
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	// ClearPointers is needed, as I don't track which objects have and have not been allocated at the application level. 
	// So ClearPointers will call delete on every object in the returned list, 
	// which is every object that the application has created. Another way to put it is
	// 	A. Send a packet to tell other systems to delete these objects
	// 	B. Delete these objects on my own system
	replicaManager.GetReferencedReplicaList(replicaListOut);
	replicaManager.BroadcastDestructionList(replicaListOut, UNASSIGNED_SYSTEM_ADDRESS);
	replicaListOut.ClearPointers( true, __FILE__, __LINE__ );
	rakPeer->Shutdown(100,0);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);

	return 1;
}
