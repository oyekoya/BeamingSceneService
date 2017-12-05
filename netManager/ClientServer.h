// Uses ReplicaManager 3: A system to automatically create, destroy, and serialize objects

#include "StringTable.h"
#include "RakPeerInterface.h"
#include "RakNetworkFactory.h"
#include <stdio.h>
#include "Kbhit.h"
#include <string.h>
#include "BitStream.h"
#include "MessageIdentifiers.h"
#include "ReplicaManager3.h"
#include "NetworkIDManager.h"
#include "RakSleep.h"
#include "FormatString.h"
#include "RakString.h"
#include "GetTime.h"
#include "SocketLayer.h"
#include "Getche.h"
#include <vector>
#include "PacketFileLogger.h"
#include "utilMath.h"
#include <iostream>
#include <string>
#include <map>
#include <math.h>
#if defined(__WIN32__) || defined(WIN32)
	#include <windows.h>
#else
	#include <cstdio>
#endif

//#define LOG_ENABLED

enum
{
	CLIENT,
	SERVER
} topology;

// ReplicaManager3 is in the namespace RakNet
using namespace RakNet;

class BeamingUser;
class BeamingAvatarJointReplica;
class BeamingEmotionReplica;
class BeamingFacialReplica;
class BeamingTactileReplica;
class BeamingRobotReplica;
class BeamingRoom;
class BeamingVideoReplica;
class BeamingObjectReplica;
class BeamingAudioReplica;
class BeamingPointCloudReplica;
class BeamingGenericReplica;

std::map<RakNetGUID, std::vector<std::string> > new_clients; //for client checkstatus() function - deprecated
//NOTE: Perhaps using different maps may not be the most efficient approach, 
//however, using one map for all replicas will require typecasting which can cause type errors or affect performance 
std::map<RakNetGUID, std::vector<BeamingAvatarJointReplica*> > avatar_joint_replicas; //lookup by guid, to get pointers for avatar joint replicas
std::map<RakNetGUID, std::vector<BeamingEmotionReplica*> > emotion_replicas; //lookup by guid, to get pointers for emotion replicas
std::map<RakNetGUID, std::vector<BeamingFacialReplica*> > facial_replicas; //lookup by guid, to get pointers for facial replicas
std::map<RakNetGUID, std::vector<BeamingTactileReplica*> > tactile_replicas; //lookup by guid, to get pointers for tactile replicas
std::map<RakNetGUID, std::vector<BeamingRobotReplica*> > robot_replicas; //lookup by guid, to get pointers for robot replicas
std::map<RakNetGUID, std::vector<BeamingVideoReplica*> > video_replicas; //lookup by guid, to get pointers for video replicas
std::map<RakNetGUID, std::vector<BeamingObjectReplica*> > object_replicas; //lookup by guid, to get pointers for object replicas
std::map<RakNetGUID, std::vector<BeamingAudioReplica*> > audio_replicas; //lookup by guid, to get pointers for audio replicas
std::map<RakNetGUID, std::vector<BeamingPointCloudReplica*> > point_cloud_replicas; //lookup by guid, to get pointers for point cloud replicas
std::map<RakNetGUID, std::vector<BeamingGenericReplica*> > generic_replicas; //lookup by guid, to get pointers for generic replicas
struct node_info {
	std::string name;
	std::string type;
	char *peername;
	char *peertype;
	char *peercfg;
	char *ipport;
};
std::map<std::string, std::vector<node_info*> > nodes_map; //lookup by guid, to get node_info
PacketFileLogger logfileHandler;
bool logging=false;


