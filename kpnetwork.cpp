#include "kpnetwork.h"
#include "kphandset.h"
#include "kphelpers.h"
#include "kpdebug.h"
#include "kpupdate.h"
#include "kptimeout.h"
#include "AEENet.h"

void kpnetwork::InitNetMgr(kphandset* pApp)
{
	pApp->network.ipAddress[0] = 0;
	pApp->network.server[0] = 0;
	pApp->network.socket_address = 0;
	pApp->network.socket_port = 0;
	pApp->network.socketConnectionState = 0;
	pApp->network.idle_time = 0;
	pApp->network.retry_timer_seconds = 0;
	pApp->network.default_keepalive = 3000;
	pApp->network.keepalive = 0;
	pApp->network.base_ping = 30000;
	pApp->network.ping_timer_seconds = 0;
	pApp->network.pNetMgr = 0;
	pApp->network.pSocket = 0;
	pApp->network.pPush = pApp->network.allocatedMessages;
	for (int i = 0; i < 4; ++i)
		pApp->network.allocatedMessages[i].pMessage = &pApp->network.allocatedMessages[i + 1];
	char* base_ping = kphelpers::ReadScriptBuf("$BASE_PING");
	if (*base_ping && *base_ping != 36)
		pApp->network.base_ping = ATOI(base_ping);
	char* default_keep_alive = kphelpers::ReadScriptBuf("$DEFAULT_KEEPALIVE");
	if (*default_keep_alive && *default_keep_alive != 36)
		pApp->network.default_keepalive = ATOI(default_keep_alive);
	pApp->network.keepalive = pApp->network.default_keepalive;
	pApp->network.max_wait_before_reconnect = 75000;
	char* ScriptBuf_sub_29130 = kphelpers::ReadScriptBuf("$MAX_WAIT_BEFORE_RECONNECT");
	if (*ScriptBuf_sub_29130 && *ScriptBuf_sub_29130 != 36)
		pApp->network.max_wait_before_reconnect = ATOI(ScriptBuf_sub_29130);
	pApp->network.max_wait_before_restart = 105000;
	char* max_wait_before_restart = kphelpers::ReadScriptBuf("$MAX_WAIT_BEFORE_RESTART");
	if (*max_wait_before_restart && *max_wait_before_restart != 36)
		pApp->network.max_wait_before_restart = ATOI(max_wait_before_restart);
	pApp->network.wait_server_tick = 20000;
	char* wait_server_tick = kphelpers::ReadScriptBuf("$WAIT_SERVER_TICK");
	if (*wait_server_tick && *wait_server_tick != 36)
		pApp->network.wait_server_tick = ATOI(wait_server_tick);
	pApp->network.pOutgoing = 0;
	STRCPY(pApp->network.ipAddress, "153.6.251.144");
	char* ipAddress = kphelpers::ReadFromAppPath(pApp, (char*)"_server.txt", "153.6.251.144");
	if (ipAddress)
		kphelpers::NullTerminatedString(pApp->network.ipAddress, (const char*)ipAddress, 22);
	if (!pApp->network.ipAddress[0])
		STRCPY(pApp->network.ipAddress, "153.6.251.144");
	kpnetwork::SetServer(pApp, pApp->network.ipAddress, 0);
	kpdebug::Print((char*)"NET: Initializing");
	int instanceError = ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_NET, (void**)&pApp->network.pNetMgr);
	if (instanceError)
		kpdebug::AssertLine("NET: Failed to initialize NetMgr (0x%x)", instanceError);
}

void kpnetwork::Initialize(kphandset* pApp)
{
	kpnetwork::SocketWriteInitialMessage(pApp);
	ISHELL_SetTimer(pApp->m_pIShell, 15000, (PFNNOTIFY)kpnetwork::Initialize, pApp);
}

