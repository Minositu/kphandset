#include "kphelpers.h"
#include "kphandset.h"
#include "kpdebug.h"

IImage* kphelpers::LoadResObject(kphandset* pApp, const char* pImagePath)
{
    IImage* pImage = 0;
    if (pImagePath && *pImagePath)
        pImage = ISHELL_LoadResImage(pApp->m_pIShell, pImagePath, 0, 0);
    if (!pImage)
        kpdebug::Assert("IMG: Unable to load %s", pImagePath, 0);
    return pImage;
}

IImage* kphelpers::LoadUIImages(kphandset* pApp, char* pName)
{
    IImage* selectedInterface = 0;
    if (pApp->ui_full_Interface && STRCMP("ui_waiting", pName) == 0)
    {
        selectedInterface = pApp->ui_waiting_Interface;
    }
    else if (pApp->ui_full_Interface && STRCMP("ui_full", pName) == 0)
    {
        selectedInterface = pApp->ui_full_Interface;
    }
    else if (pApp->ui_bottom_Interface && STRCMP("ui_bottom", pName) == 0)
    {
        selectedInterface = pApp->ui_bottom_Interface;
    }
    else if (pApp->ui_bottom_clear_Interface && STRCMP("ui_bottom_clear", pName) == 0)
    {
        selectedInterface = pApp->ui_bottom_clear_Interface;
    }
    if (selectedInterface)
    {
        IIMAGE_AddRef(selectedInterface);
    }
    else
    {
        char* image = kphelpers::ReadToScratch(pApp, pName, (char*)".png");
        if (image && *image)
            return kphelpers::LoadResObject(pApp, image);
    }
    return selectedInterface;
}

void kphelpers::NullTerminatedString(char* pDest, const char* pSrc, int size)
{
    STRNCPY(pDest, pSrc, size);
    pDest[size - 1] = 0;
}

IImage* kphelpers::PreloadFilmstrip(kphandset* pApp, char* pFilmstrip)
{
    uint32 UpTimeMS = -1;
    int use = IMAGE_CACHE_SIZE;
    int filmstripInBuffer = 0;
    IImage* Image = nullptr;
    for (int i = 0; i < IMAGE_CACHE_SIZE; ++i)
    {
        if (pApp->filmstripBuffer[i].Image)
        {
            if (STRCMP((const char*)&pApp->filmstripBuffer[i], pFilmstrip) == 0)
            {
                use = i;
                filmstripInBuffer = 1;
                break;
            }
            if (pApp->filmstripBuffer[i].UpTimeMS < UpTimeMS)
            {
                use = i;
                UpTimeMS = pApp->filmstripBuffer[i].UpTimeMS;
            }
        }
        else
        {
            UpTimeMS = 0;
            use = i;
        }
    }
    if (filmstripInBuffer)
    {
        Image = pApp->filmstripBuffer[use].Image;
        pApp->filmstripBuffer[use].UpTimeMS = GETUPTIMEMS();
    }
    else
    {
        ASSERT(use >= IMAGE_CACHE_SIZE, "use < IMAGE_CACHE_SIZE"); //Line 367
        if (pApp->filmstripBuffer[use].Image)
        {
            IIMAGE_Release(pApp->filmstripBuffer[use].Image);
            pApp->filmstripBuffer[use].Image = 0;
        }
        IImage* UIImages = kphelpers::LoadUIImages(pApp, pFilmstrip);
        Image = UIImages;
        if (UIImages)
        {
            pApp->filmstripBuffer[use].Image = UIImages;
            pApp->filmstripBuffer[use].UpTimeMS = GETUPTIMEMS();
            kphelpers::NullTerminatedString((char*)&pApp->filmstripBuffer[use], pFilmstrip, 32);
        }
    }
    if (Image)
        IIMAGE_AddRef(Image);
    return Image;
}

char* kphelpers::CreateStringTranslationPath(kphandset* pApp, char* pFilename, const char* pExtension)
{
    char* scratch = pApp->scratch;
    if (pFilename == pApp->scratch)
        scratch = pApp->kphandset_unk154;
    SNPRINTF(scratch, 400u, "fs:/card0/%c%c/%c%c/%s%s", pApp->episode[0], pApp->episode[1], pApp->lang[0], pApp->lang[1], pFilename, pExtension);
    return scratch;
}

char* kphelpers::ReadScriptFromMemory(kphandset* pApp, char* pSrc)
{
    char* pDest = pApp->kphandset_unk154;
    if (pSrc == pApp->kphandset_unk154)
        pDest = pApp->scratch;
    STRCPY(pDest, pSrc);
    return pDest;
}

