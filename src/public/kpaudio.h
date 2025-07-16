#ifndef __KPAUDIO_H__
#define __KPAUDIO_H__

#include <AEEMedia.h>

class kpaudio {
public:
	uint8 KPAudio_unk1_1;
	uint8 KPAudio_unk1_2;
	uint8 KPAudio_unk1_3;
	uint8 KPAudio_unk1_4;
	void* KPAudio_unk2;
	void* KPAudio_unk3;
	void* KPAudio_unk4;
	void* KPAudio_unk5;
	void* KPAudio_unk6;
	void* KPAudio_unk7;
	void* KPAudio_unk8;
	char songPath[32];
	char audioBuffer[64];
	AEEMediaData pMediaData;
	IMedia* pMedia;
	void* pTotalTimeCmdData;
	uint32 PausedUpTimeMS;
	uint32 UpTimeMS;
	void* pAbortCmdData;
	byte audioStatus;

	static void InitAudioMgr(class kphandset* pApp);
	static void StopMedia(kpaudio* pAudio);
	static int GetUpTimeMS(kphandset* pApp);
	static void Free(kphandset* pApp);
	static void Initialize(kpaudio* pAudio);

	static void LoadAudioFile(const char* path);
	static void PlayMedia(kpaudio* pAudio);
	static void MediaTimer(kpaudio* pAudio);
	static void MediaNotify(kphandset* pApp, AEEMediaCmdNotify* pCmdNotify);
};

#endif