void kpnetwork::Shutdown(kphandset* pApp)
{
	kpdebug::Print((char*)"NET: Shutting Down");
	pApp->network.ping_timer_seconds = 0;
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::SocketIdleChecker, pApp);
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::SocketPing, pApp);
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::ConnectionRetry, pApp);
	kpnetwork::CloseSocket(pApp);
	kpnetwork::ClearIncoming(pApp);
	kpnetwork::ClearOutgoing(pApp);
	if (pApp->network.pNetMgr)
	{
		INETMGR_Release(pApp->network.pNetMgr);
		pApp->network.pNetMgr = 0;
	}
}

void kpnetwork::SocketWriteInitialMessage(kphandset* pApp)
{
	if (!pApp->network.pOutgoing)
	{
		int count = SNPRINTF(pApp->scratch, 400, "<message handset='%s' device='vx8350' version='%s' lowBattery='%s' value='connect' software='%s' />", pApp->handsetID, pApp->contentVersion, pApp->lowBattery ? "true" : "false", "1004-22");
		ASSERT(count >= sizeof(pApp->scratch), "count < sizeof(pApp->scratch)"); //Line 699
		if (count < sizeof(pApp->scratch) && count > 0)
			kpnetwork::CreateOutgoingMessage(pApp, pApp->scratch, 0);
	}
}

void kpnetwork::CreateOutgoingMessage(kphandset* pApp, const char* message, int bSimpleMessage)
{
	NetworkMessage* pOutgoingMessage = 0;
	if (message && *message)
	{
		pOutgoingMessage = kpnetwork::CreateNetworkMessage(pApp);
		if (pOutgoingMessage)
		{
			if (bSimpleMessage)
				kpnetwork::WriteSimpleNetworkMessage(pOutgoingMessage, message, (const char*)&pApp->script);
			else
				kpnetwork::WriteNetworkMessage(pOutgoingMessage, message);
			kpdebug::Assert("NET: Pushing %s", &pOutgoingMessage->buf[2], 0);
			if (pApp->network.pOutgoing)
			{
				NetworkMessage* i;
				for (i = pApp->network.pOutgoing; i->pMessage; i = i->pMessage);
				i->pMessage = pOutgoingMessage;
			}
			else
			{
				pApp->network.pOutgoing = pOutgoingMessage;
			}
		}
		else
		{
			kpdebug::Print((char*)"NET: Error allocating Outgoing Message");
		}
	}
	if (!pOutgoingMessage)
		return;
	if (pApp->network.pOutgoing == pOutgoingMessage)
		kpnetwork::SocketWriteOutgoingMessage(pApp);
}

void kpnetwork::CreateSimpleMessage(kphandset* pApp, const char* message)
{
	kpnetwork::CreateOutgoingMessage(pApp, message, 1);
}

NetworkMessage* kpnetwork::CreateNetworkMessage(kphandset* pApp)
{
	NetworkMessage* pPush = pApp->network.pPush;
	if (pPush)
	{
		pApp->network.pPush = pApp->network.pPush->pMessage;
		AllocateNetworkMessage(pPush);
	}
	return pPush;
}

void kpnetwork::AllocateNetworkMessage(NetworkMessage* pMessage)
{
	MEMSET(pMessage, 0, 192);
	pMessage->messageLength = 0;
	pMessage->sent = 0;
	pMessage->pMessage = 0;
}