///User Replica
class BeamingUser : public Replica3
{
public:
	void PrintOutput(RakNet::BitStream *bs)
	{
		if (bs->GetNumberOfBitsUsed()==0)
			return;
		RakNet::RakString rakString;
		bs->Read(rakString);
		printf("Receive: %s\n", rakString.C_String());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{ 
		constructionBitstream->Write(clientname);
		constructionBitstream->Write(clientType);
		constructionBitstream->Write(clientConfig);
		constructionBitstream->Write(ip_port);
		strcpy(username,"");
#if defined(__WIN32__) || defined(WIN32)
	  DWORD size;
	  (GetUserNameA( username, &size ));
#else
		FILE *name;
		name = popen("whoami", "r");
		fgets(username, sizeof(username), name);
		pclose(name);
#endif
		constructionBitstream->Write(username);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) { 
		constructionBitstream->Read(clientname);
		constructionBitstream->Read(clientType);
		constructionBitstream->Read(clientConfig);
		constructionBitstream->Read(ip_port);
		constructionBitstream->Read(username);
		if (topology == SERVER)
		{
			printf("Connection from user: %s, ip: %s\n",username,sourceConnection->GetSystemAddress().ToString());
			sprintf(ip_port,"%s",sourceConnection->GetSystemAddress().ToString());

		} else {
			printf("Connection from user: %s\n",username); }
		new_clients[this->GetCreatingSystemGUID()].push_back(clientname);
		new_clients[this->GetCreatingSystemGUID()].push_back(clientType);
		new_clients[this->GetCreatingSystemGUID()].push_back(clientConfig);
		return true; 
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection) { }
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) { return true; }
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {	delete this; }
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) { }
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{ }
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) { }
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{ }
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) { }
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3) {
			return QueryConstruction_ClientConstruction(destinationConnection);
	}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {
			return QueryRemoteConstruction_ClientConstruction(sourceConnection);
	}

	virtual RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {
			return QuerySerialization_ClientSerializable(destinationConnection);
	}
	virtual RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const {
		if (topology==SERVER)
			return QueryActionOnPopConnection_Server(droppedConnection);
		else
			return QueryActionOnPopConnection_Client(droppedConnection);
	}

	char clientname[128];
	char clientConfig[128];
	char clientType[128];
	char username[128];
	char logstr[1024];
	char localtime[128];
	char ip_port[128];

	BeamingUser() { }

	virtual ~BeamingUser() { 
		if (topology==SERVER)
			BroadcastDestruction();
	}
};


///Avatar Joint Replica
class BeamingAvatarJointReplica : public BeamingUser
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("AVATAR");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(avatarjoint_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(avatarjoint_id);
		constructionBitstream->Write(parentbone);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(avatarjoint_id);
		constructionBitstream->Read(parentbone);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,avatarjoint_id);
		if (topology == CLIENT)
		{
			avatar_joint_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=avatarjoint_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),avatar_joint_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),avatar_joint_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(avatarjoint_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			//remove avatar joints from avatar_joint_replicas
			for ( std::vector<BeamingAvatarJointReplica*>::iterator cIter = avatar_joint_replicas[this->GetCreatingSystemGUID()].begin(); cIter!=avatar_joint_replicas[this->GetCreatingSystemGUID()].end(); cIter++ ) {
				if (( strcmp((*cIter)->avatarjoint_id,avatarjoint_id) == 0 ) &&  ( strcmp((*cIter)->clientname,clientname) == 0 )) {
					avatar_joint_replicas[this->GetCreatingSystemGUID()].erase(cIter);
					break;
				}
			}
			//remove avatar joints from nodes_map
			for ( std::vector<node_info*>::iterator cIter = nodes_map[this->GetCreatingSystemGUID().ToString()].begin(); cIter!=nodes_map[this->GetCreatingSystemGUID().ToString()].end(); cIter++ ) {
				if (( strcmp((*cIter)->name.c_str(),avatarjoint_id) == 0 ) &&  ( strcmp((*cIter)->peername,clientname) == 0 )) {
					nodes_map[this->GetCreatingSystemGUID().ToString()].erase(cIter);
					break;
				}
			}
			if (nodes_map[this->GetCreatingSystemGUID().ToString()].empty())
			{	nodes_map.erase(this->GetCreatingSystemGUID().ToString());	}
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),avatar_joint_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),avatar_joint_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingUser::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&fname,sizeof(fname));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&lname,sizeof(lname));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&position,sizeof(position));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&orientation,sizeof(orientation));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingUser::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&fname,sizeof(fname));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&lname,sizeof(lname));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&position,sizeof(position));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&orientation,sizeof(orientation));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,avatarjoint_id,position.x,position.y,position.z,orientation.x,orientation.y,orientation.z,orientation.w);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,avatarjoint_id,position.x,position.y,position.z,orientation.x,orientation.y,orientation.z,orientation.w);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(avatarjoint_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(avatarjoint_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char avatarjoint_id[128];
	CVec3 position;
	CQuat orientation;
	node_info *thisnode;
	RakNet::RakString parentbone;
	char fname[128];
	char lname[128];

	BeamingAvatarJointReplica() { }

	virtual ~BeamingAvatarJointReplica() { }
};


///Facial Replica
class BeamingFacialReplica : public BeamingUser
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("FACIAL");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(facial_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(facial_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(facial_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,facial_id);
		if (topology == CLIENT)
		{
			facial_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=facial_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),facial_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),facial_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(facial_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			facial_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
			nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),facial_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),facial_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingUser::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&blink,sizeof(blink));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&smile,sizeof(smile));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&frown,sizeof(frown));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&o,sizeof(o));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&e,sizeof(e));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&p,sizeof(p));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingUser::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&blink,sizeof(blink));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&smile,sizeof(smile));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&frown,sizeof(frown));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&o,sizeof(o));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&e,sizeof(e));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&p,sizeof(p));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %d %.3f %.3f %.3f %.3f %.3f\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,facial_id,blink,smile,frown,o,e,p);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%d;%.3f;%.3f;%.3f;%.3f;%.3f",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,facial_id,blink,smile,frown,o,e,p);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(facial_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(facial_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char facial_id[128];
	bool blink;
	float smile;
	float frown;
	float o;
	float e;
	float p;
	node_info *thisnode;

	BeamingFacialReplica() { }

	virtual ~BeamingFacialReplica() { }
};


