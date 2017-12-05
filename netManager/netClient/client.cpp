//Client Dll - links to RakNet

#include "client.h"
#include "ClientServer.h"
#include <list>

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

char ch;
SocketDescriptor sd;
char ip[128];
static int SERVER_PORT=12050;
Packet *packet;
bool isconnected=false;
bool connection_lost=false;
std::map<std::string, BeamingDataType> node_type_map;
std::map<std::string, BeamingAvatarJointReplica*> AvatarNodeMap;
std::map<std::string, BeamingEmotionReplica*> EmotionNodeMap;
std::map<std::string, BeamingFacialReplica*> FacialNodeMap;
std::map<std::string, BeamingTactileReplica*> TactileNodeMap;
std::map<std::string, BeamingRobotReplica*> RobotNodeMap;
std::map<std::string, BeamingVideoReplica*> VideoNodeMap;
std::map<std::string, BeamingObjectReplica*> ObjectNodeMap;
std::map<std::string, BeamingAudioReplica*> AudioNodeMap;
std::map<std::string, BeamingPointCloudReplica*> PointCloudNodeMap;
std::map<std::string, BeamingGenericReplica*> GenericNodeMap;
std::string myid, my_config, client_type;
char client_name[128];
bool viewing_only = false;

/// ReplicaManager3 requires NetworkIDManager to lookup pointers from numbers.
NetworkIDManager networkIdManager;
/// Each application has one instance of RakPeerInterface
RakPeerInterface *rakPeer;
/// The system that performs most of our functionality for this client
ReplicaManager3Beaming replicaManager;


#ifdef WIN32
CLIENT_LIB_EXPORT int startclient(char *server_address, int server_port, char *client, char *clienttype, char *config, int viewer, int reliability, int priority, int interval_ms )
#else
int startclient(char *server_address, int server_port, char *client, char *clienttype, char *config, int viewer, int reliability, int priority, int interval_ms)
#endif
{
	strcpy(ip,server_address);
	strcpy(client_name,client);
	SERVER_PORT = server_port;
	rakPeer = RakNetworkFactory::GetRakPeerInterface();
	topology=CLIENT;
	sd.port=0;
	// ObjectMemberRPC, AutoRPC for objects, and ReplicaManager3 require that you call SetNetworkIDManager()
	rakPeer->SetNetworkIDManager(&networkIdManager);
	// The network ID authority is the system that creates the common numerical identifier used to lookup pointers.
	// For client/server this is the server
	// For peer to peer this would be true on every system, and NETWORK_ID_SUPPORTS_PEER_TO_PEER should be defined in RakNetDefines.h
	networkIdManager.SetIsNetworkIDAuthority(false);

	//set reliability and priority - see PacketPriority.h
	//Note that the transmission of one of the three reliable packets types is required for the detection of lost connections. 
	//If you never send reliable packets you need to implement lost connection detection manually.
	replicaManager.SetDefaultPacketReliability((PacketReliability)reliability);
	replicaManager.SetDefaultPacketPriority((PacketPriority)priority);
	replicaManager.SetAutoSerializeInterval(interval_ms);
	//printf("%i, %i, %i\n", reliability, priority, interval_ms);

	// Start RakNet
	rakPeer->Startup(1,10,&sd,1); //isServer ? 32 : 1
	rakPeer->AttachPlugin(&replicaManager);

	rakPeer->Connect(ip,SERVER_PORT,0,0,0);
	printf("Connecting...\n");
	RakSleep(1000);
	check();
	printf("Client %s (%s) is %s\n", client, clienttype, (isconnected)?"connected":"not connected");

	myid = rakPeer->GetGuidFromSystemAddress(UNASSIGNED_SYSTEM_ADDRESS).ToString();
	if (isconnected) {
		printf("Client (%s) GUID is %s \n",myid.c_str(),(viewer)?"viewer only":"bidirectional");
	}
	//set up node_type map
	node_type_map["AVATAR"]=AVATAR;
	node_type_map["FACIAL"]=FACIAL;
	node_type_map["EMOTION"]=EMOTION;
	node_type_map["VIDEO"]=VIDEO;
	node_type_map["TACTILE"]=TACTILE;
	node_type_map["ROBOT"]=ROBOT;
	node_type_map["OBJECT"]=OBJECT;
	node_type_map["AUDIO"]=AUDIO;
	node_type_map["POINTCLOUD"]=POINTCLOUD;
	node_type_map["GENERIC"]=GENERIC;
	my_config = config;
	client_type = clienttype;
	if(viewer)
		viewing_only = true;
	return isconnected;
}


#ifdef WIN32
CLIENT_LIB_EXPORT void check()
#else
void check()
#endif
{
	for (packet = rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet = rakPeer->Receive())
	{
		switch (packet->data[0])
		{
		case ID_CONNECTION_ATTEMPT_FAILED:
			printf("ID_CONNECTION_ATTEMPT_FAILED\n");
			printf("Attempting reconnection ...");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			printf("ID_CONNECTION_REQUEST_ACCEPTED\n");
			isconnected=true;
			connection_lost=false;
			break;
		case ID_NEW_INCOMING_CONNECTION:
			printf("ID_NEW_INCOMING_CONNECTION from %s\n", packet->systemAddress.ToString());
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			removeAllNodes();
			printf("ID_DISCONNECTION_NOTIFICATION\n");
			isconnected=false;
			break;
		case ID_CONNECTION_LOST:
			printf("ID_CONNECTION_LOST\n");
			printf("Attempting reconnection ...");
			connection_lost=true;
			break;
		}
	}
	//timeBeginPeriod(1);

	if (connection_lost)
	{
		printf(".");
		RakSleep(200);
		rakPeer->Connect(ip,SERVER_PORT,0,0,0);
		RakSleep(200);
	}


	RakSleep(1);
}

#ifdef WIN32
CLIENT_LIB_EXPORT void createNode(char *id)
#else
void createNode(char *id)
#endif
{
	if (viewing_only) return;
	addNode(id,"AVATAR");
}