void kpnetwork::WriteSimpleNetworkMessage(NetworkMessage* pNetworkMessage, const char* pMessage, const char* script)
{
	uint16 messageLength = 0;
	kphandset* pApp = (kphandset*)GETAPPINSTANCE();
	if (script && *script)
	{
		messageLength = STRLEN(script) + (STRLEN(pMessage) + (STRLEN(pApp->handsetID) + 42));
		if (messageLength > 189)
		{
			messageLength = 189;
			kpdebug::Print((char*)"NET: SIMPLE MESSAGE TOO LONG!");
		}
		SNPRINTF(&pNetworkMessage->buf[2], messageLength + 1, "<message handset='%s' mission='%s' value='%s' />", pApp->handsetID, script, pMessage);
	}
	else
	{
		messageLength = STRLEN(pMessage) + (STRLEN(pApp->handsetID) + 31);
		if (messageLength > 189)
		{
			messageLength = 189;
			kpdebug::Print((char*)"NET: SIMPLE MESSAGE TOO LONG!");
		}
		SNPRINTF(&pNetworkMessage->buf[2], messageLength + 1, "<message handset='%s' value='%s' />", pApp->handsetID, pMessage);
	}
	pNetworkMessage->buf[0] = messageLength & 0xFF;
	pNetworkMessage->buf[1] = (messageLength & 0xFF00) >> 8;
	pNetworkMessage->buf[messageLength + 2] = 0;
	pNetworkMessage->messageLength = messageLength + 2;
}

void kpnetwork::WriteNetworkMessage(NetworkMessage* pNetworkMessage, const char* message)
{
	uint16 messageLength = STRLEN(message);
	if (messageLength > 189)
	{
		messageLength = 189;
		kpdebug::Print((char*)"NET: TRUNCATING MESSAGE!");
	}
	kphelpers::NullTerminatedString(&pNetworkMessage->buf[2], message, 190);
	pNetworkMessage->buf[0] = messageLength & 0xFF;
	pNetworkMessage->buf[1] = (messageLength & 0xFF00) >> 8;
	pNetworkMessage->buf[messageLength + 2] = 0;
	pNetworkMessage->messageLength = messageLength + 2;
}

void kpnetwork::SocketWriteOutgoingMessage(kphandset* pApp)
{
	if (pApp->network.pOutgoing && !pApp->network.socketConnectionState)
		kpnetwork::SocketConnect(pApp, -1);
	if (pApp->network.pOutgoing && pApp->network.socketConnectionState == 2)
	{
		pApp->network.idle_time = 0;
		if (pApp->network.pOutgoing->messageLength != pApp->network.pOutgoing->sent)
		{
			uint16 sent = ISOCKET_Write(pApp->network.pSocket, (byte*)&pApp->network.pOutgoing->buf[pApp->network.pOutgoing->sent], pApp->network.pOutgoing->messageLength - pApp->network.pOutgoing->sent);
			if (sent <= 0)
			{
				if (sent == -2)
				{
					ISOCKET_Writeable(pApp->network.pSocket, (PFNNOTIFY)kpnetwork::SocketWriteOutgoingMessage, pApp);
				}
				else
				{
					kpnetwork::CloseSocket(pApp);
					kpdebug::AssertLine("NET: Socket Write Failed (0x%x)", ISOCKET_GetLastError(pApp->network.pSocket));
					pApp->network.pOutgoing->sent = 0;
					ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_NETWORK_WRITE_ERROR, 0, 0);
				}
			}
			else
			{
				kpdebug::Assert("NET: Writing %s (%u)", &pApp->network.pOutgoing->buf[2], sent);
				pApp->network.pOutgoing->sent += sent;
			}
		}
		if (kpnetwork::CheckMessageLength(pApp->network.pOutgoing))
		{
			pApp->network.pOutgoing = pApp->network.pOutgoing->pMessage;
			kpnetwork::LogIncomingTraffic(pApp, pApp->network.pOutgoing);
		}
	}
}