///Emotion Replica - Affective State
class BeamingEmotionReplica : public BeamingUser
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("EMOTION");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(emotion_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(emotion_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(emotion_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,emotion_id);
		if (topology == CLIENT)
		{
			emotion_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=emotion_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),emotion_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),emotion_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(emotion_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			emotion_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
			nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),emotion_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),emotion_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingUser::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&valence,sizeof(valence));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&arousal,sizeof(arousal));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&misc,sizeof(misc));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingUser::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&valence,sizeof(valence));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&arousal,sizeof(arousal));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&misc,sizeof(misc));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %f %f %f\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,emotion_id,valence,arousal,misc);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%f;%f;%f",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,emotion_id,valence,arousal,misc);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(emotion_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(emotion_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char emotion_id[128];
	double valence;
	double arousal;
	double misc;
	node_info *thisnode;

	BeamingEmotionReplica() { }

	virtual ~BeamingEmotionReplica() { }
};


///Tactile Replica
class BeamingTactileReplica : public BeamingUser
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("TACTILE");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(tactile_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(tactile_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(tactile_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,tactile_id);
		if (topology == CLIENT)
		{
			tactile_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=tactile_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),tactile_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),tactile_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(tactile_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			tactile_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
			nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),tactile_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),tactile_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingUser::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&duration,sizeof(duration));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&intensity,sizeof(intensity));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&temperature,sizeof(temperature));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingUser::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&duration,sizeof(duration));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&intensity,sizeof(intensity));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&temperature,sizeof(temperature));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %f %.3f %.3f\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,tactile_id,duration,intensity,temperature);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%f;%.3f;%.3f",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,tactile_id,duration,intensity,temperature);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(tactile_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(tactile_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char tactile_id[128];
	double duration;
	float intensity;
	float temperature;
	node_info *thisnode;

	BeamingTactileReplica() { }

	virtual ~BeamingTactileReplica() { }
};


///Kali-Type Robot Replica 
class BeamingRobotReplica : public BeamingUser
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("ROBOT");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(robot_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(robot_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(robot_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,robot_id);
		if (topology == CLIENT)
		{
			robot_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=robot_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),robot_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),robot_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(robot_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			robot_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
			nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),robot_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),robot_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingUser::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&type,sizeof(type));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&details,sizeof(details));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&freespace,sizeof(freespace));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&position,sizeof(position));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&orientation,sizeof(orientation));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&time_remain,sizeof(time_remain));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&contact_type,sizeof(contact_type));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingUser::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&type,sizeof(type));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&details,sizeof(details));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&freespace,sizeof(freespace));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&position,sizeof(position));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&orientation,sizeof(orientation));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&time_remain,sizeof(time_remain));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&contact_type,sizeof(contact_type));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %i %i %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %i\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,robot_id,type,details,freespace,position.x,position.y,position.z,orientation.x,orientation.y,orientation.z,orientation.w,time_remain,contact_type);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%i;%i;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%i",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,robot_id,type,details,freespace,position.x,position.y,position.z,orientation.x,orientation.y,orientation.z,orientation.w,time_remain,contact_type);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(robot_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(robot_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char robot_id[128];
	int type;
	int details;
	float freespace;
	CVec3 position;
	CQuat orientation;
	float time_remain;
	int contact_type;
	node_info *thisnode;

	BeamingRobotReplica() { }

	virtual ~BeamingRobotReplica() { }
};

