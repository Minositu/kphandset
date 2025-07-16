#ifndef __KPCLIENT_H__
#define __KPCLIENT_H__

#include "kphandset.h"

class kpclient : public kpscreen {
public:
	IImage* kpstartup_client_unk1;
	IStatic* pHandsetID;
	IStatic* pVersion;
	IStatic* pNotice;

	static kpscreen* ExecuteCommand(kphandset* pApp);

	static bool HandleEvent(kpclient* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam);
	static void Draw(kpclient* pScreen);
	static void Release(kpclient* pScreen);
	static void Init(kpclient* pScreen, int initialize);
};

#endif
