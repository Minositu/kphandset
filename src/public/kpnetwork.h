#pragma once

#include "AEEStdLib.h"
#include "AEENet.h"

class kphandset;

struct NetworkMessage
{
	char buf[192];
	uint16 messageLength;
	uint16 sent;
	NetworkMessage* pMessage;
};

class kpnetwork {
public:
	char ipAddress[22];
	char server[22];
	INAddr socket_address;
	uint16 socket_port;
	uint8 socketConnectionState;
	int idle_time;
	int retry_timer_seconds;
	int default_keepalive;
	int keepalive;
	int base_ping;
	int ping_timer_seconds;
	int max_wait_before_reconnect;
	int max_wait_before_restart;
	int wait_server_tick;
	INetMgr* pNetMgr;
	ISocket* pSocket;
	NetworkMessage* pOutgoing;
	NetworkMessage* pIncoming;
	NetworkMessage* pPush;
	NetworkMessage allocatedMessages[5];

	static void InitNetMgr(kphandset* pApp);
	static void Initialize(kphandset* pApp);
	static void Shutdown(kphandset* pApp);
	static void SocketWriteInitialMessage(kphandset* pApp);
	static void CreateOutgoingMessage(kphandset* pApp, const char* message, int bSimpleMessage);
	static void CreateSimpleMessage(kphandset* pApp, const char* message);
	static NetworkMessage* CreateNetworkMessage(kphandset* pApp);
	static void AllocateNetworkMessage(NetworkMessage* pMessage);
	static void WriteSimpleNetworkMessage(NetworkMessage* pNetworkMessage, const char* pMessage, const char* script);
	static void WriteNetworkMessage(NetworkMessage* pNetworkMessage, const char* message);
	static void SocketWriteOutgoingMessage(kphandset* pApp);
	static void SocketConnect(kphandset* pApp, int state);
	static void SocketRefreshIdleTime(kphandset* pApp, int state);
	static void ConnectionRetry(kphandset* pApp);
	static void StartSocketPingTimer(kphandset* pApp, int ping_timer_seconds);
	static void SocketPing(kphandset* pApp);
	static void LogIncomingTraffic(kphandset* pApp, NetworkMessage* pNetworkMessage);
	static void LogTraffic(kphandset* pApp);
	static void ParseIncomingMessage(kphandset* pApp, const char* message);
	static bool CheckMessageLength(NetworkMessage* pNetworkMessage);
	static NetworkMessage* IsMessageReady(kphandset* pApp);
	static void ClearIncoming(kphandset* pApp);
	static void ClearOutgoing(kphandset* pApp);
	static void CancelConnection(kphandset* pApp);
	static void CloseSocket(kphandset* pApp);
	static void SocketIdleChecker(kphandset* pApp);
	static void SocketReadIncomingMessage(kphandset* pApp);
	static void ClearExistingConnection(kphandset* pApp);
	static int SetServer(kphandset* pApp, char* server, int serverPort);
};