///Generic Replica (to enable writing of generic replica up to 1024 bytes)
class BeamingGenericReplica : public BeamingUser
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("GENERIC");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(generic_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(generic_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(generic_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,generic_id);
		if (topology == CLIENT)
		{
			generic_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=generic_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),generic_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),generic_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingUser::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(generic_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingUser::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			//remove generic from generic_replicas
			for ( std::vector<BeamingGenericReplica*>::iterator cIter = generic_replicas[this->GetCreatingSystemGUID()].begin(); cIter!=generic_replicas[this->GetCreatingSystemGUID()].end(); cIter++ ) {
				if (( strcmp((*cIter)->generic_id,generic_id) == 0 ) &&  ( strcmp((*cIter)->clientname,clientname) == 0 )) {
					generic_replicas[this->GetCreatingSystemGUID()].erase(cIter);
					break;
				}
			}
			//remove generic data from nodes_map
			for ( std::vector<node_info*>::iterator cIter = nodes_map[this->GetCreatingSystemGUID().ToString()].begin(); cIter!=nodes_map[this->GetCreatingSystemGUID().ToString()].end(); cIter++ ) {
				if (( strcmp((*cIter)->name.c_str(),generic_id) == 0 ) &&  ( strcmp((*cIter)->peername,clientname) == 0 )) {
					nodes_map[this->GetCreatingSystemGUID().ToString()].erase(cIter);
					break;
				}
			}
			if (nodes_map[this->GetCreatingSystemGUID().ToString()].empty())
			{	nodes_map.erase(this->GetCreatingSystemGUID().ToString());	}
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),generic_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),generic_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingUser::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteBits((const unsigned char*)anydata,1024*8);
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&datasize,sizeof(datasize));
		/*//begin printf
		static int y = 0;
		struct mystruct{
			int x;
			char name[28];
			float myfloat;
		} st; 	
		if (y == 100) {
			printf("sending %s, %d, %.3f ...\n",((mystruct *)anydata)->name,((mystruct *)anydata)->x,((mystruct *)anydata)->myfloat);
			y = 0;
		}
		y++;
		//end printf*/
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingUser::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadBits((unsigned char*)anydata,1024*8);
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&datasize,sizeof(datasize));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,generic_id/*,anydata*/);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,generic_id/*,anydata*/);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
		/*//begin printf
		struct mystruct{
			int x;
			char name[28];
			float myfloat;
		} st; 	
		printf("receiving %s, %d, %.3f ...\n",((mystruct *)anydata)->name,((mystruct *)anydata)->x,((mystruct *)anydata)->myfloat);
		//end printf*/
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(generic_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(generic_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char generic_id[128];
	node_info *thisnode;
	//std::string parentbone;
	void *anydata;
	int datasize;

	BeamingGenericReplica() : anydata(malloc(1024)) { }

	virtual ~BeamingGenericReplica() { }
};



///Room Replica (destination)
class BeamingRoom : public Replica3
{
public:
	void PrintOutput(RakNet::BitStream *bs)
	{
		if (bs->GetNumberOfBitsUsed()==0)
			return;
		RakNet::RakString rakString;
		bs->Read(rakString);
		printf("Receive: %s\n", rakString.C_String());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{ 
		constructionBitstream->Write(clientname);
		constructionBitstream->Write(clientType);
		constructionBitstream->Write(clientConfig);
		constructionBitstream->Write(ip_port);
		gethostname(hostname,128);
		constructionBitstream->Write(hostname);
		constructionBitstream->Write(file_url);
		constructionBitstream->Write(host);
		constructionBitstream->Write(port);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) { 
		constructionBitstream->Read(clientname);
		constructionBitstream->Read(clientType);
		constructionBitstream->Read(clientConfig);
		constructionBitstream->Read(ip_port);
		constructionBitstream->Read(hostname);
		if (topology == SERVER)
		{
			printf("Connection from host: %s, ip: %s\n",hostname,sourceConnection->GetSystemAddress().ToString());
			sprintf(ip_port,"%s",sourceConnection->GetSystemAddress().ToString());

		} else {
			printf("Connection from host: %s\n",hostname); }
		constructionBitstream->Read(file_url);
		constructionBitstream->Read(host);
		constructionBitstream->Read(port);
		new_clients[this->GetCreatingSystemGUID()].push_back(clientname);
		new_clients[this->GetCreatingSystemGUID()].push_back(clientType);
		new_clients[this->GetCreatingSystemGUID()].push_back(clientConfig);
		return true; 
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection) { }
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) { return true; }
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {	delete this; }
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) { }
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{ }
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) { }
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{ }
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) { }
	virtual RM3ConstructionState QueryConstruction(RakNet::Connection_RM3 *destinationConnection, ReplicaManager3 *replicaManager3) {
			return QueryConstruction_ClientConstruction(destinationConnection);
	}
	virtual bool QueryRemoteConstruction(RakNet::Connection_RM3 *sourceConnection) {
			return QueryRemoteConstruction_ClientConstruction(sourceConnection);
	}

	virtual RM3QuerySerializationResult QuerySerialization(RakNet::Connection_RM3 *destinationConnection) {
			return QuerySerialization_ClientSerializable(destinationConnection);
	}
	virtual RM3ActionOnPopConnection QueryActionOnPopConnection(RakNet::Connection_RM3 *droppedConnection) const {
		if (topology==SERVER)
			return QueryActionOnPopConnection_Server(droppedConnection);
		else
			return QueryActionOnPopConnection_Client(droppedConnection);
	}

	char clientname[128];
	char clientConfig[128];
	char clientType[128];
	char hostname[128];
	char host[128];
	int port;
	char file_url[128];
	char logstr[1024];
	char localtime[128];
	char ip_port[128];

	BeamingRoom() { }

	virtual ~BeamingRoom() { 
		if (topology==SERVER)
			BroadcastDestruction();
	}
};