void kpnetwork::SocketConnect(kphandset* pApp, int state)
{
	int lastError = state;
	int socketFailed = 0;
	if (pApp->network.socketConnectionState)
	{
		if (pApp->network.socketConnectionState != 1)
			return;
		if (!state)
		{
			kpdebug::Print((char*)"NET: Socket Connected");
			kpnetwork::SocketRefreshIdleTime(pApp, 2);
			return;
		}
	}
	else
	{
		pApp->network.pSocket = INETMGR_OpenSocket(pApp->network.pNetMgr, AEE_SOCK_STREAM);
		if (pApp->network.pSocket)
		{
			lastError = ISOCKET_Connect(pApp->network.pSocket, pApp->network.socket_address, pApp->network.socket_port, (PFNCONNECTCB)kpnetwork::SocketConnect, pApp);
			if (!lastError)
			{
				kpnetwork::SocketRefreshIdleTime(pApp, 1);
				return;
			}
		}
	}
	++pApp->network.retry_timer_seconds;
	socketFailed = 1;
	lastError = INETMGR_GetLastError(pApp->network.pNetMgr);
	if (socketFailed)
	{
		kpdebug::AssertLine("NET: Open Socket Failed (0x%x)", lastError);
		if (pApp->adminMode)
		{
			ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_NETWORK_CONNECTION_ERROR, 0, lastError);
		}
		else if (pApp->network.retry_timer_seconds <= 3)
		{
			kpnetwork::CloseSocket(pApp);
			ISHELL_SetTimer(pApp->m_pIShell, 3000 * pApp->network.retry_timer_seconds, (PFNNOTIFY)kpnetwork::ConnectionRetry, pApp);
		}
		else
		{
			kpnetwork::ClearExistingConnection(pApp);
		}
	}
}

void kpnetwork::SocketRefreshIdleTime(kphandset* pApp, int state)
{
	pApp->network.idle_time = 0;
	if (pApp->network.socketConnectionState != state)
	{
		pApp->network.socketConnectionState = state;
		if (pApp->network.socketConnectionState == 2)
		{
			pApp->network.retry_timer_seconds = 0;
			ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::ConnectionRetry, pApp);
			kpnetwork::SocketIdleChecker(pApp);
			ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_NETWORK_CONNECTION_RETRY, 0, 0);
		}
		else if (!pApp->network.socketConnectionState)
		{
			if (pApp->network.ping_timer_seconds)
				ISHELL_SetTimer(pApp->m_pIShell, pApp->network.ping_timer_seconds, (PFNNOTIFY)kpnetwork::SocketPing, pApp);
		}
	}
}

void kpnetwork::ConnectionRetry(kphandset* pApp)
{
	if (pApp->network.pOutgoing)
	{
		if (!pApp->network.socketConnectionState)
		{
			kpdebug::Print((char*)"NET: Connection Retry");
			kpnetwork::SocketConnect(pApp, -1);
		}
	}
}

void kpnetwork::StartSocketPingTimer(kphandset* pApp, int ping_timer_seconds)
{
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::SocketPing, pApp);
	if (!pApp->network.socketConnectionState)
	{
		if (pApp->network.ping_timer_seconds)
			ISHELL_SetTimer(pApp->m_pIShell, pApp->network.ping_timer_seconds, (PFNNOTIFY)kpnetwork::SocketPing, pApp);
	}
}

void kpnetwork::SocketPing(kphandset* pApp)
{
	if (!pApp->network.pOutgoing)
		kpnetwork::CreateSimpleMessage(pApp, "ping");
}

void kpnetwork::LogIncomingTraffic(kphandset* pApp, NetworkMessage* pNetworkMessage)
{
	if (pNetworkMessage)
	{
		pNetworkMessage->pMessage = pApp->network.pPush;
		pApp->network.pPush = pNetworkMessage;
		kpnetwork::LogTraffic(pApp);
	}
}

void kpnetwork::LogTraffic(kphandset* pApp)
{
	int outgoing = 0;
	int incoming = 0;
	int free = 0;
	int cached = 0;
	for (NetworkMessage* i = pApp->network.pOutgoing; i; i = i->pMessage)
		++outgoing;
	for (NetworkMessage* j = pApp->network.pIncoming; j; j = j->pMessage)
		++incoming;
	for (NetworkMessage* k = pApp->network.pPush; k; k = k->pMessage)
		++free;
	for (NetworkMessage* m = pApp->pPop; m; m = m->pMessage)
		++cached;
	//kpdebug::kpdebug_sub_2776C((kphandset*)GETAPPINSTANCE(), "NET: IN:%lu/OUT:%lu FREE:%lu", incoming, outgoing, free);
	if (cached)
		kpdebug::AssertLine("NET: CACHED:%lu", cached);
	if (incoming + outgoing + free + cached != 5)
		kpdebug::Print((char*)"NET: UNACCOUNTED MESSAGES!");
}