char* kphelpers::ReadToScratch(kphandset* pApp, char* pName, char* pExtension)
{
    char* file = pName;
    ASSERT(pName == pApp->scratch, "pName != pApp->scratch");
    ASSERT(pExtension == pApp->scratch, "pExtension != pApp->scratch");
    pApp->scratch[0] = 0;
    bool fileExists = 0;
    if (STRBEGINS("root:", file))
    {
        file += 5;
    }
    else
    {
        if (pApp->episode[0] && pApp->lang[0])
        {
            fileExists = IFILEMGR_Test(pApp->pFileMgr, kphelpers::CreateStringTranslationPath(pApp, file, pExtension)) == 0;
        }
        if (!fileExists && pApp->episode[0])
        {
            fileExists = IFILEMGR_Test(pApp->pFileMgr, kphelpers::ReadFromEpisodePath(pApp, file, pExtension)) == 0;
        }
    }
    if (!fileExists && pApp->lang[0])
    {
        fileExists = IFILEMGR_Test(pApp->pFileMgr, kphelpers::ReadFromLangPath(pApp, file, pExtension)) == 0;
    }
    if (!fileExists)
    {
        fileExists = IFILEMGR_Test(pApp->pFileMgr, kphelpers::ReadFromRootSD(pApp, file, pExtension)) == 0;
    }
    if (!fileExists)
        pApp->scratch[0] = 0;
    return pApp->scratch;
}

char* kphelpers::ReadFromEpisodePath(kphandset* pApp, const char* pFilename, const char* pExtension)
{
    char* scratch = pApp->scratch;
    if (pFilename == pApp->scratch)
        scratch = pApp->kphandset_unk154;
    SNPRINTF(scratch, 400u, "fs:/card0/%c%c/%s%s", pApp->episode[0], pApp->episode[1], pFilename, pExtension);
    return scratch;
}

char* kphelpers::ReadFromLangPath(kphandset* pApp, const char* pFilename, const char* pExtension)
{
    char* scratch = pApp->scratch;
    if (pFilename == pApp->scratch)
        scratch = pApp->kphandset_unk154;
    SNPRINTF(scratch, 400u, "fs:/card0/%c%c/%s%s", pApp->lang[0], pApp->lang[1], pFilename, pExtension);
    return scratch;
}

char* kphelpers::ReadFromRootSD(kphandset* pApp, const char* pFilename, const char* pExtension)
{
    char* scratch = pApp->scratch;
    if (pFilename == pApp->scratch)
        scratch = pApp->kphandset_unk154;
    SNPRINTF(scratch, 400u, "fs:/card0/%s%s", pFilename, pExtension);
    return scratch;
}

char* kphelpers::ReadFromAppPath(kphandset* pApp, char* file, const char* fallbackData)
{
    char* scratch = pApp->scratch;
    unsigned int scratchSize = 400;
    if (file == pApp->scratch)
        scratch = pApp->kphandset_unk154;
    char* token = kphelpers::LoadStringFromFile(pApp, (const char*)file, scratch, &scratchSize);
    if (token)
    {
        while (kphandset::kp_CheckToken((unsigned char)*token))
            ++token;
        scratch = token;
        while (*token && !kphandset::kp_CheckToken((unsigned char)*token))
            ++token;
        *token = 0;
    }
    else if (fallbackData)
    {
        kphelpers::NullTerminatedString(scratch, fallbackData, 400);
    }
    else
    {
        *scratch = 0;
    }
    return scratch;
}

char* kphelpers::BuildPath(kphandset* pApp, char* pFilename, unsigned int filenameLength)
{
    char* filenamePtr = pFilename;
    char* scratch = pApp->scratch;
    if (pFilename == pApp->scratch)
        scratch = pApp->kphandset_unk154;
    STRCPY(scratch, "fs:/card0/");
    char* scratchBuf = &scratch[STRLEN(scratch)];
    for (unsigned int i = 0; i < filenameLength; ++i)
    {
        char tempChar = *filenamePtr++;
        *scratchBuf++ = tempChar;
    }
    *scratchBuf = 0;
    return scratch;
}

void kphelpers::UpdateFile(kphandset* result, char* pszFile, int pBuffer, int dwCount)
{
    if (pBuffer && pszFile && *pszFile)
    {
        /*IFILEMGR_Remove(result->pFileMgr, pszFile);
        IFile* file = IFILEMGR_OpenFile(result->pFileMgr, pszFile, 4);
        if (file)
        {
            IFILE_Write(file, (const void*)pBuffer, dwCount);
            IFILE_Release(file);
        }*/
    }
}


