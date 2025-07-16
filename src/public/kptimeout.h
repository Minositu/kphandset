#ifndef __KPTIMEOUT_H__
#define __KPTIMEOUT_H__

#include "kphandset.h"

class kptimeout {
public:
	static void InitGlobalTimeoutMgr(kphandset* pApp);
	static void HandleTimeout(kphandset* pApp);
	static void CheckTimeout(kphandset* pApp);
	static void ClearTimeout(kphandset* pApp);
};

#endif