void kpnetwork::ParseIncomingMessage(kphandset* pApp, const char* message)
{
	if (STRIBEGINS("<state", message))
	{
		int v4 = 0;
		char* episode = kphandset::kphandset_ReadFromScratch(message, "episode");
		if (episode && *episode && (*episode != pApp->episode[0] || episode[1] != pApp->episode[1]))
		{
			kphandset::kpsys_SetPavilion(pApp, episode);
			v4 = 1;
		}
		char* mission = kphandset::kphandset_ReadFromScratch(message, "mission");
		if (mission && *mission && STRICMP(mission, (const char*)&pApp->script))
		{
			char* v8 = kphelpers::ReadScriptFromMemory(pApp, mission);
			if (v8)
				kphandset::kpsys_LoadScript(pApp, v8);
			v4 = 1;
		}
		char* value = kphandset::kphandset_ReadFromScratch(message, "value");
		if (value && *value && !STRICMP(value, "doreset"))
		{
			ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_RESTART, 0, 0);
		}
		else if (value)
		{
			if (*value)
			{
				char* v12 = kphelpers::ReadScriptFromMemory(pApp, value);
				if (v4 || v12 && STRCMP(v12, pApp->kphandset_unk98_1))
				{
					pApp->kphandset_unk98_1[0] = 0;
					kphandset::kpsys_ReadScript(pApp, v12);
				}
			}
		}
	}
	else if (STRIBEGINS("<update", message))
	{
		char* url = kphandset::kphandset_ReadFromScratch(message, "url");
		if (url && *url)
		{
			kpscreen* updated = (kpscreen*)kpupdate::InitializeUpdateScreen(pApp, url);
			kphandset::kpscreen_FetchSelectedScreen(pApp, updated);
		}
	}
	else if (STRIBEGINS("<broadcast", message))
	{
		const char* v18 = kphandset::kphandset_ReadFromScratch(message, "text");
		kphandset::DisplayMessageBox(pApp, v18);
		kphandset::kphandset_ReadFromScratch(message, "halt");
	}
	else if (STRIBEGINS("<assign", message))
	{
		char* handset = kphandset::kphandset_ReadFromScratch(message, "handset");
		if (handset && *handset)
		{
			STRNCPY(pApp->handsetID, handset, 8);
			kphelpers::UpdateFile(pApp, "handsetid.txt", (int)pApp->handsetID, STRLEN(pApp->handsetID));
			ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_RESTART, 0, 0);
		}
	}
	else if (STRIBEGINS("<reconnect", message))
	{
		char* language = kphandset::kphandset_ReadFromScratch(message, "language");
		if (language && *language && language[1] && !language[2])
			kphandset::kpsys_SetLanguage(pApp, language);
		char* ip = kphandset::kphandset_ReadFromScratch(message, "ip");
		if (ip && *ip)
		{
			if (kpnetwork::SetServer(pApp, ip, 0))
				kpnetwork::SocketWriteInitialMessage(pApp);
			pApp->network.keepalive = 15000;
			kpnetwork::StartSocketPingTimer(pApp, 5000);
		}
		else
		{
			kpnetwork::ClearExistingConnection(pApp);
		}
	}
	else if (STRIBEGINS("<lost", message))
	{
		if (!pApp->pTranslatorMgr.length)
			kphandset::kpsys_SetPavilion(pApp, "cn");
		kptimeout::ClearTimeout(pApp);
		pApp->globalTimeoutState = 3;
		ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_TIMEOUT, 0, 0);
	}

}

