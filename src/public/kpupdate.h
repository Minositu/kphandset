#ifndef __KPUPDATE_H__
#define __KPUPDATE_H__

#include "AEEFile.h"
#include "AEEWeb.h"
#include "kphandset.h"

struct KPPUpdateBuffer
{
	IFile* updateFile;
	int fileCount;
	int currentFileIndex;
	int selectedFileSize;
	int currentSelectedFileSize;
	unsigned int bufferSize;
	char buffer[516];
};

class kpupdate : kpscreen {
public:
	IImage* ui_waiting;
	IStatic* pUpdatingText;
	char pszURL[128];
	AEECallback pResponseCallback;
	IWeb* pIWeb;
	IWebResp* pWebResp;
	KPPUpdateBuffer updatebuffer;
	uint8 recievedHttpResponse;
	uint8 errorDetected;
	int (*ParseKPPFileCB)(kpupdate*);

	static int ParseBufferInt(int a1);
	static int ParseKPPFile(kpupdate* pScreen);
	static int ParseKPPGetFileSize(kpupdate* pScreen);
	static int ParseKPPFileHandler(kpupdate* pScreen);
	static int ParseKPPWriteFile(kpupdate* pScreen);
	static void WebResponseCB(void* pData);
	static void Shutdown(kpupdate* pUpdate);
	static void UpdateResponse_PFNWEBSTATUS(void* pNotifyData, WebStatus ws, void* pData);
	static kpupdate* InitializeUpdateScreen(kphandset* a1, const char* url);

	static bool HandleEvent(kpupdate* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam);
	static void Draw(kpupdate* pScreen);
	static void Release(kpupdate* pScreen);
	static void Init(kpupdate* pScreen, int initialize);
};

#endif