///Object Replica - 3D Models
class BeamingObjectReplica : public BeamingRoom
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("OBJECT");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(object_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(object_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(object_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,object_id);
		if (topology == CLIENT)
		{
			object_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=object_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),object_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),object_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(object_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			object_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
			nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),object_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),object_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingRoom::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&position,sizeof(position));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&orientation,sizeof(orientation));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingRoom::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&position,sizeof(position));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&orientation,sizeof(orientation));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,object_id,position.x,position.y,position.z,orientation.x,orientation.y,orientation.z,orientation.w);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f;%.3f",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,object_id,position.x,position.y,position.z,orientation.x,orientation.y,orientation.z,orientation.w);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(object_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(object_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char object_id[128];
	CVec3 position;
	CQuat orientation;
	node_info *thisnode;

	BeamingObjectReplica() { }

	virtual ~BeamingObjectReplica() { }
};


///Video Replica
class BeamingVideoReplica : public BeamingRoom
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("VIDEO");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(video_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(video_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(video_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,video_id);
		if (topology == CLIENT)
		{
			video_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=video_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),video_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),video_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(video_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			video_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
			nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),video_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),video_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingRoom::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&frame_width,sizeof(frame_width));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&frame_height,sizeof(frame_height));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&bandwidth,sizeof(bandwidth));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&calibration1,sizeof(calibration1));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&calibration2,sizeof(calibration2));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingRoom::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&frame_width,sizeof(frame_width));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&frame_height,sizeof(frame_height));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&bandwidth,sizeof(bandwidth));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&calibration1,sizeof(calibration1));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&calibration2,sizeof(calibration2));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %i %i %f\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,video_id,frame_width,frame_height,bandwidth);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%i;%i;%f",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,video_id,frame_width,frame_height,bandwidth);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(video_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(video_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char video_id[128];
	int frame_width;
	int frame_height;
	double bandwidth;
	CMatrix calibration1;
	CMatrix calibration2;
	node_info *thisnode;

	BeamingVideoReplica() { }

	virtual ~BeamingVideoReplica() { }
};


