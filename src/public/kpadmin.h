#ifndef __KPADMIN_H__
#define __KPADMIN_H__

#include "kphandset.h"

class kpadmin : public kpscreen {
public:
	char pavillion[2];
	uint8 kpstartupAdmin_unk2_1;
	uint8 kpstartupAdmin_unk2_2;
	IStatic* pHandsetID;
	IMenuCtl* pMenuCtl;
	char* pNotice;

	static kpscreen* ExecuteCommand(kphandset* pApp);

	static void CreateOutgoingMessage(kphandset* pApp, const char* message);
	static void CommandParser(kpadmin* pScreen, int unk2, char* command);
	static void ParseMenu(kpadmin* pScreen, const char* path);
	static void RefreshDisplay(kpadmin* pScreen, char* pText, char a3);

	static bool HandleEvent(kpadmin* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam);
	static void Draw(kpadmin* pScreen);
	static void Release(kpadmin* pScreen);
	static void Init(kpadmin* pScreen, int initialize);
};

#endif
