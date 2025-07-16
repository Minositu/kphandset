#ifndef __KPSCREEN_H__
#define __KPSCREEN_H__

#include <AEEStdLib.h>

class kphandset;

class kpscreen {
public:
	void (*InitPtr)(kpscreen*, int initialize);
	void (*DrawPtr)(kpscreen*);
	bool (*HandleEventPtr)(kpscreen*, AEEEvent eCode, uint16 wParam, uint32 dwParam);
	void (*ReleasePtr)(kpscreen*);

	char help_buffer[20];
	char retrigger_buffer[20];
	uint8 kpstartup_unk11_1;

	static void ExecuteCommand(kpscreen* pScreen);
	static void RefreshDisplay(kphandset* pApp);
	static void FetchSelectedScreen(kphandset* pApp, kpscreen* newScreen);
	static void ClearStartup(kphandset* pApp, void* startup);

	static bool HandleEvent(kpscreen* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam);
	static void Draw(kpscreen* pScreen);
	static void Release(kpscreen* pScreen);
	static void Init(kpscreen* pScreen, int initialize);
};

#endif