///Audio Replica
class BeamingAudioReplica : public BeamingRoom
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("AUDIO");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(audio_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(audio_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(audio_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,audio_id);
		if (topology == CLIENT)
		{
			audio_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=audio_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),audio_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),audio_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(audio_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		audio_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
		nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),audio_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),audio_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingRoom::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&config,sizeof(config));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingRoom::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&config,sizeof(config));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %s\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,audio_id,config);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%s",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,audio_id,config);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(audio_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(audio_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char audio_id[128];
	char config[128];
	node_info *thisnode;

	BeamingAudioReplica() { }

	virtual ~BeamingAudioReplica() { }
};


///Point Cloud Replica
class BeamingPointCloudReplica : public BeamingRoom
{
	virtual RakNet::RakString GetName(void) const {return RakNet::RakString("POINTCLOUD");}
	virtual void WriteAllocationID(RakNet::BitStream *allocationIdBitstream) const {
		allocationIdBitstream->Write(GetName());
	}
	virtual void SerializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeConstruction(constructionBitstream, destinationConnection);
		constructionBitstream->Write(GetName() + RakNet::RakString(pointcloud_id) + RakNet::RakString(" created for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
		constructionBitstream->Write(pointcloud_id);
	}
	virtual bool DeserializeConstruction(RakNet::BitStream *constructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeConstruction(constructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(constructionBitstream);
		constructionBitstream->Read(pointcloud_id);
		printf("Client: %s, Type: %s, Config: %s, Node: %s\n",clientname,clientType,clientConfig,pointcloud_id);
		if (topology == CLIENT)
		{
			point_cloud_replicas[this->GetCreatingSystemGUID()].push_back(this); //create within map on new client connection
			thisnode = new node_info;
			thisnode->name=pointcloud_id;
			thisnode->type=GetName().C_String();
			thisnode->peername = clientname;
			thisnode->peertype = clientType;
			thisnode->peercfg = clientConfig;
			thisnode->ipport = ip_port;
			nodes_map[this->GetCreatingSystemGUID().ToString()].push_back(thisnode);
		}
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client created; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),point_cloud_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"C;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),point_cloud_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void SerializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *destinationConnection)	{
		BeamingRoom::SerializeDestruction(destructionBitstream, destinationConnection);
		destructionBitstream->Write(GetName() + RakNet::RakString(pointcloud_id) + RakNet::RakString(" removed for guid ") + RakNet::RakString(this->GetCreatingSystemGUID().ToString()));
	}
	virtual bool DeserializeDestruction(RakNet::BitStream *destructionBitstream, RakNet::Connection_RM3 *sourceConnection) {
		if (!BeamingRoom::DeserializeDestruction(destructionBitstream, sourceConnection))
			return false;
		RakNetTimeMS time = RakNet::GetTime();
		PrintOutput(destructionBitstream);
		if (topology == CLIENT)
		{
			point_cloud_replicas.erase(this->GetCreatingSystemGUID()); //erase from map on client disconnection
			nodes_map.erase(this->GetCreatingSystemGUID().ToString());
		}
		new_clients.erase(this->GetCreatingSystemGUID()); //deprecated
		if ((topology==SERVER)&&(logging==true))
		{
			//printf("%"PRINTF_64_BIT_MODIFIER"u: %s client deleted; rem %i\n",(unsigned long long)time,sourceConnection->GetRakNetGUID().ToString(),point_cloud_replicas.size());
			logfileHandler.GetLocalTime(localtime);
			sprintf(logstr,"D;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;0;%i",(unsigned long long)time,localtime,sourceConnection->GetRakNetGUID().ToString(),point_cloud_replicas.size());
			logfileHandler.WriteLog(logstr);
		}
		return true;
	}
	virtual void DeallocReplica(RakNet::Connection_RM3 *sourceConnection) {
		delete this;
	}
	virtual RM3SerializationResult Serialize(SerializeParameters *serializeParameters)	{
		BeamingRoom::Serialize(serializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&size,sizeof(size));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&bandwidth,sizeof(bandwidth));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&RGB_flag,sizeof(RGB_flag));
		serializeParameters->outputBitstream[0].WriteAlignedBytes((const unsigned char*)&quality_level,sizeof(quality_level));
		return RM3SR_BROADCAST_IDENTICALLY;
	}
	virtual void Deserialize(RakNet::DeserializeParameters *deserializeParameters) {
		BeamingRoom::Deserialize(deserializeParameters);
		RakNetTimeMS time = RakNet::GetTime();
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&size,sizeof(size));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&bandwidth,sizeof(bandwidth));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&RGB_flag,sizeof(RGB_flag));
		deserializeParameters->serializationBitstream[0].ReadAlignedBytes((unsigned char*)&quality_level,sizeof(quality_level));
		//printf("%"PRINTF_64_BIT_MODIFIER"u: %s %s %s %s %s - %i %f %i\n",(unsigned long long)time,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,pointcloud_id,size,bandwidth,RGB_flag);
		if ((topology==SERVER)&&(logging==true))//logging on server
		{
				logfileHandler.GetLocalTime(localtime);
				sprintf(logstr,"M;%"PRINTF_64_BIT_MODIFIER"u;%s;%s;%s;%s;%s;%s;%i;%f;%i",(unsigned long long)time,localtime,clientname,this->GetCreatingSystemGUID().ToString(),clientType,clientConfig,pointcloud_id,size,bandwidth,RGB_flag);
				logfileHandler.WriteLog(logstr); //writes additional lines
		}
	}
	virtual void SerializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(pointcloud_id) + RakNet::RakString(" created on server"));
	}
	virtual void DeserializeConstructionRequestAccepted(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *acceptingConnection) {
		PrintOutput(serializationBitstream);
	}
	virtual void SerializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *requestingConnection)	{
		serializationBitstream->Write(GetName() + RakNet::RakString(pointcloud_id) + RakNet::RakString(" not created on server"));
	}
	virtual void DeserializeConstructionRequestRejected(RakNet::BitStream *serializationBitstream, RakNet::Connection_RM3 *rejectingConnection) {
		PrintOutput(serializationBitstream);
	}

