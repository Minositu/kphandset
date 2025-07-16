#include "kpwait.h"
#include "kphelpers.h"
#include "kpnetwork.h"
#include "kpdebug.h"

kpscreen* kpwait::ExecuteCommand(kphandset* pApp, char* commandBuffer)
{
	pApp->kphandset_unk98_1[0] = 0;
	kpstill* screen = (kpstill*)kpstill::ExecuteCommand(pApp, commandBuffer, "ui_waiting");
	if (screen)
	{
		screen->InitPtr = (void (*)(kpscreen*, int))kpwait::Init;
		char* scriptBuf = kphelpers::ReadScriptBuf("$WAIT_PING");
		screen->still_pingTime = ATOI(scriptBuf);
		scriptBuf = kphelpers::ReadScriptBuf("$WAIT_KEEPALIVE");
		screen->still_keepalive = ATOI(scriptBuf);
		screen->advance_enabled = 0;
		screen->replay_enabled = 0;
	}
	return screen;
}

int kpwait::GetServerTick(kpwait* pScreen)
{
	kphandset* instance = (kphandset*)GETAPPINSTANCE();
	if (pScreen->serverTick >= instance->network.wait_server_tick)
		return pScreen->serverTick + 2000;
	return instance->network.wait_server_tick;
}

void kpwait::InitTimeout(kpwait* pScreen)
{
	kphandset* instance = (kphandset*)GETAPPINSTANCE();
	ISHELL_SetTimer(instance->m_pIShell, kpwait::GetServerTick(pScreen), (PFNNOTIFY)kpwait::ServerTickTimer, pScreen);
	//ISHELL_SetTimer(instance->m_pIShell, instance->network.max_wait_before_reconnect, (PFNNOTIFY)kpwait::TimeoutReconnect, pScreen); //Crashes
	ISHELL_SetTimer(instance->m_pIShell, instance->network.max_wait_before_restart, (PFNNOTIFY)kpwait::TimeoutRestarting, pScreen);
}

void kpwait::TimeoutRestarting(kpwait* pScreen)
{
	kpdebug::Print("WAIT: MAX TIMEOUT - RESTARTING");
	kphandset* instance = (kphandset*)GETAPPINSTANCE();
	ISHELL_PostEvent(instance->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_RESTART, 0, 0);
	ISHELL_SetTimer(instance->m_pIShell, instance->network.max_wait_before_restart, (PFNNOTIFY)kpwait::TimeoutRestarting, pScreen);
}

void kpwait::TimeoutReconnect(kpwait* pScreen)
{
	kpdebug::Print("WAIT: TIMEOUT - RECONNECTING");
	kphandset* instance = (kphandset*)GETAPPINSTANCE();
	kpnetwork::ClearExistingConnection(instance);
	ISHELL_SetTimer(instance->m_pIShell, instance->network.max_wait_before_reconnect, (PFNNOTIFY)kpwait::TimeoutReconnect, pScreen);
}

void kpwait::ServerTickTimer(kpwait* pScreen)
{
	kphandset* instance = (kphandset*)GETAPPINSTANCE();
	int serverTick = kpwait::GetServerTick(pScreen);
	ISHELL_SetTimer(instance->m_pIShell, serverTick, (PFNNOTIFY)kpwait::ServerTickTimer, pScreen);
}

void kpwait::CancelTimers(kpwait* pScreen)
{
	kphandset* instance = (kphandset*)GETAPPINSTANCE();
	ISHELL_CancelTimer(instance->m_pIShell, (PFNNOTIFY)kpwait::ServerTickTimer, pScreen);
	ISHELL_CancelTimer(instance->m_pIShell, (PFNNOTIFY)kpwait::TimeoutReconnect, pScreen);
	ISHELL_CancelTimer(instance->m_pIShell, (PFNNOTIFY)kpwait::TimeoutRestarting, pScreen);
}

void kpwait::Init(kpwait* pScreen, int initialize)
{
	kpstill::Init(pScreen, initialize);
	if (initialize)
	{
		kphandset* instance = (kphandset*)GETAPPINSTANCE();
		kpwait::InitTimeout(pScreen);
		kphandset::kpscr_func_2AB8C(instance);
	}
	else
	{
		kpwait::CancelTimers(pScreen);
	}
}