#ifdef WIN32
CLIENT_LIB_EXPORT int addRocketBoxAvatar(char *avatar_id, char *avatar_cfg)
#else
int addRocketBoxAvatar(char *avatar_id, char *avatar_cfg)
#endif
{
	if (viewing_only) return 0;
	char id1[128];
	char id2[128];
	sprintf(id1,"%s0",avatar_id); 
	if (AvatarNodeMap.find(id1) != AvatarNodeMap.end()) //if id exists in node database
	{
		printf("An avatar with the same id already exists\n");
		return 0;
	}
	sprintf(id1,"%s0",avatar_id); addNode(id1,"AVATAR"); updateAvatarNodes(id1,0,-1.18448e-015,-2.70976e-008,0,0.707106,-3.09086e-008,0.707107); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"0"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s1",avatar_id); sprintf(id2,"%s0",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0,-0.897348,3.92243e-008,0,-0.707106,3.09086e-008,0.707107); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"1"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s2",avatar_id); sprintf(id2,"%s0",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0,0,0,0.499999,0.5,-0.5,0.500001); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"2"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s3",avatar_id); sprintf(id2,"%s2",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.120265,1.66875e-007,0.00162871,7.39912e-007,-2.04582e-008,-7.15256e-007,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"3"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s4",avatar_id); sprintf(id2,"%s3",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.153172,-9.10502e-009,0.000121967,-1.55036e-006,-2.79393e-009,-2.98024e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"4"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s5",avatar_id); sprintf(id2,"%s4",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.153172,7.48332e-009,0.000121967,1.22113e-006,4.56349e-008,7.45057e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"5"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s6",avatar_id); sprintf(id2,"%s5",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.199124,2.79132e-008,-0.0156307,1.39536e-007,4.39412e-013,4.61936e-007,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"6"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s7",avatar_id); sprintf(id2,"%s6",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.069177,-3.00309e-006,-1.07102e-006,1.55691e-006,-0.0758123,-5.73888e-007,0.997122); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"7"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s8",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.207666,-1.2034e-008,5.26022e-016,0,0,0,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"8"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s9",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.110821,-0.0318851,-0.0699177,-6.49591e-006,-0.651464,4.80961e-006,0.758679); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"9"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s10",avatar_id); sprintf(id2,"%s9",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0250902,0,0,0.707388,9.59679e-007,-0.706825,9.37676e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"10"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s11",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.110821,0.0318855,-0.0699175,6.47779e-006,-0.651464,-8.29754e-006,0.758679); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"11"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s12",avatar_id); sprintf(id2,"%s11",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0250902,0,0,0.707388,9.36822e-007,-0.706825,9.80782e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"12"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s13",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0328945,1.326e-008,-0.0124002,-0.00120468,-0.764102,-0.00426142,0.64508); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"13"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s14",avatar_id); sprintf(id2,"%s13",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.149929,-9.31325e-010,-5.96046e-008,0.707388,9.85348e-007,-0.706825,9.66667e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"14"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s15",avatar_id); sprintf(id2,"%s13",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.10554,-0.000890068,0.023332,8.28424e-007,0.173648,1.9523e-006,0.984808); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"15"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s16",avatar_id); sprintf(id2,"%s15",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0174011,-7.21775e-009,5.96046e-008,0.707388,9.51017e-007,-0.706825,1.02989e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"16"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s17",avatar_id); sprintf(id2,"%s13",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0725112,0.000609742,0.0156659,-0.00303031,0.175704,0.0133431,0.984348); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"17"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s18",avatar_id); sprintf(id2,"%s17",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0225375,6.98492e-010,-3.05321e-017,0.707388,9.18663e-007,-0.706825,9.91247e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"18"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s19",avatar_id); sprintf(id2,"%s13",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.103353,0.0130727,0.0243573,0.0383811,0.169407,-0.216065,0.960804); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"19"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s20",avatar_id); sprintf(id2,"%s19",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0101238,3.72529e-009,-1.62838e-016,0.707388,9.87662e-007,-0.706825,9.60084e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"20"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s21",avatar_id); sprintf(id2,"%s13",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.10315,-0.014825,0.0242156,-0.0382873,0.169427,0.215535,0.960923); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"21"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s22",avatar_id); sprintf(id2,"%s21",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0101238,-7.45058e-009,5.96046e-008,0.707388,9.48031e-007,-0.706825,9.16134e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"22"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s23",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0458031,-0.0571939,-0.0772107,0.258198,-0.681405,0.282449,0.623894); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"23"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s24",avatar_id); sprintf(id2,"%s23",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0189569,0,0,0.707388,9.60477e-007,-0.706825,1.021e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"24"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s25",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0458031,0.0571943,-0.0772104,-0.2582,-0.681403,-0.282455,0.623893); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"25"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s26",avatar_id); sprintf(id2,"%s25",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0189569,0,0,0.707388,9.62452e-007,-0.706825,9.81499e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"26"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s27",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0508165,1.60252e-007,-0.118314,1.12487e-007,-0.675884,-2.66852e-006,0.737008); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"27"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s28",avatar_id); sprintf(id2,"%s27",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.014684,-2.60115e-010,1.137e-017,0.707388,1.01365e-006,-0.706825,1.03353e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"28"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s29",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0624822,-0.029447,-0.10813,0.190613,-0.652834,0.175083,0.711914); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"29"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s30",avatar_id); sprintf(id2,"%s29",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0131441,-2.6054e-015,-5.96046e-008,0.707388,9.49204e-007,-0.706825,1.01581e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"30"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s31",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0624822,0.0294476,-0.10813,-0.190613,-0.652833,-0.175086,0.711914); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"31"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s32",avatar_id); sprintf(id2,"%s31",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0131441,0,0,0.707388,9.94708e-007,-0.706825,1.00289e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"32"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s33",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.103746,-0.0341106,-0.0875056,0.0961292,-0.670102,0.0882967,0.730703); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"33"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s34",avatar_id); sprintf(id2,"%s33",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0106172,0,0,0.707388,1.02598e-006,-0.706825,9.51712e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"34"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s35",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.103746,0.034111,-0.0875054,-0.0961293,-0.670101,-0.0883003,0.730703); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"35"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s36",avatar_id); sprintf(id2,"%s35",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0106172,3.72529e-009,-5.96046e-008,0.707388,9.79238e-007,-0.706825,9.76644e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"36"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s37",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0502486,-0.0142556,-0.116842,0.170335,-0.686537,0.170568,0.685974); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"37"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s38",avatar_id); sprintf(id2,"%s37",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0143248,-7.45058e-009,-5.96046e-008,0.707388,9.53894e-007,-0.706825,1.01441e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"38"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s39",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0502486,0.0142563,-0.116842,-0.170181,-0.686574,-0.170416,0.686012); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"39"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s40",avatar_id); sprintf(id2,"%s39",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0143248,2.6054e-015,5.96046e-008,0.707388,9.55869e-007,-0.706825,1.00454e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"40"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s41",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0408259,-0.0277009,-0.107142,0.15813,-0.714628,0.201546,0.650908); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"41"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s42",avatar_id); sprintf(id2,"%s41",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0198967,3.72529e-009,-1.62838e-016,0.707388,9.0946e-007,-0.706825,1.03449e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"42"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s43",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.040826,0.0277015,-0.107142,-0.158133,-0.714626,-0.201553,0.650907); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"43"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s44",avatar_id); sprintf(id2,"%s43",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0198967,-5.58794e-009,-5.96046e-008,0.707388,9.16372e-007,-0.706825,9.04809e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"44"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s45",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0847544,-0.0413737,-0.0965376,0.132563,-0.699922,0.134937,0.688715); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"45"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s46",avatar_id); sprintf(id2,"%s45",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.016866,3.72529e-009,-1.62838e-016,0.707388,9.85492e-007,-0.706825,9.74587e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"46"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s47",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0847544,0.0413742,-0.0965374,-0.132569,-0.69992,-0.134947,0.688714); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"47"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s48",avatar_id); sprintf(id2,"%s47",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.016866,0,0,0.707388,9.69364e-007,-0.706825,9.86765e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"48"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s49",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.118678,-0.0333581,-0.0843964,0.101902,-0.561859,0.108616,0.813716); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"49"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s50",avatar_id); sprintf(id2,"%s49",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0125087,3.72529e-009,-1.62838e-016,0.707388,9.97917e-007,-0.706825,1.02355e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"50"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s51",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.118678,0.0333586,-0.0843962,-0.101902,-0.561858,-0.108619,0.813716); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"51"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s52",avatar_id); sprintf(id2,"%s51",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0125086,7.45058e-009,-3.25675e-016,0.707388,9.47805e-007,-0.706825,1.00857e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"52"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s53",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.130053,-0.0264132,-0.0946507,0.0641865,-0.673312,0.0589568,0.734204); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"53"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s54",avatar_id); sprintf(id2,"%s53",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0229148,-1.86265e-009,8.14188e-017,0.707388,1.01771e-006,-0.706825,9.55497e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"54"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s55",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.130053,0.0264137,-0.0946506,-0.0641866,-0.673312,-0.0589604,0.734204); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"55"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s56",avatar_id); sprintf(id2,"%s55",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0229148,-1.86265e-009,8.14188e-017,0.707388,8.84075e-007,-0.706825,9.96187e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"56"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s57",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.127065,1.26145e-007,-0.100129,1.46018e-009,-0.676903,-1.82665e-006,0.736073); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"57"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s58",avatar_id); sprintf(id2,"%s57",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0183988,6.95763e-010,-3.04128e-017,0.707388,9.31625e-007,-0.706825,1.01705e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"58"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s59",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.125416,-0.0492667,-0.0829298,0.180391,-0.656002,0.185586,0.708998); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"59"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s60",avatar_id); sprintf(id2,"%s59",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0209223,-7.45058e-009,3.25675e-016,0.707388,9.55951e-007,-0.706825,9.64137e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"60"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s61",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.125416,0.0492671,-0.0829295,-0.180391,-0.656,-0.18559,0.708999); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"61"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s62",avatar_id); sprintf(id2,"%s61",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0209223,3.72529e-009,5.96046e-008,0.707388,9.38425e-007,-0.706825,9.95652e-007); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"62"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s63",avatar_id); sprintf(id2,"%s7",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0870816,1.58052e-007,-0.121933,1.08481e-007,-0.673129,-2.65695e-006,0.739525); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"63"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s64",avatar_id); sprintf(id2,"%s63",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0296604,-3.87445e-010,1.69357e-017,0.707388,9.65289e-007,-0.706825,1.00774e-006); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"64"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s65",avatar_id); sprintf(id2,"%s6",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,-0.0608789,0.0728218,0.0139226,0.706984,0.70723,0.000321929,-0.000241136); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"65"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s66",avatar_id); sprintf(id2,"%s65",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.14108,-5.96046e-008,2.6054e-015,-0.000398014,-0.000816058,-0.000173655,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"66"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s67",avatar_id); sprintf(id2,"%s66",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.285801,1.27217e-018,2.91038e-011,3.59714e-013,0.00292861,-1.28013e-010,0.999996); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"67"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s68",avatar_id); sprintf(id2,"%s67",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.27092,-2.03547e-017,-4.65661e-010,0.707104,-0.00144577,-0.00146178,0.707106); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"68"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s69",avatar_id); sprintf(id2,"%s68",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0267608,-0.0319825,-0.00628209,-0.653939,0.0332818,0.366751,0.66087); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"69"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s70",avatar_id); sprintf(id2,"%s69",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0282782,-5.96046e-008,2.6054e-015,-0.00489643,-0.282866,-0.235788,0.929713); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"70"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s71",avatar_id); sprintf(id2,"%s70",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0310035,5.96046e-008,-7.45058e-009,-1.11757e-008,-1.1921e-007,-8.19564e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"71"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s72",avatar_id); sprintf(id2,"%s71",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0268396,-1.62838e-016,-3.72529e-009,0,0,0,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"72"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s73",avatar_id); sprintf(id2,"%s68",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0943896,-0.0224244,0.00203925,8.34451e-007,-1.111e-005,6.67363e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"73"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s74",avatar_id); sprintf(id2,"%s73",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0312896,7.45058e-009,5.96046e-008,0,-1.49011e-008,-4.52129e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"74"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s75",avatar_id); sprintf(id2,"%s74",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0237906,9.31323e-009,-4.07094e-016,-4.26326e-014,-2.17221e-014,-9.28401e-009,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"75"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s76",avatar_id); sprintf(id2,"%s75",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0209992,5.58794e-009,-2.44256e-016,0,0,0,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"76"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s77",avatar_id); sprintf(id2,"%s68",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0955927,0.00145351,0.00186419,1.07287e-006,-1.12441e-005,-8.65295e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"77"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s78",avatar_id); sprintf(id2,"%s77",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0380142,1.86265e-009,-8.14188e-017,2.98023e-008,-7.45057e-009,9.31311e-009,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"78"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s79",avatar_id); sprintf(id2,"%s78",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0284879,3.0268e-009,-1.32306e-016,-1.77636e-014,-1.49012e-008,6.33302e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"79"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s80",avatar_id); sprintf(id2,"%s79",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0201886,1.5134e-009,-6.61528e-017,0,0,0,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"80"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s81",avatar_id); sprintf(id2,"%s68",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0899363,0.0230169,-0.00226975,1.10267e-006,-1.15198e-005,-2.4363e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"81"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s82",avatar_id); sprintf(id2,"%s81",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0336964,1.11759e-008,-4.88513e-016,-2.98023e-008,5.21541e-008,9.31323e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"82"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s83",avatar_id); sprintf(id2,"%s82",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0244422,7.45058e-009,5.96046e-008,-5.96043e-008,2.83122e-007,-1.05705e-007,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"83"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s84",avatar_id); sprintf(id2,"%s83",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0205782,5.58794e-009,-2.44256e-016,0,0,0,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"84"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s85",avatar_id); sprintf(id2,"%s68",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0802381,0.0397473,-0.0120191,1.10267e-006,-1.13335e-005,9.90361e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"85"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s86",avatar_id); sprintf(id2,"%s85",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0290878,0,0,-8.88178e-016,-1.09616e-013,9.31323e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"86"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s87",avatar_id); sprintf(id2,"%s86",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.015358,2.6054e-015,5.96046e-008,8.88178e-015,7.45058e-009,-1.11759e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"87"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s88",avatar_id); sprintf(id2,"%s87",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0163014,-3.72529e-009,1.62838e-016,0,0,0,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"88"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s89",avatar_id); sprintf(id2,"%s6",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,-0.0608788,-0.0728219,0.0139221,-0.706984,0.707229,-0.000319832,-0.000243294); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"89"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s90",avatar_id); sprintf(id2,"%s89",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.14108,5.96046e-008,5.82051e-011,0.000398013,-0.000819397,0.00017363,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"90"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s91",avatar_id); sprintf(id2,"%s90",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.285801,5.08868e-018,1.16415e-010,-3.59714e-013,0.00292861,-1.28014e-010,0.999996); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"91"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s92",avatar_id); sprintf(id2,"%s91",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.27092,-1.01774e-017,-2.32831e-010,-0.707104,-0.00144577,0.00146178,0.707106); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"92"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s93",avatar_id); sprintf(id2,"%s92",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0267608,0.0319825,-0.00628209,0.653939,0.0332817,-0.366751,0.66087); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"93"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s94",avatar_id); sprintf(id2,"%s93",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0282782,0,0,0.00489644,-0.282866,0.235788,0.929713); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"94"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s95",avatar_id); sprintf(id2,"%s94",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0310035,-5.96046e-008,3.72529e-009,7.45057e-009,-1.04308e-007,7.26432e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"95"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s96",avatar_id); sprintf(id2,"%s95",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0268396,0,0,-3.47783e-023,1,-4.37114e-008,0); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"96"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s97",avatar_id); sprintf(id2,"%s92",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0943896,0.0224244,0.00203925,-8.34466e-007,-1.111e-005,-6.65031e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"97"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s98",avatar_id); sprintf(id2,"%s97",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0312896,-9.31322e-009,5.96046e-008,2.98023e-008,-1.49012e-008,4.49817e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"98"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s99",avatar_id); sprintf(id2,"%s98",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0237906,-5.58794e-009,2.44256e-016,-2.98023e-008,1.46269e-014,9.51695e-009,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"99"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s100",avatar_id); sprintf(id2,"%s99",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0209992,-5.58794e-009,2.44256e-016,2.84217e-014,1,-2.88102e-008,1.42109e-014); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"100"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s101",avatar_id); sprintf(id2,"%s92",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0955927,-0.00145351,0.00186419,-1.04308e-006,-1.12441e-005,7.95455e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"101"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s102",avatar_id); sprintf(id2,"%s101",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0380142,-2.56114e-009,1.11951e-016,-4.47035e-008,-1.49012e-008,-7.45057e-009,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"102"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s103",avatar_id); sprintf(id2,"%s102",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0284879,-1.74623e-009,7.63301e-017,-2.66454e-015,-1.49012e-008,-6.14673e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"103"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s104",avatar_id); sprintf(id2,"%s103",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0201886,-1.62981e-009,7.12415e-017,1.06581e-014,1,-4.37114e-008,-2.13163e-014); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"104"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s105",avatar_id); sprintf(id2,"%s92",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0899363,-0.0230169,-0.00226974,-1.08778e-006,-1.15198e-005,2.34336e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"105"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s106",avatar_id); sprintf(id2,"%s105",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0336965,-1.11759e-008,4.88513e-016,4.47035e-008,5.2154e-008,-9.31323e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"106"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s107",avatar_id); sprintf(id2,"%s106",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0244422,-7.45058e-009,5.96046e-008,2.98023e-008,2.83122e-007,1.05239e-007,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"107"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s108",avatar_id); sprintf(id2,"%s107",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0205782,-5.58794e-009,2.44256e-016,5.32907e-014,1,-2.88102e-008,-7.10543e-015); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"108"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s109",avatar_id); sprintf(id2,"%s92",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0802381,-0.0397473,-0.0120191,-1.08779e-006,-1.13409e-005,-9.53098e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"109"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s110",avatar_id); sprintf(id2,"%s109",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0290878,-3.72529e-009,1.62838e-016,2.66454e-015,-5.02562e-014,-9.31323e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"110"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s111",avatar_id); sprintf(id2,"%s110",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0153581,3.72529e-009,5.96046e-008,8.88178e-016,7.4506e-009,1.11759e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"111"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s112",avatar_id); sprintf(id2,"%s111",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0163014,0,0,0,1,-4.37114e-008,-2.13163e-014); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"112"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s113",avatar_id); sprintf(id2,"%s3",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,-0.120265,0.0883685,-0.00162871,-2.04243e-008,-7.31909e-007,1,2.93099e-014); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"113"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s114",avatar_id); sprintf(id2,"%s113",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.399035,1.49012e-008,-3.61785e-014,-9.76641e-012,0.00063169,8.65941e-010,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"114"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s115",avatar_id); sprintf(id2,"%s114",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.396689,7.45058e-009,5.82073e-011,-9.26113e-008,-0.000631698,-8.07444e-010,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"115"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s116",avatar_id); sprintf(id2,"%s115",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.101624,-5.95767e-009,-0.136296,-2.85965e-008,-0.707107,6.55324e-008,0.707107); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"116"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s117",avatar_id); sprintf(id2,"%s116",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0838147,-7.45058e-009,3.25675e-016,0,1,-4.37114e-008,0); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"117"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s118",avatar_id); sprintf(id2,"%s3",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,-0.120265,-0.0883685,-0.00162872,-2.04243e-008,7.26309e-007,1,-1.77636e-015); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"118"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s119",avatar_id); sprintf(id2,"%s118",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.399035,1.49012e-008,1.35595e-014,-1.0294e-011,0.00063169,-5.51985e-011,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"119"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s120",avatar_id); sprintf(id2,"%s119",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.396689,7.45058e-009,-3.25675e-016,9.26502e-008,-0.000631698,4.47002e-008,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"120"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s121",avatar_id); sprintf(id2,"%s120",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.101624,-1.34083e-008,-0.136296,-3.46237e-008,-0.707107,5.95052e-008,0.707107); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"121"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	sprintf(id1,"%s122",avatar_id); sprintf(id2,"%s121",avatar_id); addChildNode(id1,id2,"AVATAR"); updateAvatarNodes(id1,0.0838147,0,0,0,0,0,1); strcpy(AvatarNodeMap[id1]->clientConfig,avatar_cfg); strcpy(AvatarNodeMap[id1]->avatarjoint_id,"122"); strcpy(AvatarNodeMap[id1]->clientname,avatar_id);
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT void addNode(char *id, char node_type[])
#else
void addNode(char *id, char node_type[])
#endif
{
	int i = 0; 
	float calibration1[9] = {0.f};
	float calibration2[16] = {0.f};
	if (viewing_only) return;
	if (node_type_map.find(node_type) == node_type_map.end()) {
		printf("Incorrect node type (see BeamingDataType in documentation\n");
		return; 
	}
	//convert to uppercase letters
	/*while (node_type[i])
	{
		node_type[i] = (char)toupper(node_type[i]);
		i++;
	}*/
	//create a new replica instance and tell the replication system about it.
	//first update needed after node created
	switch (node_type_map[node_type])
	{
	case AVATAR:
		AvatarNodeMap[id] = new BeamingAvatarJointReplica;
		updateAvatarNodes(id,0.f,1.f,0.f,0.f,1.f,0.f,0.f);
		replicaManager.Reference(AvatarNodeMap[id]);
		//copy the global id, name and cfg_file here for backwards compatibility, gets overwritten in addRocketBoxAvatar
		strcpy(AvatarNodeMap[id]->clientConfig,my_config.c_str()); 
		strcpy(AvatarNodeMap[id]->clientname,client_name);
		strcpy(AvatarNodeMap[id]->avatarjoint_id,id);
		strcpy(AvatarNodeMap[id]->clientType,node_type);
		break;
	case FACIAL:
		FacialNodeMap[id] = new BeamingFacialReplica;
		updateFacialNodes(id,false,0.f,0.f,0.f,0.f,0.f);
		replicaManager.Reference(FacialNodeMap[id]);
		strcpy(FacialNodeMap[id]->clientType,node_type);
		break;
	case EMOTION:
		EmotionNodeMap[id] = new BeamingEmotionReplica;
		updateEmotionNodes(id,0,0,0);
		replicaManager.Reference(EmotionNodeMap[id]);
		strcpy(EmotionNodeMap[id]->clientType,node_type);
		break;
	case VIDEO:
		VideoNodeMap[id] = new BeamingVideoReplica;
		updateVideoNodes(id,"127.0.0.1",0,"http://localhost",1,1,1,calibration1,calibration2);
		replicaManager.Reference(VideoNodeMap[id]);
		strcpy(VideoNodeMap[id]->clientType,node_type);
		break;
	case TACTILE:
		TactileNodeMap[id] = new BeamingTactileReplica;
		updateTactileNodes(id,0,0,0);
		replicaManager.Reference(TactileNodeMap[id]);
		strcpy(TactileNodeMap[id]->clientType,node_type);
		break;
	case ROBOT:
		RobotNodeMap[id] = new BeamingRobotReplica;
		updateRobotNodes(id,0,0,0,0,0,0,0,1,0,0,0,0);
		replicaManager.Reference(RobotNodeMap[id]);
		strcpy(RobotNodeMap[id]->clientType,node_type);
		break;
	case OBJECT:
		ObjectNodeMap[id] = new BeamingObjectReplica;
		updateObjectNodes(id,"127.0.0.1",0,"http://localhost",0,0,0,0,1,0,0);
		replicaManager.Reference(ObjectNodeMap[id]);
		strcpy(ObjectNodeMap[id]->clientType,node_type);
		break;
	case AUDIO:
		AudioNodeMap[id] = new BeamingAudioReplica;
		updateAudioNodes(id,"127.0.0.1",0,"http://localhost","config file");
		replicaManager.Reference(AudioNodeMap[id]);
		strcpy(AudioNodeMap[id]->clientType,node_type);
		break;
	case POINTCLOUD:
		PointCloudNodeMap[id] = new BeamingPointCloudReplica;
		updatePointCloudNodes(id,"127.0.0.1",0,"http://localhost",0,0,0,0);
		replicaManager.Reference(PointCloudNodeMap[id]);
		strcpy(PointCloudNodeMap[id]->clientType,node_type);
		break;
	case GENERIC:
		GenericNodeMap[id] = new BeamingGenericReplica;
		void *data = malloc(1024);
		memset(data,0x00,1024);
		updateGenericNodes(id,data,1024);
		replicaManager.Reference(GenericNodeMap[id]);
		strcpy(GenericNodeMap[id]->clientType,node_type);
		break;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT void addChildNode(char *id, char *parent_id, char node_type[])
#else
void addChildNode(char *id, char *parent_id, char node_type[])
#endif
{
	if (viewing_only) return;
	addNode(id,node_type);
	if (AvatarNodeMap.find(parent_id) != AvatarNodeMap.end()) //if id exists in node database, then set parent
	{
		AvatarNodeMap[id]->parentbone = AvatarNodeMap[parent_id]->avatarjoint_id;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int deleteRocketBoxAvatar(char *avatar_id)
#else
int deleteRocketBoxAvatar(char *avatar_id)
#endif
{
	char id[128];
	int i;
	for ( int i=0; i<123; i++ )
	{
		sprintf(id,"%s%d",avatar_id,i); 
		std::map<std::string, BeamingAvatarJointReplica*>::iterator it = AvatarNodeMap.find(id);
		if (it != AvatarNodeMap.end()) //if id exists in node database, delete
		{
			replicaManager.BroadcastDestruction(AvatarNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
			replicaManager.Dereference(AvatarNodeMap[id]);
			delete AvatarNodeMap[id]; //destructor in replica3 broadcasts destruction
			AvatarNodeMap.erase(id);
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT void removeNode(char *id, char node_type[])
#else
void removeNode(char *id, char node_type[])
#endif
{
	int i = 0; 
	if (viewing_only) return;
	if (node_type_map.find(node_type) == node_type_map.end()) {
		printf("Incorrect node type (see BeamingDataType in documentation\n");
		return; 
	}
	//delete a new replica instance and tell the replication system about it.
	switch (node_type_map[node_type])
	{
/*	case AVATAR:
		std::map<std::string, BeamingAvatarJointReplica*>::iterator it = AvatarNodeMap.find(id);
		if (it != AvatarNodeMap.end()) //if id exists in node database, delete
		{
			replicaManager.BroadcastDestruction(AvatarNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
			replicaManager.Dereference(AvatarNodeMap[id]);
			delete AvatarNodeMap[id]; //destructor in replica3 broadcasts destruction
			AvatarNodeMap.erase(id);
		}
		break;*/
	case FACIAL:
		{
			std::map<std::string, BeamingFacialReplica*>::iterator it = FacialNodeMap.find(id);
			if (it != FacialNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(FacialNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(FacialNodeMap[id]);
				delete FacialNodeMap[id]; //destructor in replica3 broadcasts destruction
				FacialNodeMap.erase(id);
			}
		}
		break;
	case EMOTION:
		{
			std::map<std::string, BeamingEmotionReplica*>::iterator it = EmotionNodeMap.find(id);
			if (it != EmotionNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(EmotionNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(EmotionNodeMap[id]);
				delete EmotionNodeMap[id]; //destructor in replica3 broadcasts destruction
				EmotionNodeMap.erase(id);
			}
		}
		break;
	case VIDEO:
		{
			std::map<std::string, BeamingVideoReplica*>::iterator it = VideoNodeMap.find(id);
			if (it != VideoNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(VideoNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(VideoNodeMap[id]);
				delete VideoNodeMap[id]; //destructor in replica3 broadcasts destruction
				VideoNodeMap.erase(id);
			}
		}
		break;
	case TACTILE:
		{
			std::map<std::string, BeamingTactileReplica*>::iterator it = TactileNodeMap.find(id);
			if (it != TactileNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(TactileNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(TactileNodeMap[id]);
				delete TactileNodeMap[id]; //destructor in replica3 broadcasts destruction
				TactileNodeMap.erase(id);
			}
		}
		break;
	case ROBOT:
		{
			std::map<std::string, BeamingRobotReplica*>::iterator it = RobotNodeMap.find(id);
			if (it != RobotNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(RobotNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(RobotNodeMap[id]);
				delete RobotNodeMap[id]; //destructor in replica3 broadcasts destruction
				RobotNodeMap.erase(id);
			}
		}
		break;
	case OBJECT:
		{
			std::map<std::string, BeamingObjectReplica*>::iterator it = ObjectNodeMap.find(id);
			if (it != ObjectNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(ObjectNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(ObjectNodeMap[id]);
				delete ObjectNodeMap[id]; //destructor in replica3 broadcasts destruction
				ObjectNodeMap.erase(id);
			}
		}
		break;
	case AUDIO:
		{
			std::map<std::string, BeamingAudioReplica*>::iterator it = AudioNodeMap.find(id);
			if (it != AudioNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(AudioNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(AudioNodeMap[id]);
				delete AudioNodeMap[id]; //destructor in replica3 broadcasts destruction
				AudioNodeMap.erase(id);
			}
		}
		break;
	case POINTCLOUD:
		{
			std::map<std::string, BeamingPointCloudReplica*>::iterator it = PointCloudNodeMap.find(id);
			if (it != PointCloudNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(PointCloudNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(PointCloudNodeMap[id]);
				delete PointCloudNodeMap[id]; //destructor in replica3 broadcasts destruction
				PointCloudNodeMap.erase(id);
			}
		}
		break;
	case GENERIC:
		{
			std::map<std::string, BeamingGenericReplica*>::iterator it = GenericNodeMap.find(id);
			if (it != GenericNodeMap.end()) //if id exists in node database, delete
			{
				replicaManager.BroadcastDestruction(GenericNodeMap[id],UNASSIGNED_SYSTEM_ADDRESS);
				replicaManager.Dereference(GenericNodeMap[id]);
				delete GenericNodeMap[id]; //destructor in replica3 broadcasts destruction
				GenericNodeMap.erase(id);
			}
		}
		break;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int setAvatarName(char *avatar_id, char *firstname, char *lastname)
#else
int setAvatarName(char *avatar_id, char *firstname, char *lastname)
#endif
{
	if (viewing_only) return 0;
	char id[128];
	sprintf(id,"%s0",avatar_id); 
	std::map<std::string, BeamingAvatarJointReplica*>::iterator it = AvatarNodeMap.find(id);
	if (it != AvatarNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->fname,firstname);
		strcpy(it->second->lname,lastname);
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateNodes(char *id, float x, float y, float z, float rx, float ry, float rz, float w)
#else
int updateNodes(char *id, float x, float y, float z, float rx, float ry, float rz, float w)
#endif
{
	if (viewing_only) return 0;
	updateAvatarNodes(id,x,y,z,rx,ry,rz,w);
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateRocketBoxAvatar(char *avatar_id, char *node_id, float x, float y, float z, float rx, float ry, float rz, float w)
#else
int updateRocketBoxAvatar(char *avatar_id, char *node_id, float x, float y, float z, float rx, float ry, float rz, float w)
#endif
{
	if (viewing_only) return 0;
	char id1[128];
	sprintf(id1,"%s%s",avatar_id,node_id); 
	return updateAvatarNodes(id1,x,y,z,rx,ry,rz,w);
}

#ifdef WIN32
CLIENT_LIB_EXPORT int updateAvatarNodes(char *id, float x, float y, float z, float rx, float ry, float rz, float w)
#else
int updateAvatarNodes(char *id, float x, float y, float z, float rx, float ry, float rz, float w)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingAvatarJointReplica*>::iterator it = AvatarNodeMap.find(id);
	if (it != AvatarNodeMap.end()) //if id exists in node database, then update
	{
		//strcpy(it->second->avatarjoint_id,id);
		//rounding floats to 4 decimal places here to avoid network flooding due to very tiny differences
		if (x!=999.f && y!=999.f && z!=999.f)
		{
			it->second->position.x = floor((x*10000.f)+0.5f)/10000.f; 
			it->second->position.y = floor((y*10000.f)+0.5f)/10000.f; 
			it->second->position.z = floor((z*10000.f)+0.5f)/10000.f;
		}
		it->second->orientation.x = floor((rx*10000.f)+0.5f)/10000.f;
		it->second->orientation.y = floor((ry*10000.f)+0.5f)/10000.f;
		it->second->orientation.z = floor((rz*10000.f)+0.5f)/10000.f;
		it->second->orientation.w = floor((w*10000.f)+0.5f)/10000.f;
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateGenericNodes(char *id, void *pdata, int psize)
#else
int updateGenericNodes(char *id, void *pdata, int psize)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingGenericReplica*>::iterator it = GenericNodeMap.find(id);
	if (it != GenericNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->generic_id,id);
		memcpy ( it->second->anydata, pdata, 1024 );
		it->second->datasize = psize; 
		/*//begin printf
		struct mystruct{
			int x;
			char name[28];
			float myfloat;
		} st; 				
		printf("sending %s, %d, %.3f ...\n",((mystruct *)it->second->anydata)->name,((mystruct *)it->second->anydata)->x,((mystruct *)it->second->anydata)->myfloat);
		//end printf*/
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateFacialNodes(char *id, bool blink, float smile, float frown, float o, float e, float p)
#else
int updateFacialNodes(char *id, bool blink, float smile, float frown, float o, float e, float p)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingFacialReplica*>::iterator it = FacialNodeMap.find(id);
	if (it != FacialNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->facial_id,id);
		it->second->blink = blink;
		//rounding floats to 4 decimal places here to avoid network flooding due to very tiny differences
		it->second->smile = floor((smile*10000.f)+0.5f)/10000.f; 
		it->second->frown = floor((frown*10000.f)+0.5f)/10000.f;
		it->second->o = floor((o*10000.f)+0.5f)/10000.f;
		it->second->e = floor((e*10000.f)+0.5f)/10000.f;
		it->second->p= floor((p*10000.f)+0.5f)/10000.f;
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateEmotionNodes(char *id, double valence, double arousal, double misc)
#else
int updateEmotionNodes(char *id, double valence, double arousal, double misc)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingEmotionReplica*>::iterator it = EmotionNodeMap.find(id);
	if (it != EmotionNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->emotion_id,id);
		it->second->valence = valence;
		it->second->arousal = arousal;
		it->second->misc = misc;
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateTactileNodes(char *id, double duration, float intensity, float temperature)
#else
int updateTactileNodes(char *id, double duration, float intensity, float temperature)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingTactileReplica*>::iterator it = TactileNodeMap.find(id);
	if (it != TactileNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->tactile_id,id);
		it->second->duration = duration;
		it->second->intensity = intensity;
		it->second->temperature = temperature;
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateRobotNodes(char *id, int type, int details, float freespace, float x, float y, float z, float rx, float ry, float rz, float w, float time_remain, int contact_type)
#else
int updateRobotNodes(char *id, int type, int details, float freespace, float x, float y, float z, float rx, float ry, float rz, float w, float time_remain, int contact_type)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingRobotReplica*>::iterator it = RobotNodeMap.find(id);
	if (it != RobotNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->robot_id,id);
		it->second->type = type;
		it->second->details = details;
		it->second->freespace = freespace; 
		it->second->time_remain = time_remain; 
		it->second->contact_type = contact_type;
		//rounding floats to 4 decimal places here to avoid network flooding due to very tiny differences
		it->second->position.x = floor((x*10000.f)+0.5f)/10000.f; 
		it->second->position.y = floor((y*10000.f)+0.5f)/10000.f; 
		it->second->position.z = floor((z*10000.f)+0.5f)/10000.f;
		it->second->orientation.x = floor((rx*10000.f)+0.5f)/10000.f;
		it->second->orientation.y = floor((ry*10000.f)+0.5f)/10000.f;
		it->second->orientation.z = floor((rz*10000.f)+0.5f)/10000.f;
		it->second->orientation.w = floor((w*10000.f)+0.5f)/10000.f;
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateAudioNodes(char *id, char *host, int port, char *file_url, char *config)
#else
int updateAudioNodes(char *id, char *host, int port, char *file_url, char *config)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingAudioReplica*>::iterator it = AudioNodeMap.find(id);
	if (it != AudioNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->audio_id,id);
		strcpy(it->second->host,host);
		it->second->port = port;
		strcpy(it->second->file_url,file_url);
		strcpy(it->second->config,config);
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateVideoNodes(char *id, char *host, int port, char *file_url, int frame_width, int frame_height, double bandwidth, float *camera_calibration1, float *camera_calibration2)
#else
int updateVideoNodes(char *id, char *host, int port, char *file_url, int frame_width, int frame_height, double bandwidth, float *camera_calibration1, float *camera_calibration2)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingVideoReplica*>::iterator it = VideoNodeMap.find(id);
	if (it != VideoNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->video_id,id);
		strcpy(it->second->host,host);
		it->second->port = port;
		strcpy(it->second->file_url,file_url);
		it->second->frame_width = frame_width;
		it->second->frame_height = frame_height;
		it->second->bandwidth = bandwidth;
		for (int i=0; i<9; i++)
			it->second->calibration1.mf[i] = camera_calibration1[i];
		//printf("%.3f\n",camera_calibration1[5]); 
		for (int i=0; i<16; i++)
			it->second->calibration2.mf[i] = camera_calibration2[i];
		//printf("%.3f\n",camera_calibration2[5]); 
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updateObjectNodes(char *id, char *host, int port, char *file_url, float x, float y, float z, float rx, float ry, float rz, float w)
#else
int updateObjectNodes(char *id, char *host, int port, char *file_url, float x, float y, float z, float rx, float ry, float rz, float w)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingObjectReplica*>::iterator it = ObjectNodeMap.find(id);
	if (it != ObjectNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->object_id,id);
		strcpy(it->second->host,host);
		it->second->port = port;
		strcpy(it->second->file_url,file_url);
		//rounding floats to 4 decimal places here to avoid network flooding due to very tiny differences
		it->second->position.x = floor((x*10000.f)+0.5f)/10000.f; 
		it->second->position.y = floor((y*10000.f)+0.5f)/10000.f; 
		it->second->position.z = floor((z*10000.f)+0.5f)/10000.f;
		it->second->orientation.x = floor((rx*10000.f)+0.5f)/10000.f;
		it->second->orientation.y = floor((ry*10000.f)+0.5f)/10000.f;
		it->second->orientation.z = floor((rz*10000.f)+0.5f)/10000.f;
		it->second->orientation.w = floor((w*10000.f)+0.5f)/10000.f;
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int updatePointCloudNodes(char *id, char *host, int port, char *file_url, int size, double bandwidth, bool RGB_flag, int quality)
#else
int updatePointCloudNodes(char *id, char *host, int port, char *file_url, int size, double bandwidth, bool RGB_flag, int quality)
#endif
{
	if (viewing_only) return 0;
	std::map<std::string, BeamingPointCloudReplica*>::iterator it = PointCloudNodeMap.find(id);
	if (it != PointCloudNodeMap.end()) //if id exists in node database, then update
	{
		strcpy(it->second->clientname,client_name);
		strcpy(it->second->clientConfig,my_config.c_str());
		strcpy(it->second->pointcloud_id,id);
		strcpy(it->second->host,host);
		it->second->port = port;
		strcpy(it->second->file_url,file_url);
		it->second->size = size;
		it->second->bandwidth = bandwidth;
		it->second->RGB_flag = RGB_flag;
		it->second->quality_level = quality;
		return 1;
	}
	else {
		//printf("no node %s to update\n",id);
		return 0;
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getPeersInfo(char *peers_info)
#else
int getPeersInfo(char *peers_info)
#endif
{
	strcpy(peers_info,"");
	for ( std::map<std::string, std::vector<node_info*> >::iterator cIter = nodes_map.begin(); cIter!=nodes_map.end(); cIter++ )
	{
		char tmpstr[64];
		sprintf(tmpstr,"%s,%s,%s,%s;",cIter->first.c_str(),cIter->second[0]->peername,cIter->second[0]->peertype,cIter->second[0]->peercfg);//guid,client-name,client-type,client-config
		strcat(peers_info,tmpstr);
	}
	return nodes_map.size();
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getMyGUID(char *mGUID)
#else
int getMyGUID(char *mGUID)
#endif
{
	strcpy(mGUID,""); 
	strcpy(mGUID,myid.c_str());
	return 1;
}

#ifdef WIN32
CLIENT_LIB_EXPORT void getIPinfo(char strid[], char *ipinfo)
#else
void getIPinfo(char strid[], char *ipinfo)
#endif
{
	//RakNetGUID strguid;
	//strguid.FromString(strid);
	std::map<std::string, std::vector<node_info*> >::iterator it = nodes_map.find(strid);
	if (it != nodes_map.end()) //if id exists in node database, get ip
	{
		sprintf(ipinfo,"%s",it->second[0]->ipport);
	}
	//SystemAddress address = rakPeer->GetSystemAddressFromGuid(strguid);
	//sprintf(ipinfo,"%s",address.ToString());
	//printf("%s\n",address.ToString());
	/*DataStructures::List<SystemAddress> connected_ips;
	DataStructures::List<RakNetGUID> connected_guids;
	unsigned short numConnections;
	SystemAddress *connectedSystems = new SystemAddress;
	printf("%d\n",replicaManager.GetConnectionCount());
	rakPeer->GetConnectionList(connectedSystems,&numConnections);
	printf("%d\n",numConnections);
	for (int index=0; index < connectedSystems->size(); index++)
	{
		printf("%s\n",connectedSystems[index].ToString());
	}
	rakPeer->GetSystemList(connected_ips,connected_guids);
	for (int index=0; index < connected_guids.Size(); index++)
	{
		printf("%s\n",connected_guids[index].ToString());
	}*/
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getPeersID(char *peers_info)
#else
int getPeersID(char *peers_info)
#endif
{
	strcpy(peers_info,"");
	for ( std::map<std::string, std::vector<node_info*> >::iterator cIter = nodes_map.begin(); cIter!=nodes_map.end(); cIter++ )
	{
		char tmpstr[64];
		sprintf(tmpstr,"%s;",cIter->first.c_str());//guid
		strcat(peers_info,tmpstr);
	}
	return nodes_map.size();
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getNodesInfo(char peer_guid[], char *nodes_info)
#else
int getNodesInfo(char peer_guid[], char *nodes_info)
#endif
{
	int x=0;
	char tmpstr[256];
	strcpy(nodes_info,"");
	std::map<std::string, std::vector<node_info*> >::iterator it = nodes_map.find(peer_guid);
	if (it != nodes_map.end()) //if id exists in node database, add to nodes_info string
	{
		for ( std::vector<node_info*>::iterator cIter = it->second.begin(); cIter!=it->second.end(); cIter++ )
		{
			// presents one full rocketbox avatar as one node
			if ( ( (strcmp((*cIter)->name.c_str(),"0")==0) && (strcmp((*cIter)->peertype,"AVATAR")==0) ) //check for multiple avatars through unique root node 
				|| (strcmp((*cIter)->type.c_str(),"AVATAR")!=0) ) 
			{
				sprintf(tmpstr,"%s,%s,%s,%s;",(*cIter)->peername,(*cIter)->name.c_str(),(*cIter)->type.c_str(),(*cIter)->peercfg);  
				//printf("%s,%s,%s,%s\n",(*cIter)->peername,(*cIter)->name.c_str(),(*cIter)->type.c_str(),(*cIter)->peercfg);
				strcat(nodes_info,tmpstr);
				x++;
			}
		}
		//printf("%s\t",nodes_info);
	}
	return x;
}

#ifdef WIN32
CLIENT_LIB_EXPORT void getAvatarName(char strid[], char avatar_id[], char *firstname, char *lastname)
#else
void getAvatarName(char strid[], char avatar_id[], char *firstname, char *lastname)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		avatar_joint_replicas.erase(strguid);
		return;
	}
	strcpy(firstname,"");
	strcpy(lastname,"");
	for ( std::vector<BeamingAvatarJointReplica*>::iterator cIter = avatar_joint_replicas[strguid].begin(); cIter!=avatar_joint_replicas[strguid].end(); cIter++ )
	{
		if (( strcmp((*cIter)->avatarjoint_id,"0") == 0 ) && ( strcmp((*cIter)->clientname,avatar_id) == 0 ))
		{
			char tmpstr[128];
			sprintf(firstname,"%s",(*cIter)->fname);  
			sprintf(lastname,"%s",(*cIter)->lname); 
			break;
		}
	}
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getAvatarData(char strid[], char avatar_id[], char *node_ids, float *fa)
#else
int getAvatarData(char strid[], char avatar_id[], char *node_ids,  float *fa)
#endif
{
	//printf("fetching %s ",strid);
	int x = 0, y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		avatar_joint_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
	for ( std::vector<BeamingAvatarJointReplica*>::iterator cIter = avatar_joint_replicas[strguid].begin(); cIter!=avatar_joint_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->clientname,avatar_id) == 0 )
		{
			char tmpstr[128];
			sprintf(tmpstr,"%s,",(*cIter)->avatarjoint_id); //return node-ids as comma-delimited strings 
			strcat(node_ids,tmpstr);
			fa[x] = (*cIter)->position.x;
			fa[++x] = (*cIter)->position.y;
			fa[++x] = (*cIter)->position.z;
			fa[++x] = (*cIter)->orientation.x;
			fa[++x] = (*cIter)->orientation.y;
			fa[++x] = (*cIter)->orientation.z;
			fa[++x] = (*cIter)->orientation.w;
			x++;
			y++;
		}
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getFacialData(char strid[], char *node_ids, bool *blink, float *smile, float *frown, float *o, float *e, float *p)
#else
int getFacialData(char strid[], char *node_ids, bool *blink, float *smile, float *frown, float *o, float *e, float *p)
#endif
{
	//printf("fetching %s ",strid);
	int y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		facial_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
	for ( std::vector<BeamingFacialReplica*>::iterator cIter = facial_replicas[strguid].begin(); cIter!=facial_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->facial_id); //return node-ids as comma-delimited strings 
		strcat(node_ids,tmpstr);
		blink[y] = (*cIter)->blink;
		smile[y] = (*cIter)->smile;
		frown[y] = (*cIter)->frown;
		o[y] = (*cIter)->o;
		e[y] = (*cIter)->e;
		p[y] = (*cIter)->p;
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getEmotionData(char strid[], char *node_ids, double *valence, double *arousal, double *misc)
#else
int getEmotionData(char strid[], char *node_ids, double *valence, double *arousal, double *misc)
#endif
{
	//printf("fetching %s ",strid);
	int y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		emotion_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
	for ( std::vector<BeamingEmotionReplica*>::iterator cIter = emotion_replicas[strguid].begin(); cIter!=emotion_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->emotion_id); //return node-ids as comma-delimited strings 
		strcat(node_ids,tmpstr);
		valence[y] = (*cIter)->valence;
		arousal[y] = (*cIter)->arousal;
		misc[y] = (*cIter)->misc;
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getTactileData(char strid[], char *node_ids, double *duration, float *intensity, float *temperature)
#else
int getTactileData(char strid[], char *node_ids, double *duration, float *intensity, float *temperature)
#endif
{
	//printf("fetching %s ",strid);
	int x = 0, y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		tactile_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
	for ( std::vector<BeamingTactileReplica*>::iterator cIter = tactile_replicas[strguid].begin(); cIter!=tactile_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->tactile_id); //return node-ids as comma-delimited strings 
		strcat(node_ids,tmpstr);
		duration[y] = (*cIter)->duration;
		intensity[y] = (*cIter)->intensity;
		temperature[y] = (*cIter)->temperature;
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getRobotData(char strid[], char *node_ids, int *type, int *details, float *freespace, float *fa, float *time_remain, int *contact_type)
#else
int getRobotData(char strid[], char *node_ids, int *type, int *details, float *freespace, float *fa, float *time_remain, int *contact_type)
#endif
{
	//printf("fetching %s ",strid);
	int x = 0, y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		robot_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
	for ( std::vector<BeamingRobotReplica*>::iterator cIter = robot_replicas[strguid].begin(); cIter!=robot_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->robot_id); //return node-ids as comma-delimited strings 
		strcat(node_ids,tmpstr);
		type[y] = (*cIter)->type;
		details[y] = (*cIter)->details;
		freespace[y] = (*cIter)->freespace;
		fa[x] = (*cIter)->position.x;
		fa[++x] = (*cIter)->position.y;
		fa[++x] = (*cIter)->position.z;
		fa[++x] = (*cIter)->orientation.x;
		fa[++x] = (*cIter)->orientation.y;
		fa[++x] = (*cIter)->orientation.z;
		fa[++x] = (*cIter)->orientation.w;
		time_remain[y] = (*cIter)->time_remain;
		contact_type[y] = (*cIter)->contact_type;
		x++;
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getAudioData(char strid[], char *node_ids, char *host, int *port, char *file_url, char *config)
#else
int getAudioData(char strid[], char *node_ids, char *host, int *port, char *file_url, char *config)
#endif
{
	//printf("fetching %s ",strid);
	int y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		audio_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
    strcpy(host,"");
    strcpy(file_url,"");
    strcpy(config,"");
	for ( std::vector<BeamingAudioReplica*>::iterator cIter = audio_replicas[strguid].begin(); cIter!=audio_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->audio_id); //return node-ids as comma-delimited strings 
		strcat(node_ids,tmpstr);
		sprintf(tmpstr,"%s,",(*cIter)->host); //return hosts as comma-delimited strings 
		strcat(host,tmpstr);
		port[y] = (*cIter)->port;
		sprintf(tmpstr,"%s,",(*cIter)->file_url); //return file_url as comma-delimited strings 
		strcat(file_url,tmpstr);
		sprintf(tmpstr,"%s,",(*cIter)->config); //return config as comma-delimited strings 
		strcat(config,tmpstr);
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getVideoData(char strid[], char *node_ids, char *host, int *port, char *file_url, int *frame_width, int *frame_height, double *bandwidth, float *camera_calibration1, float *camera_calibration2)
#else
int getVideoData(char strid[], char *node_ids, char *host, int *port, char *file_url, int *frame_width, int *frame_height, double *bandwidth, float *camera_calibration1, float *camera_calibration2)
#endif
{
	//printf("fetching %s ",strid);
	int y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		video_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
    strcpy(host,"");
    strcpy(file_url,"");
	for ( std::vector<BeamingVideoReplica*>::iterator cIter = video_replicas[strguid].begin(); cIter!=video_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->video_id); //return node-ids as strings 
		strcat(node_ids,tmpstr);
		sprintf(tmpstr,"%s,",(*cIter)->host); //return hosts as comma-delimited strings 
		strcat(host,tmpstr);
		port[y] = (*cIter)->port;
		sprintf(tmpstr,"%s,",(*cIter)->file_url); //return file_url as comma-delimited strings 
		strcat(file_url,tmpstr);
		frame_width[y] = (*cIter)->frame_width;
		frame_height[y] = (*cIter)->frame_height;
		bandwidth[y] = (*cIter)->bandwidth;
		for (int i=0; i<9; i++)
			camera_calibration1[i+(y*8)] = (*cIter)->calibration1.mf[i];
		for (int i=0; i<16; i++)
			camera_calibration2[i+(y*15)] = (*cIter)->calibration2.mf[i];
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getObjectData(char strid[], char *node_ids, char *host, int *port, char *file_url, float *fa)
#else
int getObjectData(char strid[], char *node_ids, char *host, int *port, char *file_url, float *fa)
#endif
{
	//printf("fetching %s ",strid);
	int x = 0, y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		object_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
    strcpy(host,"");
    strcpy(file_url,"");
	for ( std::vector<BeamingObjectReplica*>::iterator cIter = object_replicas[strguid].begin(); cIter!=object_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->object_id); //return node-ids as strings 
		strcat(node_ids,tmpstr);
		sprintf(tmpstr,"%s,",(*cIter)->host); //return hosts as comma-delimited strings 
		strcat(host,tmpstr);
		port[y] = (*cIter)->port;
		sprintf(tmpstr,"%s,",(*cIter)->file_url); //return file_url as comma-delimited strings 
		strcat(file_url,tmpstr);
		fa[x] = (*cIter)->position.x;
		fa[++x] = (*cIter)->position.y;
		fa[++x] = (*cIter)->position.z;
		fa[++x] = (*cIter)->orientation.x;
		fa[++x] = (*cIter)->orientation.y;
		fa[++x] = (*cIter)->orientation.z;
		fa[++x] = (*cIter)->orientation.w;
		x++;
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int getPointCloudData(char strid[], char *node_ids, char *host, int *port, char *file_url, int *size, double *bandwidth, bool *RGB_flag, int *quality)
#else
int getPointCloudData(char strid[], char *node_ids, char *host, int *port, char *file_url, int *size, double *bandwidth, bool *RGB_flag, int *quality)
#endif
{
	//printf("fetching %s ",strid);
	int y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		point_cloud_replicas.erase(strguid);
		return 0;
	}
	strcpy(node_ids,"");
    strcpy(host,"");
    strcpy(file_url,"");
	for ( std::vector<BeamingPointCloudReplica*>::iterator cIter = point_cloud_replicas[strguid].begin(); cIter!=point_cloud_replicas[strguid].end(); cIter++ )
	{
		char tmpstr[128];
		sprintf(tmpstr,"%s,",(*cIter)->pointcloud_id); //return node-ids as strings 
		strcat(node_ids,tmpstr);
		sprintf(tmpstr,"%s,",(*cIter)->host); //return hosts as comma-delimited strings 
		strcat(host,tmpstr);
		port[y] = (*cIter)->port;
		sprintf(tmpstr,"%s,",(*cIter)->file_url); //return file_url as comma-delimited strings 
		strcat(file_url,tmpstr);
		size[y] = (*cIter)->size;
		RGB_flag[y] = (*cIter)->RGB_flag;
		bandwidth[y] = (*cIter)->bandwidth;
		quality[y] = (*cIter)->quality_level;
		y++;
	}
	//printf("%s\n",node_ids);
	return y;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getAvatarSpecificData(char strid[], char avatar_id[], char node_id[], float *fa)
#else
bool getAvatarSpecificData(char strid[], char avatar_id[], char node_id[], float *fa)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		avatar_joint_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingAvatarJointReplica*>::iterator cIter = avatar_joint_replicas[strguid].begin(); cIter!=avatar_joint_replicas[strguid].end(); cIter++ )
	{
		if (( strcmp((*cIter)->avatarjoint_id,node_id) == 0 ) && ( strcmp((*cIter)->clientname,avatar_id) == 0 ))
		{
			fa[0] = (*cIter)->position.x;
			fa[1] = (*cIter)->position.y;
			fa[2] = (*cIter)->position.z;
			fa[3] = (*cIter)->orientation.x;
			fa[4] = (*cIter)->orientation.y;
			fa[5] = (*cIter)->orientation.z;
			fa[6] = (*cIter)->orientation.w;
			break;
		}
	}
	return 1;
}


//NOTE: I haven't tested this function visually to ensure it works as expected.
#ifdef WIN32
CLIENT_LIB_EXPORT bool getAvatarSpecificGlobalData(char strid[], char avatar_id[], char node_id[], float *fa)
#else
bool getAvatarSpecificGlobalData(char strid[], char avatar_id[], char node_id[], float *fa)
#endif
{
	std::map<std::string, std::vector<float> > global_vars;
	std::map<std::string, std::string > parent;
	std::list<std::string> parents;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		avatar_joint_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingAvatarJointReplica*>::iterator cIter = avatar_joint_replicas[strguid].begin(); cIter!=avatar_joint_replicas[strguid].end(); cIter++ )
	{
		global_vars[(*cIter)->avatarjoint_id].push_back((*cIter)->position.x);
		global_vars[(*cIter)->avatarjoint_id].push_back((*cIter)->position.y);
		global_vars[(*cIter)->avatarjoint_id].push_back((*cIter)->position.z);
		global_vars[(*cIter)->avatarjoint_id].push_back((*cIter)->orientation.x);
		global_vars[(*cIter)->avatarjoint_id].push_back((*cIter)->orientation.y);
		global_vars[(*cIter)->avatarjoint_id].push_back((*cIter)->orientation.z);
		global_vars[(*cIter)->avatarjoint_id].push_back((*cIter)->orientation.w);
		parent[(*cIter)->avatarjoint_id] = (*cIter)->parentbone;
		//printf("%s,%s\n",(*cIter)->avatarjoint_id,(*cIter)->parentbone.c_str());
		if (( strcmp((*cIter)->avatarjoint_id,node_id) == 0 ) && ( strcmp((*cIter)->clientname,avatar_id) == 0 ))
		{
			//printf("%s",(*cIter)->avatarjoint_id);
			parents.push_front((*cIter)->avatarjoint_id);
			std::string avatarbone = (*cIter)->parentbone.C_String();
			while(!avatarbone.empty())
			{
				//printf("\t%s",avatarbone.c_str());
				parents.push_front(avatarbone);
				avatarbone = parent[avatarbone];
			}
			//printf("\n");
			break;
		}
	}
	CVec3 pos = CVec3();
	CQuat rot = CQuat();
	for (std::list<std::string>::iterator it=parents.begin(); it!=parents.end(); ++it)
	{
		pos = pos + CVec3(global_vars[*it][0],global_vars[*it][1],global_vars[*it][2]);
		rot = rot * CQuat(global_vars[*it][3],global_vars[*it][4],global_vars[*it][5],global_vars[*it][6]);
	}
	fa[0] = pos.x;
	fa[1] = pos.y;
	fa[2] = pos.z;
	fa[3] = rot.x;
	fa[4] = rot.y;
	fa[5] = rot.z;
	fa[6] = rot.w;
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getFacialSpecificData(char strid[], char node_id[], bool *blink, float *smile, float *frown, float *o, float *e, float *p)
#else
bool getFacialSpecificData(char strid[], char node_id[], bool *blink, float *smile, float *frown, float *o, float *e, float *p)
#endif
{
	int y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		facial_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingFacialReplica*>::iterator cIter = facial_replicas[strguid].begin(); cIter!=facial_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->facial_id,node_id) == 0 )
		{
			*blink = (*cIter)->blink;
			*smile = (*cIter)->smile;
			*frown = (*cIter)->frown;
			*o = (*cIter)->o;
			*e = (*cIter)->e;
			*p = (*cIter)->p;
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getEmotionSpecificData(char strid[], char node_id[], double *valence, double *arousal, double *misc)
#else
bool getEmotionSpecificData(char strid[], char node_id[], double *valence, double *arousal, double *misc)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		emotion_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingEmotionReplica*>::iterator cIter = emotion_replicas[strguid].begin(); cIter!=emotion_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->emotion_id,node_id) == 0 )
		{
			*valence = (*cIter)->valence;
			*arousal = (*cIter)->arousal;
			*misc = (*cIter)->misc;
			break;
		}
	}
	return 1;
}

#ifdef WIN32
CLIENT_LIB_EXPORT bool getTactileSpecificData(char strid[], char node_id[], double *duration, float *intensity, float *temperature)
#else
bool getTactileSpecificData(char strid[], char node_id[], double *duration, float *intensity, float *temperature)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		tactile_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingTactileReplica*>::iterator cIter = tactile_replicas[strguid].begin(); cIter!=tactile_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->tactile_id,node_id) == 0 )
		{
			*duration = (*cIter)->duration;
			*intensity = (*cIter)->intensity;
			*temperature = (*cIter)->temperature;
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getRobotSpecificData(char strid[], char node_id[], int *type, int *details, float *freespace, float *fa, float *time_remain, int *contact_type)
#else
bool getRobotSpecificData(char strid[], char node_id[], int *type, int *details, float *freespace, float *fa, float *time_remain, int *contact_type)
#endif
{
	//printf("fetching %s ",strid);
	int x = 0, y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		robot_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingRobotReplica*>::iterator cIter = robot_replicas[strguid].begin(); cIter!=robot_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->robot_id,node_id) == 0 )
		{
			*type = (*cIter)->type;
			*details = (*cIter)->details;
			*freespace = (*cIter)->freespace;
			fa[0] = (*cIter)->position.x;
			fa[1] = (*cIter)->position.y;
			fa[2] = (*cIter)->position.z;
			fa[3] = (*cIter)->orientation.x;
			fa[4] = (*cIter)->orientation.y;
			fa[5] = (*cIter)->orientation.z;
			fa[6] = (*cIter)->orientation.w;
			*time_remain = (*cIter)->time_remain;
			*contact_type = (*cIter)->contact_type;
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getAudioSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, char *config)
#else
bool getAudioSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, char *config)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		audio_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingAudioReplica*>::iterator cIter = audio_replicas[strguid].begin(); cIter!=audio_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->audio_id,node_id) == 0 )
		{
			strcpy(host,(*cIter)->host);
			*port = (*cIter)->port;
			strcpy(file_url,(*cIter)->file_url);
			strcpy(config,(*cIter)->config);
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getVideoSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, int *frame_width, int *frame_height, double *bandwidth, float *camera_calibration1, float *camera_calibration2)
#else
bool getVideoSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, int *frame_width, int *frame_height, double *bandwidth, float *camera_calibration1, float *camera_calibration2)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		video_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingVideoReplica*>::iterator cIter = video_replicas[strguid].begin(); cIter!=video_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->video_id,node_id) == 0 )
		{
			strcpy(host,(*cIter)->host);
			*port = (*cIter)->port;
			strcpy(file_url,(*cIter)->file_url);
			*frame_width = (*cIter)->frame_width;
			*frame_height = (*cIter)->frame_height;
			*bandwidth = (*cIter)->bandwidth;
			for (int i=0; i<9; i++)
				camera_calibration1[i] = (*cIter)->calibration1.mf[i];
			for (int i=0; i<16; i++)
				camera_calibration2[i] = (*cIter)->calibration2.mf[i];
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getObjectSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, float *fa)
#else
bool getObjectSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, float *fa)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		object_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingObjectReplica*>::iterator cIter = object_replicas[strguid].begin(); cIter!=object_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->object_id,node_id) == 0 )
		{
			strcpy(host,(*cIter)->host);
			*port = (*cIter)->port;
			strcpy(file_url,(*cIter)->file_url);
			fa[0] = (*cIter)->position.x;
			fa[1] = (*cIter)->position.y;
			fa[2] = (*cIter)->position.z;
			fa[3] = (*cIter)->orientation.x;
			fa[4] = (*cIter)->orientation.y;
			fa[5] = (*cIter)->orientation.z;
			fa[6] = (*cIter)->orientation.w;
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getPointCloudSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, int *size, double *bandwidth, bool *RGB_flag, int *quality)
#else
bool getPointCloudSpecificData(char strid[], char node_id[], char *host, int *port, char *file_url, int *size, double *bandwidth, bool *RGB_flag, int *quality)
#endif
{
	int y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		point_cloud_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingPointCloudReplica*>::iterator cIter = point_cloud_replicas[strguid].begin(); cIter!=point_cloud_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->pointcloud_id,node_id) == 0 )
		{
			strcpy(host,(*cIter)->host);
			*port = (*cIter)->port;
			strcpy(file_url,(*cIter)->file_url);
			*size = (*cIter)->size;
			*RGB_flag = (*cIter)->RGB_flag;
			*bandwidth = (*cIter)->bandwidth;
			*quality = (*cIter)->quality_level;
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT bool getGenericData(char strid[], char generic_id[], void *pdata, int *psize)
#else
bool getGenericData(char strid[], char generic_id[], void *pdata, int *psize)
#endif
{
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0) {
		generic_replicas.erase(strguid);
		return 0;
	}
	for ( std::vector<BeamingGenericReplica*>::iterator cIter = generic_replicas[strguid].begin(); cIter!=generic_replicas[strguid].end(); cIter++ )
	{
		if ( strcmp((*cIter)->generic_id,generic_id) == 0 )
		{
			*psize = (*cIter)->datasize;
			memcpy ( pdata, (*cIter)->anydata, 1024 );
			/*//begin printf
			struct mystruct{
				int x;
				char name[28];
				float myfloat;
			} st; 				
			printf("receiving %s, %d, %.3f ...\n",((mystruct *)(*cIter)->anydata)->name,((mystruct *)(*cIter)->anydata)->x,((mystruct *)(*cIter)->anydata)->myfloat);
			//end printf*/
			break;
		}
	}
	return 1;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int checkStatus(char *idstr, char *mytype, char *name, char *myconfig)
#else
int checkStatus(char *idstr, char *mytype, char *name, char *myconfig)
#endif
{
	int x = new_clients.size();
	if ( x > 0 )
	{
		std::map<RakNetGUID, std::vector<std::string> >::iterator cIter = new_clients.begin(); //get first item in the list 
		strcpy(idstr,cIter->first.ToString()); //copy guid
		strcpy(name,cIter->second[0].c_str()); //copy name
		strcpy(mytype,cIter->second[1].c_str()); //copy type
		strcpy(myconfig,cIter->second[2].c_str()); //copy avatar name
		printf("checkStatus -> %s %s %s \n",name,mytype,myconfig);
		new_clients.erase(cIter->first); //erase this item once accessed
	}
	//printf("size %i \n",x);
	return x;
}


#ifdef WIN32
CLIENT_LIB_EXPORT int fetch(char strid[], float *fa)
#else
int fetch(char strid[],  float *fa)
#endif
{
	//printf("fetching %s ",strid);
	int x = 0, y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0)
	{
		avatar_joint_replicas.erase(strguid);
		//printf("client %s not available\n", strid);
		return 0;
	}
	//std::cout<<" guid="<<cIter->first.ToString()<<" replicas="<<replicaListOut.GetSize()<<", "<<cIter->second.size()<<std::endl; 
	for ( std::vector<BeamingAvatarJointReplica*>::iterator cIter = avatar_joint_replicas[strguid].begin(); cIter!=avatar_joint_replicas[strguid].end(); cIter++ )
	{
		if (y>122) {
			printf("More than one AVATAR added by %s, use getNodesInfo()",strid);
			break;
		}
		float z=0; //for printf
		fa[x] = z = y;//atof((*cIter))->avatarjoint_id; //return node_ids as a number even if string
		//printf("object fa[%i]=%.f ",x,z);
		fa[++x] = z = (*cIter)->position.x;
		//printf("position/orientation fa[%i]=%.3f ",x,z);
		fa[++x] = z = (*cIter)->position.y;
		//printf("fa[%i]=%.3f ",x,z);
		fa[++x] = z = (*cIter)->position.z;
		//printf("fa[%i]=%.3f ",x,z);
		fa[++x] = z = (*cIter)->orientation.x;
		//printf("fa[%i]=%.3f ",x,z);
		fa[++x] = z = (*cIter)->orientation.y;
		//printf("fa[%i]=%.3f ",x,z);
		fa[++x] = z = (*cIter)->orientation.z;
		//printf("fa[%i]=%.3f ",x,z);
		fa[++x] = z = (*cIter)->orientation.w;
		//printf("fa[%i]=%.3f ",x,z);
		x++;
		y++;
	}
	//printf("count=%i \n",y);	
	return y;
}

#ifdef WIN32
CLIENT_LIB_EXPORT int fetch2(char strid[], char *fa)
#else
int fetch2(char strid[],  char *fa)
#endif
{
	//printf("fetching %s ",strid);
	int x = 0, y = 0;
	RakNetGUID strguid;
	strguid.FromString(strid);
	//check to see if deleted
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	replicaManager.GetReplicasCreatedByGuid(strguid,replicaListOut);
	if (replicaListOut.GetSize()==0)
	{
		avatar_joint_replicas.erase(strguid);
		//printf("client %s not available\n", strid);
		return 0;
	}
	//std::cout<<" guid="<<cIter->first.ToString()<<" replicas="<<replicaListOut.GetSize()<<", "<<cIter->second.size()<<std::endl; 
	std::stringstream tmp;
	for ( std::vector<BeamingAvatarJointReplica*>::iterator cIter = avatar_joint_replicas[strguid].begin(); cIter!=avatar_joint_replicas[strguid].end(); cIter++ )
	{
		if (y>122) {
			printf("More than one AVATAR added by %s, use getNodesInfo()",strid);
			break;
		}
		tmp << y << ","<<(*cIter)->position.x<<","<<(*cIter)->position.y<<","<<(*cIter)->position.z<<","<<(*cIter)->orientation.x<<","<<(*cIter)->orientation.y<<","<<(*cIter)->orientation.z<<","<<(*cIter)->orientation.w<<";";
		y++;
	}
	strcpy(fa,tmp.str().c_str());
	//printf("count=%i \n",y);	
	return y;
}

#ifdef WIN32
CLIENT_LIB_EXPORT void removeAllNodes()
#else
void removeAllNodes()
#endif
{
	printf("All nodes for %s destroyed.\n",myid.c_str());
	DataStructures::Multilist<ML_STACK, Replica3*> replicaListOut;
	// ClearPointers is needed, as I don't track which objects have and have not been allocated at the application level. 
	// So ClearPointers will call delete on every object in the returned list, 
	// which is every object that the application has created. Another way to put it is
	// 	A. Send a packet to tell other systems to delete these objects
	// 	B. Delete these objects on my own system
	replicaManager.GetReplicasCreatedByMe(replicaListOut);
	replicaManager.BroadcastDestructionList(replicaListOut, UNASSIGNED_SYSTEM_ADDRESS);
	replicaListOut.ClearPointers( true, __FILE__, __LINE__ );
	AvatarNodeMap.clear();
	EmotionNodeMap.clear();
	FacialNodeMap.clear();
	TactileNodeMap.clear();
	RobotNodeMap.clear();
	VideoNodeMap.clear();
	ObjectNodeMap.clear();
	AudioNodeMap.clear();
	PointCloudNodeMap.clear();
	GenericNodeMap.clear();

}


#ifdef WIN32
CLIENT_LIB_EXPORT void stop()
#else
void stop()
#endif
{
	rakPeer->Shutdown(100,0);
	RakNetworkFactory::DestroyRakPeerInterface(rakPeer);
	isconnected=false;
	viewing_only = false;
}

#ifdef WIN32
CLIENT_LIB_EXPORT void exitlibrary()
#else
void exitlibrary()
#endif
{
	exit(0);
}