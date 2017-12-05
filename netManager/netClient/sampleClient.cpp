/*!
 *	sampleClient.cpp 
 *	This client acts as a thin client, in that it simply connects to the server and sends and recieves data from server
 *	Use this file as a guide on how to connect to the Beaming Scene Service
 *
 *	Contact Wole Oyekoya <w.oyekoya@cs.ucl.ac.uk> for any queries
 * 
 *  Include client.h
 *
 */
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
#include<cstdlib>
#endif


int main(int argc, char**argv)
{
	srand((unsigned)time(0));
	float px,py,pz,eh,ep,er,hh,hp,hr;
	///connect using startclient
	/// - startclient("127.0.0.1", 12050,"WOLE1","AVATAR","CMan0023.cfg",false);
	int connected = startclient(/*"128.16.7.66"*/"127.0.0.1", 12050,"UCL","VISITOR","m016.cfg",false);
#if defined(_WIN32)
	Sleep(100);
#else
	sleep(2);
#endif

	printf("Commands:\n(Q)uit\n'C'reate objects\n'M'ove objects\n'R'otate objects\n'F'etch remote objects\n'D'estroy all objects\n");
	//printf("connected = %i\n", connected);
	char m_id[32];
	getMyGUID(m_id);
	printf("My GUID is %s\n",m_id);

	bool quit = false;
	char ch;
	char buf[32];
	float j=0.f;
	char iddata[128];
	char type[128];
	char name[128];
	char my_config[128];
	char fname[128], lname[128];
	strcpy(name,"");
	strcpy(type,"");
	strcpy(iddata,"");
	struct mystruct{
		int x;
		char name[28];
		float myfloat;
	} st; 				

	strcpy(buf,"");
	float tx,ty,tz,ax,ay,az,aw = 0.f;
	///add Rocketbox avatar
	/// - addRocketBoxAvatar("avatar");
	addRocketBoxAvatar("avatar","m016.cfg");
	setAvatarName("avatar","John","Doe");
	///add FACIAL root node
	/// - addNode("Head","FACIAL");
	addNode("Head","FACIAL");
	updateFacialNodes("Head",false,0.5f,0.2f,0.f,0.f,0.f);
	///add AUDIO root node
	/// - addNode("Head","AUDIO");
	addNode("Audio1","AUDIO");
	updateAudioNodes("Audio1","127.0.0.1",0,"http://localhost","config file");

	while (!quit)
	{
		///check incoming packets
		/// - check();
		check();
		j = j + 0.001f;
		if (j>2.f) 
			j=-2.f;
		///update AVATAR root node
		/// - updateRocketBoxAvatar("avatar","0",j,0.8841f,-0.026f,0.f,0.707f,0.f,0.707f);
		updateRocketBoxAvatar("avatar","0",j,-1.18448e-015,-2.70976e-008,0,0.707106,-3.09086e-008,0.707107);
		if (kbhit())
		{
			ch=getch();
			if (ch=='q' || ch=='Q')
			{
				printf("Quitting.\n");
				quit=true;
			}
			if (ch=='g' || ch=='G')
			{
				static int i = 0;
				char genericnode[128];
				sprintf(genericnode,"mystruct%i",i++);
				addNode(genericnode,"GENERIC");
				void *somedata;
				if(!(somedata = malloc(1024))) {
					perror("failed to allocate buffer");
					return 0;
				}
				mystruct *s;
				s = &st;
				s->x = rand() % 11;
				strcpy(s->name,"wole");
				s->myfloat = 100.f * s->x;
				memcpy(somedata,s,1024);
				updateGenericNodes(genericnode,somedata,1024);
				printf("sending %s, %d, %.3f ...\n",((mystruct *)somedata)->name,((mystruct *)somedata)->x,((mystruct *)somedata)->myfloat);
			}
			if (ch=='c' || ch=='C')
			{
				float m1[9] = {0.f};
				float m2[16] = {0.f};
				int datatype = rand() % 9;
				static int i = 0;
				sprintf(buf,"node%i",i++);
				//add node
				switch (datatype)
				{
				case 0:
					char avname[32];
					sprintf(avname,"avatar%i_",i++);
					addRocketBoxAvatar(avname,"m016.cfg");
					setAvatarName(avname,"Another",buf);
					updateRocketBoxAvatar(avname,"0",0,-1.18448e-015,-2.70976e-008,0,0.707106,-3.09086e-008,0.707107);
					break;
				case 1:
					addNode(buf,"FACIAL");
					updateFacialNodes(buf,false,0.f,0.f,0.f,0.f,0.f);
					break;
				case 2:
					addNode(buf,"EMOTION");
					updateEmotionNodes(buf,0,0,0);
					break;
				case 3:
					addNode(buf,"VIDEO");
					updateVideoNodes(buf,"127.0.0.1",0,"http://localhost",1,1,1,m1,m2);
					break;
				case 4:
					addNode(buf,"TACTILE");
					updateTactileNodes(buf,0,0,0);
					break;
				case 5:
					addNode(buf,"ROBOT");
					updateRobotNodes(buf,0,0,0,0,0,0,0,1,0,0,0,0);
					break;
				case 6:
					addNode(buf,"OBJECT");
					updateObjectNodes(buf,"127.0.0.1",0,"http://localhost",0,0,0,0,1,0,0);
					break;
				case 7:
					addNode(buf,"AUDIO");
					updateAudioNodes(buf,"127.0.0.1",0,"http://localhost","config file");
					break;
				case 8:
					addNode(buf,"POINTCLOUD");
					updatePointCloudNodes(buf,"127.0.0.1",0,"http://localhost",0,0,0,0);
					break;
				}
			}
			if (ch=='v' || ch=='V')
			{
				static int v = 0;
				sprintf(buf,"node%i",v++);
				float m1[9] = {1.999f};
				float m2[16] = {3.123f};
				addNode(buf,"VIDEO");
				printf("%.3f\n",m1[0]);
				updateVideoNodes(buf,"127.0.0.1",0,"http://localhost",1,1,128,m1,m2);
				addNode(buf,"POINTCLOUD");
				updatePointCloudNodes(buf,"127.0.0.1",0,"http://localhost",0,0,0,0);
			}
			if (ch=='d' || ch=='D')
			{
				deleteRocketBoxAvatar("avatar");
			}
			if (ch=='r' || ch=='R')
			{
				static int k = 0.f;
				k = k + 0.01f;
				//updateRocketBoxAvatar("avatar","0",j,-1.18448e-015,-2.70976e-008,0,0.707106,-3.09086e-008,k);
				updateRocketBoxAvatar("avatar","0",j,-1.18448e-015,-2.70976e-008,0,0.707106,-3.09086e-008,k);
			}
			if (ch=='a' || ch=='A')
			{
				char connectedclients[1024];
				char *guid, *node, *tmpid, *nodeid;
				char allnodes[2048], node_info[256];
				int loop = 0;
				float avdata[1024];
				char nodes[512];
				float data[7] = {0.f};
				///Reading AVATAR data
				///get information on all connected clients. use to populate a drop-down combo box for example.
				/// - Use getPeersID(char *peers_info) or getPeersInfo(char *peers_info);
				/// - getPeersID(allnodes);
				/// - getPeersInfo(allnodes);
				printf("number of connected clients %i\n",getPeersID(connectedclients));
				//printf("connected clients (getPeersID): %s\n",connectedclients);
				for (guid = strtok (connectedclients, ";"); guid != NULL;
				   guid = strtok (guid + strlen (guid) + 1, ";"))
				{
					strncpy (iddata, guid, sizeof (iddata));
					printf ("GUID: %s\n", iddata);
					///for each connected guid in the database, get the nodes information.
					/// - getNodesInfo(char peer_guid[], char *nodes_info)
					/// - geNodesInfo(iddata,allnodes);
					printf("Number of nodes %i\n",getNodesInfo(iddata,allnodes));
					for (node = strtok (allnodes, ";"); node != NULL;
					   node = strtok (node + strlen (node) + 1, ";"))
					{
						strncpy (node_info, node, sizeof (node_info));
						printf (" Node: %s\n", node_info);
						for (tmpid = strtok (node_info, ","); tmpid != NULL;
						   tmpid = strtok (tmpid + strlen (tmpid) + 1, ","))
						{
							static char avid[256];
							switch (loop) 
							{
							case 0:// first token is the avatar id
								printf ("  Avatar id: %s\n", tmpid);
								strncpy(avid,tmpid,sizeof(avid));
								break;
							case 2: //third token is the TYPE
								if (!strcmp(tmpid,"AVATAR")) //only process if type is AVATAR
								{
									int c = 0;
									///pass iddata and avid to fetch data of all AVATAR nodes of the client as a pointer
									/// - int count = getAvatarData(iddata,nodes,client_info); 
									int count = getAvatarData(iddata,avid,nodes,avdata);
									for (nodeid = strtok (nodes, ","); nodeid != NULL;
									   nodeid = strtok (nodeid + strlen (nodeid) + 1, ","))
									{
										printf("   %s - %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",nodeid,
											avdata[0+(c*7)],avdata[1+(c*7)],avdata[2+(c*7)],avdata[3+(c*7)],
											avdata[4+(c*7)],avdata[5+(c*7)],avdata[6+(c*7)]);
										c++;
									}
									///OR
									///get specific AVATAR node data in local coordinates using
									/// - getAvatarSpecificData(iddata,avid,"0",data);
									if (getAvatarSpecificData(iddata,avid,"0",data))
										printf("   Local-coord data for 0: %.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
									///OR
									///get specific AVATAR node data in world coordinates using
									/// - getAvatarSpecificGlobalData(iddata,avid,"85",data);
									if (getAvatarSpecificGlobalData(iddata,avid,"85",data))
										printf("   World-coord data for 85: %.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
									///to get Avatar Name
									///use getAvatarName(...)
									getAvatarName(iddata,avid,fname,lname);
									printf("   Avatar Name: %s %s\n",fname,lname);
								} else 
									printf("   Not AVATAR\n");
								break;
							}
							loop++;
						}
						loop=0;
					}
				}
			}
			if (ch=='h' || ch=='H')
			{
				///Reading other types of data (e.g. GENERIC)
				///get information on all connected clients.
				/// - Use getPeersID(char *peers_info) or getPeersInfo(char *peers_info);
				/// - getPeersID(allnodes);
				/// - getPeersInfo(allnodes);
				char connectedclients[1024];
				char *guid, *node, *tmpid, *nodeid;
				char allnodes[2048], node_info[256];
				int loop = 0;
				char nodes[512];
				float data[7] = {0.f};
				printf("number of connected clients %i\n",getPeersID(connectedclients));
				//printf("connected clients (getPeersID): %s\n",connectedclients);
				for (guid = strtok (connectedclients, ";"); guid != NULL;
				   guid = strtok (guid + strlen (guid) + 1, ";"))
				{
					strncpy (iddata, guid, sizeof (iddata));
					printf ("GUID: %s\n", iddata);
					///for each connected guid in the database, get the nodes information.
					/// - getNodesInfo(char peer_guid[], char *nodes_info)
					/// - geNodesInfo(iddata,allnodes);
					printf("Number of nodes %i\n",getNodesInfo(iddata,allnodes));
					for (node = strtok (allnodes, ";"); node != NULL;
					   node = strtok (node + strlen (node) + 1, ";"))
					{
						strncpy (node_info, node, sizeof (node_info));
						printf (" Node: %s\n", node_info);
						for (tmpid = strtok (node_info, ","); tmpid != NULL;
						   tmpid = strtok (tmpid + strlen (tmpid) + 1, ","))
						{
							static char genericid[256];
							switch (loop) 
							{
							case 1:// second token is the node_id 
								printf ("  Generic id: %s\n", tmpid);
								strncpy(genericid,tmpid,sizeof(genericid));
								break;
							case 2: //third token is the TYPE
								if (!strcmp(tmpid,"GENERIC")) //only process if type is GENERIC
								{
									///pass iddata and genericid to fetch data of all GENERIC nodes of the client
									/// - int count = getGenericData(iddata,void pointer,pointer size); 
									int datasize; 
									void *receiveddata;
									if(!(receiveddata = malloc(1024))) {
										perror("failed to allocate buffer");
										return 0;
									}
									if (getGenericData(iddata,genericid,receiveddata,&datasize))
										printf("   generic %s - %s,%d,%.3f\n",genericid,((mystruct *)receiveddata)->name,((mystruct *)receiveddata)->x,((mystruct *)receiveddata)->myfloat);
								} else 
									printf("   Not GENERIC\n");
								break;
							}
							loop++;
						}
						loop=0;
					}
				}
			}
			if (ch=='f' || ch=='F')
			{
				///Reading other types of data (e.g. FACIAL)
				///get information on all connected clients.
				/// - Use getPeersID(char *peers_info) or getPeersInfo(char *peers_info);
				/// - getPeersID(allnodes);
				/// - getPeersInfo(allnodes);
				char connectedclients[1024];
				char *guid, *node, *tmpid, *nodeid;
				char allnodes[2048], node_info[256];
				int loop = 0;
				char nodes[512];
				float data[7] = {0.f};
				printf("number of connected clients %i\n",getPeersID(connectedclients));
				//printf("connected clients (getPeersID): %s\n",connectedclients);
				for (guid = strtok (connectedclients, ";"); guid != NULL;
				   guid = strtok (guid + strlen (guid) + 1, ";"))
				{
					strncpy (iddata, guid, sizeof (iddata));
					printf ("GUID: %s\n", iddata);
					///for each connected guid in the database, get the nodes information.
					/// - getNodesInfo(char peer_guid[], char *nodes_info)
					/// - geNodesInfo(iddata,allnodes);
					printf("Number of nodes %i\n",getNodesInfo(iddata,allnodes));
					for (node = strtok (allnodes, ";"); node != NULL;
					   node = strtok (node + strlen (node) + 1, ";"))
					{
						strncpy (node_info, node, sizeof (node_info));
						printf (" Node: %s\n", node_info);
						for (tmpid = strtok (node_info, ","); tmpid != NULL;
						   tmpid = strtok (tmpid + strlen (tmpid) + 1, ","))
						{
							static char faceid[256];
							switch (loop) 
							{
							case 1:// second token is the node_id 
								printf ("  Facial id: %s\n", tmpid);
								strncpy(faceid,tmpid,sizeof(faceid));
								break;
							case 2: //third token is the TYPE
								if (!strcmp(tmpid,"FACIAL")) //only process if type is FACIAL
								{
									int c = 0;
									///pass iddata and faceid to fetch data of all AVATAR nodes of the client as a pointer
									/// - int count = getFacialData(iddata,nodes,blink,smile,frown,o,e,p); 
									bool blink[10]; 
									float smile[10], frown[10], o[10], e[10], p[10];
									int count = getFacialData(iddata,nodes,blink,smile,frown,o,e,p);
									for (nodeid = strtok (nodes, ","); nodeid != NULL;
									   nodeid = strtok (nodeid + strlen (nodeid) + 1, ","))
									{
										printf("   %s - %d,%.3f,%.3f,%.3f,%.3f,%.3f\n",nodeid,blink[c],smile[c],frown[c],o[c],e[c],p[c]);
										c++;
									}
									///OR
									///get specific FACIAL node data using
									/// - getFacialSpecificData(iddata,faceid,data,blink,smile,frown,o,e,p);
									bool blink1=0; 
									float smile1=0.f, frown1=0.f, o1=0.f, e1=0.f, p1=0.f;
									if (getFacialSpecificData(iddata,faceid,&blink1,&smile1,&frown1,&o1,&e1,&p1))
										printf("   Specific data for %s: %d,%.3f,%.3f,%.3f,%.3f,%.3f\n",faceid,blink1,smile1,frown1,o1,e1,p1);
								} else 
									printf("   Not FACIAL\n");
								break;
							}
							loop++;
						}
						loop=0;
					}
				}
			}
			if (ch=='1')
			{
				///Reading other types of data (e.g. AUDIO)
				///get information on all connected clients.
				/// - Use getPeersID(char *peers_info) or getPeersInfo(char *peers_info);
				/// - getPeersID(allnodes);
				/// - getPeersInfo(allnodes);
				char connectedclients[1024];
				char *guid, *node, *tmpid, *nodeid;
				char allnodes[2048], node_info[256];
				int loop = 0;
				char nodes[512];
				float data[7] = {0.f};
				printf("number of connected clients %i\n",getPeersID(connectedclients));
				//printf("connected clients (getPeersID): %s\n",connectedclients);
				for (guid = strtok (connectedclients, ";"); guid != NULL;
				   guid = strtok (guid + strlen (guid) + 1, ";"))
				{
					strncpy (iddata, guid, sizeof (iddata));
					printf ("GUID: %s\n", iddata);
					///for each connected guid in the database, get the nodes information.
					/// - getNodesInfo(char peer_guid[], char *nodes_info)
					/// - geNodesInfo(iddata,allnodes);
					printf("Number of nodes %i\n",getNodesInfo(iddata,allnodes));
					for (node = strtok (allnodes, ";"); node != NULL;
					   node = strtok (node + strlen (node) + 1, ";"))
					{
						strncpy (node_info, node, sizeof (node_info));
						printf (" Node: %s\n", node_info);
						for (tmpid = strtok (node_info, ","); tmpid != NULL;
						   tmpid = strtok (tmpid + strlen (tmpid) + 1, ","))
						{
							static char audioid[256];
							switch (loop) 
							{
							case 1:// second token is the node_id 
								printf ("  Audio id: %s\n", tmpid);
								strncpy(audioid,tmpid,sizeof(audioid));
								break;
							case 2: //third token is the TYPE
								if (!strcmp(tmpid,"AUDIO")) //only process if type is AUDIO
								{
									int c = 0;
									///pass iddata and audioid to fetch data of all AUDIO nodes of the client as a pointer
									/// - int count = getAudioData(iddata,nodes,audioserver,audioport,url,config); 
									char audioserver[32], url[256], config[128];
									int audioport; 
									/*int count = getAudioData(iddata,nodes,audioserver,audioport,url,config);
									for (nodeid = strtok (nodes, ","); nodeid != NULL;
									   nodeid = strtok (nodeid + strlen (nodeid) + 1, ","))
									{
										printf("   %s - %s,%s,%d,%s,%s\n",nodeid,audioserver,audioport[c],url,config);
										c++;
									}
									///OR
									///get specific AUDIO node data using
									/// - getAudioSpecificData(iddata,audioid,audioserver,audioport,url,config);*/
									if (getAudioSpecificData(iddata,audioid,audioserver,&audioport,url,config))
										printf("   Specific data for %s: %s,%d,%s,%s\n",audioid,audioserver,audioport,url,config);
								} else 
									printf("   Not AUDIO\n");
								break;
							}
							loop++;
						}
						loop=0;
					}
				}
			}
			if (ch=='2')
			{
				///Reading other types of data (e.g. VIDEO)
				///get information on all connected clients.
				/// - Use getPeersID(char *peers_info) or getPeersInfo(char *peers_info);
				/// - getPeersID(allnodes);
				/// - getPeersInfo(allnodes);
				char connectedclients[1024];
				char *guid, *node, *tmpid, *nodeid;
				char allnodes[2048], node_info[256];
				int loop = 0;
				char nodes[512];
				float data[7] = {0.f};
				printf("number of connected clients %i\n",getPeersID(connectedclients));
				//printf("connected clients (getPeersID): %s\n",connectedclients);
				for (guid = strtok (connectedclients, ";"); guid != NULL;
				   guid = strtok (guid + strlen (guid) + 1, ";"))
				{
					strncpy (iddata, guid, sizeof (iddata));
					printf ("GUID: %s\n", iddata);
					///for each connected guid in the database, get the nodes information.
					/// - getNodesInfo(char peer_guid[], char *nodes_info)
					/// - geNodesInfo(iddata,allnodes);
					printf("Number of nodes %i\n",getNodesInfo(iddata,allnodes));
					for (node = strtok (allnodes, ";"); node != NULL;
					   node = strtok (node + strlen (node) + 1, ";"))
					{
						strncpy (node_info, node, sizeof (node_info));
						printf (" Node: %s\n", node_info);
						for (tmpid = strtok (node_info, ","); tmpid != NULL;
						   tmpid = strtok (tmpid + strlen (tmpid) + 1, ","))
						{
							static char videoid[256];
							switch (loop) 
							{
							case 1:// second token is the node_id 
								printf ("  Video id: %s\n", tmpid);
								strncpy(videoid,tmpid,sizeof(videoid));
								break;
							case 2: //third token is the TYPE
								if (!strcmp(tmpid,"VIDEO")) //only process if type is VIDEO
								{
									int c = 0;
									///pass iddata and videoid to fetch data of all VIDEO nodes of the client as a pointer
									/// - int count = getVideoData(iddata,nodes,videoserver,videoport,url,frame_width,frame_height,bandwidth,calibration1,calibration2); 
									char videoserver[32], url[256], config[128];
									int videoport, width, height = 0;
									double bandwidth;
									float calibration1[9], calibration2[16] = {0};
									/*int count = getVideoData(iddata,nodes,videoserver,videoport,url,frame_width,frame_height,bandwidth,calibration1,calibration2);
									for (nodeid = strtok (nodes, ","); nodeid != NULL;
									   nodeid = strtok (nodeid + strlen (nodeid) + 1, ","))
									{
										printf("   %s - %s,%s,%d,%s,%s\n",nodeid,videoserver,videoport[c],url,frame_width,frame_height,bandwidth,calibration1,calibration2);
										c++;
									}
									///OR
									///get specific VIDEO node data using
									/// - getVideoSpecificData(iddata,videoid,videoserver,videoport,url,frame_width,frame_height,bandwidth,camera_calibration1,camera_calibration2);*/
									if (getVideoSpecificData(iddata,videoid,videoserver,&videoport,url,&width,&height,&bandwidth,calibration1,calibration2))
										printf("   Specific data for %s: %s,%d,%s,%d,%d,%.3f,%.3f,%.3f\n",videoid,videoserver,videoport,url,width,height,bandwidth,calibration1[0],calibration2[0]);
								} else 
									printf("   Not VIDEO\n");
								break;
							}
							loop++;
						}
						loop=0;
					}
				}
			}
		}
	}
	///remove all nodes and clean up on exit 
	/// - removeAllNodes();
	/// - stop();
	removeAllNodes();
	stop();
	exitlibrary();
}