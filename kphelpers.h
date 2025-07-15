#pragma once

#include "AEEStdLib.h"

class kphandset;

class kphelpers {
public:
	static IImage* LoadResObject(kphandset* pApp, const char* pImagePath);
	static IImage* LoadUIImages(kphandset* pApp, char* pName);
	static void NullTerminatedString(char* pDest, const char* pSrc, int size);
	static IImage* PreloadFilmstrip(kphandset* pApp, char* pFilmstrip);
	static char* CreateStringTranslationPath(kphandset* pApp, char* pFilename, const char* pExtension);
	static char* ReadScriptFromMemory(kphandset* pApp, char* pSrc);
	static char* ReadToScratch(kphandset* pApp, char* pName, char* pExtension);
	static char* ReadFromEpisodePath(kphandset* pApp, const char* pFilename, const char* pExtension);
	static char* ReadFromLangPath(kphandset* pApp, const char* pFilename, const char* pExtension);
	static char* ReadFromRootSD(kphandset* pApp, const char* pFilename, const char* pExtension);
	static char* ReadFromAppPath(kphandset* pApp, char* file, const char* fallbackData);
	static char* BuildPath(kphandset* pApp, char* pFilename, unsigned int filenameLength);
	static void UpdateFile(kphandset* result, char* pszFile, int pBuffer, int dwCount);
	static char* LoadStringFromFile(kphandset* pApp, const char* fileName, char* suppliedBuffer, unsigned int* outSize);
	static char* ReadScriptBuf(const char* buf);
	static char* ParseTranslation(kphandset* pApp, const char* translation);

	//Tokens
	static void ParseTokenizer(char* tokenizerBuf, const char* tokenizer);
	static bool CheckToken(int token);

	static char* FetchMainToken(char* token);
	static char* FetchSubToken(char* token);
	static void FetchGoToToken(kphandset* pApp, const char* script);
};