bool kpnetwork::CheckMessageLength(NetworkMessage* pNetworkMessage)
{
	return pNetworkMessage->messageLength && pNetworkMessage->messageLength == pNetworkMessage->sent;
}

NetworkMessage* kpnetwork::IsMessageReady(kphandset* pApp)
{
	NetworkMessage* pReady = 0;
	if (pApp->network.pIncoming)
	{
		NetworkMessage* pCurrentMessage = 0;
		for (pReady = pApp->network.pIncoming; pReady->pMessage; pReady = pReady->pMessage)
			pCurrentMessage = pReady;
		if (kpnetwork::CheckMessageLength(pReady))
		{
			if (pCurrentMessage)
			{
				pCurrentMessage->pMessage = 0;
			}
			else
			{
				ASSERT(pApp->network.pIncoming != pReady, "pReady == pApp->network.network.pIncoming"); //Line 504
				pApp->network.pIncoming = 0;
			}
		}
		else
		{
			return 0;
		}
	}
	return pReady;
}

void kpnetwork::ClearIncoming(kphandset* pApp)
{
	kpdebug::Print((char*)"NET: Clear Incoming");
	while (pApp->network.pIncoming)
	{
		pApp->network.pIncoming = pApp->network.pIncoming->pMessage;
		kpnetwork::LogIncomingTraffic(pApp, pApp->network.pIncoming);
	}
}

void kpnetwork::ClearOutgoing(kphandset* pApp)
{
	kpdebug::Print((char*)"NET: Clear Outgoing");
	while (pApp->network.pOutgoing)
	{
		pApp->network.pOutgoing = pApp->network.pOutgoing->pMessage;
		kpnetwork::LogIncomingTraffic(pApp, pApp->network.pOutgoing);
	}
}

void kpnetwork::CancelConnection(kphandset* pApp)
{
	if (!pApp->network.pOutgoing && pApp->network.socketConnectionState == 2)
		kpnetwork::CloseSocket(pApp);
}

void kpnetwork::CloseSocket(kphandset* pApp)
{
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::SocketIdleChecker, pApp);
	if (pApp->network.pSocket)
	{
		ISOCKET_Cancel(pApp->network.pSocket, 0, 0);
		if (pApp->network.pSocket)
		{
			ISOCKET_Release(pApp->network.pSocket);
			pApp->network.pSocket = 0;
		}
		kpnetwork::SocketRefreshIdleTime(pApp, 0);
	}
}

void kpnetwork::SocketIdleChecker(kphandset* pApp)
{
	pApp->network.idle_time += 250;
	kpnetwork::SocketWriteOutgoingMessage(pApp);
	kpnetwork::SocketReadIncomingMessage(pApp);
	if (pApp->network.socketConnectionState == 2 && pApp->network.idle_time >= pApp->network.keepalive && pApp->network.keepalive)
	{
		kpdebug::Print((char*)"NET: Closing idle socket");
		kpnetwork::CloseSocket(pApp);
	}
	if (pApp->network.socketConnectionState == 2)
		pApp->m_pIShell->pvt->SetTimer(pApp->m_pIShell, 250, (PFNNOTIFY)kpnetwork::SocketIdleChecker, pApp);
}

