#ifndef __KPCUTSCENE_H__
#define __KPCUTSCENE_H__

#include "kphandset.h"

class kpcutscene : public kpscreen {
public:
	char cutsceneFilePath[32];
	uint8 cutsceneLoaded;
	uint8 scriptLoaded;
	char cutsceneData[2048];
	IImage* filmstripImages[5];
	char* filmstripNames[5];
	uint8* subtitleStartFrame;
	kpfilmstrip currentFilmstrip;

	static kpscreen* ExecuteCommand(kphandset* pApp, char* commandBuffer);
	static void LoadCutscene(kpcutscene* pScreen);
	static void StopCutscene(kphandset* pApp, kpcutscene* pScreen);
	static void ParseKPSFile(kphandset* pApp, kpcutscene* pScreen);
	static void PingSocket(kpcutscene* pScreen);
	static void SetReplayBuffer(kphandset* pApp);
	static void UpdateFrame(kpcutscene* pScreen);
	static boolean SubtitleHandler(kpcutscene* pScreen);
	static boolean SplitSubtitles(kpBottom* pBottom, int amount);

	static bool HandleEvent(kpcutscene* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam);
	static void Draw(kpcutscene* pScreen);
	static void Release(kpcutscene* pScreen);
	static void Init(kpcutscene* pScreen, int initialize);
};

#endif
