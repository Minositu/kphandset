#ifndef __KPWAIT_H__
#define __KPWAIT_H__

#include "kphandset.h"
#include "kpstill.h"

class kpwait : public kpstill {
public:
	static kpscreen* ExecuteCommand(kphandset* pApp, char* commandBuffer);

	static int GetServerTick(kpwait* pScreen);
	static void InitTimeout(kpwait* pScreen);
	static void TimeoutRestarting(kpwait* pScreen);
	static void TimeoutReconnect(kpwait* pScreen);
	static void ServerTickTimer(kpwait* pScreen);
	static void CancelTimers(kpwait* pScreen);


	static void Init(kpwait* pScreen, int initialize);
};

#endif