void kpnetwork::SocketReadIncomingMessage(kphandset* pApp)
{
	if (pApp->network.socketConnectionState == 2)
	{
		NetworkMessage* pIncoming = pApp->network.pIncoming;
		if (!pIncoming || kpnetwork::CheckMessageLength(pApp->network.pIncoming))
		{
			NetworkMessage* pNetworkMessage = kpnetwork::CreateNetworkMessage(pApp);
			pIncoming = pNetworkMessage;
			if (pNetworkMessage)
			{
				pNetworkMessage->messageLength = 2;
				pNetworkMessage->pMessage = pApp->network.pIncoming;
				pApp->network.pIncoming = pNetworkMessage;
			}
		}
		if (pIncoming)
		{
			char* pBuf = &pIncoming->buf[pIncoming->sent];
			int messageLength = pIncoming->messageLength;
			int sent = pIncoming->sent;
			int bufSize = messageLength - sent;
			if (messageLength - sent > 0)
			{
				if (bufSize > 2)
					kpdebug::AssertLine("NET: Wanting %lu bytes", messageLength - sent);
				int readError = ISOCKET_Read(pApp->network.pSocket, pBuf, bufSize);
				if (readError <= 0)
				{
					if (readError == -2)
					{
						ISOCKET_Writeable(pApp->network.pSocket, (PFNNOTIFY)kpnetwork::SocketReadIncomingMessage, pApp);
					}
					else
					{
						if (readError)
						{
							kpdebug::AssertLine("NET: Socket Read Failed - error 0x%x", pApp->network.pSocket->pvt->GetLastError(pApp->network.pSocket));
						}
						else
						{
							kpdebug::Print((char*)"NET: Socket Closed by Server");
						}
						kpnetwork::CloseSocket(pApp);
					}
				}
				else
				{
					pApp->network.idle_time = 0;
					pIncoming->sent += readError;
					kpdebug::AssertLine("NET: Read %lu bytes", readError);
					if (pIncoming->sent == 2)
					{
						if (pIncoming->buf[1] + (pIncoming->buf[0] << 8) <= 189)
						{
							pIncoming->messageLength = pIncoming->buf[1] + (pIncoming->buf[0] << 8) + 2;
							kpnetwork::SocketReadIncomingMessage(pApp);
						}
						else
						{
							kpdebug::Print((char*)"NET: MESSAGE LENGTH OVERFLOW!");
							kpnetwork::CloseSocket(pApp);
							ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_NETWORK_READ_ERROR, 0, 0);
						}
					}
					if (kpnetwork::CheckMessageLength(pIncoming))
					{
						if (pIncoming->sent > 2u)
						{
							kpdebug::AssertLine("NET: read %s", (int)&pIncoming->buf[2], 0);
							ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_NETWORK_INCOMING_MESSAGE, 0, (uint32)pIncoming);
						}
					}
				}
			}
		}
		else
		{
			kpdebug::Print((char*)"NET: Error allocating Incoming Message");
		}
	}
}

void kpnetwork::ClearExistingConnection(kphandset* pApp)
{
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::SocketIdleChecker, pApp);
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::SocketPing, pApp);
	ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kpnetwork::ConnectionRetry, pApp);
	kpnetwork::CloseSocket(pApp);
	kpnetwork::ClearIncoming(pApp);
	kpnetwork::ClearOutgoing(pApp);
	kpnetwork::SetServer(pApp, pApp->network.ipAddress, 1234);
	kpnetwork::SocketWriteInitialMessage(pApp);
	pApp->network.keepalive = 15000;
	kpnetwork::StartSocketPingTimer(pApp, 0);
}

int kpnetwork::SetServer(kphandset* pApp, char* server, int serverPort)
{
	if (!server)
		server = pApp->network.ipAddress;
	if (!serverPort)
	{
		char* addressDelimiter = STRCHR(server, ':');
		if (addressDelimiter)
			serverPort = ATOI(addressDelimiter + 1);
	}
	if (!serverPort)
		serverPort = 1234;
	INAddr address = 0;
	if (INET_ATON(server, &address) != 1)
		return 0;
	INPort port = SWAPS(serverPort);
	if (pApp->network.socket_address == address && pApp->network.socket_port == port)
		return 0;
	kpnetwork::ClearOutgoing(pApp);
	kpnetwork::CloseSocket(pApp);
	pApp->network.socket_address = address;
	pApp->network.socket_port = port;
	INET_NTOA(pApp->network.socket_address, pApp->network.server, 22);
	kpdebug::AssertLine("NET: Set server %s", (int)pApp->network.server, 0);
	return 1;
}