char* kphelpers::LoadStringFromFile(kphandset* pApp, const char* fileName, char* suppliedBuffer, unsigned int* outSize)
{
    unsigned int maxBuffer;
    char* buffer = 0;
    int maxSize = 0;
    if (fileName && *fileName)
    {
        IFile* file = IFILEMGR_OpenFile(pApp->pFileMgr, fileName, _OFM_READ);
        if (file)
        {
            FileInfo fileInfo;
            if (IFILE_GetInfo(file, &fileInfo))
            {
                kpdebug::Print((char*)"LOAD: File open failed");
            }
            else
            {
                maxBuffer = fileInfo.dwSize;//Maybe?
                if (suppliedBuffer)
                {
                    unsigned int bufferSize = 0;
                    if (outSize)
                        bufferSize = *outSize;
                    if (maxBuffer >= bufferSize)
                        kpdebug::Print((char*)"LOAD: SUPPLIED BUFFER IS TOO SMALL!");
                    else
                        buffer = suppliedBuffer;
                }
                else
                {
                    buffer = (char*)MALLOC(maxBuffer + 1);
                    if (!buffer)
                        kpdebug::Print((char*)"LOAD: MEMORY ALLOCATION FAILED!");
                }
                if (buffer)
                {
                    int v12 = IFILE_Read(file, buffer, maxBuffer + 1);
                    if (v12)
                    {
                        maxSize = maxBuffer;
                        buffer[v12] = 0;
                    }
                    else
                    {
                        if (!suppliedBuffer)
                            FREE(buffer);
                        buffer = 0;
                        kpdebug::Print((char*)"LOAD: Read Failure");
                    }
                }
            }
            IFILE_Release(file);
        }
        if (!buffer)
            IFILEMGR_GetLastError(pApp->pFileMgr);
    }
    if (outSize)
        *outSize = maxSize;
    return buffer;
}

char* kphelpers::ReadScriptBuf(const char* buf)
{
    const char* scriptBuf = buf;
    if (buf && *buf == 36)
    {
        kphandset* instance = (kphandset*)GETAPPINSTANCE();
        if (STRCMP(scriptBuf, "$EXTRACTION") == 0)
        {
            return (char*)&instance->script.kpscript_unk20;
        }
        else if (STRCMP(scriptBuf, "$MISSION") == 0)
        {
            return (char*)&instance->script;
        }
        else if (instance->script.constants)
        {
            char* v3 = MEMSTR(instance->script.constants, scriptBuf, instance->script.constantsLength);
            if (v3)
            {
                char* i;
                for (i = v3; *i && !kphelpers::CheckToken((unsigned char)*i); ++i)
                    ;
                while (*i && kphelpers::CheckToken((unsigned char)*i))
                    ++i;
                return i;
            }
        }
    }
    return (char*)scriptBuf;
}

char* kphelpers::ParseTranslation(kphandset* pApp, const char* translation)
{
    char* i = 0;
    if (translation && *translation && pApp->pTranslatorMgr.buffer)
    {
        for (i = MEMSTR(pApp->pTranslatorMgr.buffer, translation, pApp->pTranslatorMgr.length); i && *i && pApp->pTranslatorMgr.buffer != i && *(i - 1); i = MEMSTR(i + 1, translation, pApp->pTranslatorMgr.length - (i - pApp->pTranslatorMgr.buffer)))
        {
            ;
        }
        if (i)
        {
            char* j;
            for (j = i; *j && !kphelpers::CheckToken((unsigned char)*j); ++j)
                ;
            while (*j && kphelpers::CheckToken((unsigned char)*j))
                ++j;
            i = j;
        }
    }
    if (i)
        return i;
    else
        return (char*)translation;
}

void kphelpers::ParseTokenizer(char* tokenizerBuf, const char* tokenizer)
{
    while (*tokenizer && kphelpers::CheckToken(*(unsigned char*)tokenizer))
        ++tokenizer;
    size_t tokenLength = (unsigned short)&STRCHREND(tokenizer, '\n')[-(int)tokenizer];
    if (tokenLength >= 192)
    {
        tokenLength = 191;
        kpdebug::DBGPrintF(GETAPPINSTANCE(), (char*)"TOKENIZER: OVER LENGTH");
        const char* tokenBuf;
        if (tokenizer)
            tokenBuf = tokenizer;
        else
            tokenBuf = "<null>";
        kpdebug::nullfunc2((kphandset*)GETAPPINSTANCE(), "TOKENIZER: %s", tokenBuf, 0);
    }
    STRNCPY(tokenizerBuf, tokenizer, tokenLength);
    tokenizerBuf[tokenLength - 1] = 0;
    *((char**)tokenizerBuf + 48) = tokenizerBuf;
}

bool kphelpers::CheckToken(int token)
{
    return token == 32 || token == 10 || token == 13 || token == 9;
}

char* kphelpers::FetchMainToken(char* token)
{
    char* tokenPtr = token;
    if (token)
    {
        while (*tokenPtr && *tokenPtr != 13 && *tokenPtr != 10)
            ++tokenPtr;
        while (*tokenPtr && kphelpers::CheckToken((unsigned char)*tokenPtr))
            ++tokenPtr;
    }
    return tokenPtr;
}

char* kphelpers::FetchSubToken(char* token)
{
    char* tokenPtr = token;
    if (token)
    {
        while (*tokenPtr && kphelpers::CheckToken((unsigned char)*tokenPtr))
            ++tokenPtr;
    }
    return tokenPtr;
}

bool kphelpers::FetchGoToToken(kphandset* pApp, const char* script)
{
    bool returnData = kphandset::kpscr_Command_Goto(pApp, script);
    if (returnData)
    {
        char* mainToken = kphelpers::FetchMainToken(pApp->script.tokenizer);
        pApp->script.tokenizer = kphelpers::FetchSubToken(mainToken);
    }
    return returnData;
}