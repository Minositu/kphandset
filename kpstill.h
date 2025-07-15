#ifndef __KPSTILL_H__
#define __KPSTILL_H__

#include "kphandset.h"

class kpstill : public kpscreen {
public:
	IImage* still_Image;
	uint8 still_frameCount;
	void* kpscreen_still_unk3;								//
	kpBottom* bottomScreen;
	int kpscreen_still_unk5_timer;							//
	int still_pingTime;
	int still_keepalive;
	uint8 kpscreen_still_unk8_1;							//
	uint8 kpscreen_still_unk8_2;							//
	char still_audioFile[32];
	uint8 kpscreen_still_unk10;								//
	uint8 kpscreen_still_unk11;								//

	static kpscreen* ExecuteCommand(kphandset* pApp, char* commandBuffer, char* uiImage);
	static void RefreshDisplay(kpstill* pScreen);
	static void func_2DB80(kpstill* pScreen);												//

	static bool HandleEvent(kpstill* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam);
	static void Draw(kpstill* pScreen);
	static void Release(kpstill* pScreen);
	static void Init(kpstill* pScreen, int initialize);
};

#endif