public:
	char pointcloud_id[128];
	int size;
	double bandwidth;
	bool RGB_flag;
	int quality_level;
	node_info *thisnode;

	BeamingPointCloudReplica() { }

	virtual ~BeamingPointCloudReplica() { }
};


/// One instance of Connection_RM3 is implicitly created per connection that uses ReplicaManager3. 
class BeamingConnection : public Connection_RM3 {
public:
	BeamingConnection(SystemAddress _systemAddress, RakNetGUID _guid) : Connection_RM3(_systemAddress, _guid) {}
	virtual ~BeamingConnection() {}

	/// Callback used to create objects of different data types
	virtual Replica3 *AllocReplica(RakNet::BitStream *allocationId, ReplicaManager3 *replicaManager3)
	{
		RakNet::RakString typeName;
		allocationId->Read(typeName);
		if (typeName=="AVATAR") return new BeamingAvatarJointReplica;
		if (typeName=="FACIAL") return new BeamingFacialReplica;
		if (typeName=="EMOTION") return new BeamingEmotionReplica;
		if (typeName=="TACTILE") return new BeamingTactileReplica;
		if (typeName=="ROBOT") return new BeamingRobotReplica;
		if (typeName=="OBJECT") return new BeamingObjectReplica;
		if (typeName=="VIDEO") return new BeamingVideoReplica;
		if (typeName=="AUDIO") return new BeamingAudioReplica;
		if (typeName=="POINTCLOUD") return new BeamingPointCloudReplica;
		if (typeName=="GENERIC") return new BeamingGenericReplica;
		return 0;
	}
protected:
};


///Created on intialisation and attached as a plugin
class ReplicaManager3Beaming : public ReplicaManager3
{
	virtual Connection_RM3* AllocConnection(SystemAddress systemAddress, RakNetGUID rakNetGUID) const {
		return new BeamingConnection(systemAddress,rakNetGUID);
	}
	virtual void DeallocConnection(Connection_RM3 *connection) const {
		delete connection;
	}
};



