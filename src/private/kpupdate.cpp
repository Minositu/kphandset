#include "kpupdate.h"
#include "kpdebug.h"
#include "kptimeout.h"
#include "kphelpers.h"

int kpupdate::ParseBufferInt(int a1)
{
    return (*(unsigned char*)(a1 + 3) << 24) | (*(unsigned char*)(a1 + 2) << 16) | (*(unsigned char*)(a1 + 1) << 8) | *(unsigned char*)a1;
}

int kpupdate::ParseKPPFile(kpupdate* pScreen)
{
    int v2 = 0;
    if (pScreen->updatebuffer.bufferSize >= 20u)
    {
        int version = kpupdate::ParseBufferInt(*(int*)pScreen->updatebuffer.buffer + 4);
        pScreen->updatebuffer.fileCount = kpupdate::ParseBufferInt(*(int*)pScreen->updatebuffer.buffer + 16);
        if (**(uint8**)pScreen->updatebuffer.buffer == 75 && *(uint8*)(*(int*)pScreen->updatebuffer.buffer + 1) == 80 && *(uint8*)(*(int*)pScreen->updatebuffer.buffer + 2) == 80 && *(uint8*)(*(int*)pScreen->updatebuffer.buffer + 3) == 32)
        {
            if (version == 10001)
            {
                *(char*)pScreen->updatebuffer.buffer += 20;
                pScreen->updatebuffer.bufferSize -= 20;
                pScreen->ParseKPPFileCB = kpupdate::ParseKPPGetFileSize;
                return 1;
            }
            else
            {
                kpdebug::Print((char*)"UPD: BAD KPP VERSION");
                pScreen->errorDetected = 1;
            }
        }
        else
        {
            kpdebug::Print((char*)"UPD: BAD HEADER");
            pScreen->errorDetected = 1;
        }
    }
    return v2;
}

int kpupdate::ParseKPPGetFileSize(kpupdate* pScreen)
{
    int v2 = 0;
    if (pScreen->updatebuffer.bufferSize >= 4u)
    {
        pScreen->updatebuffer.selectedFileSize = kpupdate::ParseBufferInt(*(int*)pScreen->updatebuffer.buffer);
        *(char*)pScreen->updatebuffer.buffer += 4;
        pScreen->updatebuffer.bufferSize -= 4;
        pScreen->ParseKPPFileCB = kpupdate::ParseKPPFileHandler;
        return 1;
    }
    return v2;
}

int kpupdate::ParseKPPFileHandler(kpupdate* pScreen)
{
    int v2 = 0;
    unsigned int fileNameLength = **(unsigned char**)pScreen->updatebuffer.buffer;
    if (pScreen->updatebuffer.bufferSize > fileNameLength)
    {
        kphandset* v4 = (kphandset*)GETAPPINSTANCE();
        ++*(char*)pScreen->updatebuffer.buffer;
        --pScreen->updatebuffer.bufferSize;
        char* v5 = *(char**)pScreen->updatebuffer.buffer;
        *(char**)pScreen->updatebuffer.buffer = &v5[fileNameLength];
        pScreen->updatebuffer.bufferSize -= fileNameLength;
        if (*v5 && *v5 == 92)
        {
            ++v5;
            --fileNameLength;
        }
        while (*v5 && *v5 != 92)
        {
            ++v5;
            --fileNameLength;
        }
        if (*v5 && *v5 == 92)
        {
            ++v5;
            --fileNameLength;
        }
        char* scratch;
        if (STRBEGINS("_root\\", v5))
        {
            kphelpers::NullTerminatedString(v4->scratch, (const char*)(v5 + 6), fileNameLength - 6);
            v4->kphandset_unk150[fileNameLength + 3] = 0;
            scratch = v4->scratch;
        }
        else
        {
            scratch = kphelpers::BuildPath(v4, (char*)v5, fileNameLength);
        }
        for (char* i = scratch; *i; ++i)
        {
            if (*i == 92)
                *i = 47;
        }
        //if (IFILEMGR_Remove(v4->pFileMgr, scratch))
        //    kpdebug::Assert("ERROR deleting %s", scratch, 0);
        if ((int)pScreen->updatebuffer.selectedFileSize < 0)
        {
            kpdebug::Assert("UPD: -%s", scratch, 0);
            pScreen->ParseKPPFileCB = kpupdate::ParseKPPGetFileSize;
        }
        else
        {
            kpdebug::Assert("UPD: +%s", scratch, 0);
            pScreen->updatebuffer.updateFile = IFILEMGR_OpenFile(v4->pFileMgr, scratch, 4);
            pScreen->updatebuffer.currentSelectedFileSize = 0;
            pScreen->ParseKPPFileCB = kpupdate::ParseKPPWriteFile;
            if (!pScreen->updatebuffer.updateFile)
            {
                int error = IFILEMGR_GetLastError(v4->pFileMgr);
                kpdebug::AssertLine("No file - error %d", error);
                if (error == EBADFILENAME)
                {
                    kpdebug::Print((char*)"Bad file name~");
                }
                else if (error > EBADFILENAME)
                {
                    switch (error)
                    {
                        case EBADSEEKPOS:
                            kpdebug::Print((char*)"Bad seek position~");
                        break;
                        case EFILEEOF:
                            kpdebug::Print((char*)"End of file~");
                        break;
                        case EFSFULL:
                            kpdebug::Print((char*)"File system full~");
                        break;
                        case EFILEOPEN:
                            kpdebug::Print((char*)"File already open~");
                        break;
                    }
                }
                else
                {
                    switch (error)
                    {
                        case EBADPARM:
                            kpdebug::Print((char*)"Invalid parameter~");
                        break;
                        case EFILEEXISTS:
                            kpdebug::Print((char*)"File exists~");
                        break;
                        case EFILENOEXISTS:
                            kpdebug::Print((char*)"File does not exist~");
                        break;
                        case EDIRNOTEMPTY:
                            kpdebug::Print((char*)"Directory not empty~");
                        break;
                    }
                }
            }
        }
        return 1;
    }
    return v2;
}

int kpupdate::ParseKPPWriteFile(kpupdate* pScreen)
{
    uint32 bufferSize;

    bufferSize = (char*)pScreen->updatebuffer.selectedFileSize - (char*)pScreen->updatebuffer.currentSelectedFileSize;
    if (pScreen->updatebuffer.bufferSize < bufferSize)
        bufferSize = pScreen->updatebuffer.bufferSize;
    //if (pScreen->updatebuffer.updateFile)
    //    IFILE_Write(pScreen->updatebuffer.updateFile,*(const void**)pScreen->updatebuffer.buffer,bufferSize);
    pScreen->updatebuffer.currentSelectedFileSize += bufferSize;
    *(uint32*)pScreen->updatebuffer.buffer += bufferSize;
    pScreen->updatebuffer.bufferSize -= bufferSize;
    if (pScreen->updatebuffer.currentSelectedFileSize == pScreen->updatebuffer.selectedFileSize)
    {
        if (pScreen->updatebuffer.updateFile)
        {
            IFILE_Release(pScreen->updatebuffer.updateFile);
            pScreen->updatebuffer.updateFile = 0;
        }
        pScreen->ParseKPPFileCB = kpupdate::ParseKPPGetFileSize;
        ++pScreen->updatebuffer.currentFileIndex;
    }
    return pScreen->updatebuffer.bufferSize != 0;
}

void kpupdate::WebResponseCB(void* pData)
{
    kpupdate* pUpdate = (kpupdate*)pData;
    WebRespInfo* updateInfo = IWEBRESP_GetInfo(pUpdate->pWebResp);
    if (updateInfo)
    {
        uint8* i;
        pUpdate->errorDetected = 0;
        if (updateInfo->nCode < 200 || updateInfo->nCode >= 300)
        {
            kpdebug::Print((char*)"UPD: HTTP FAILURE");
            pUpdate->errorDetected = 1;
        }
        if (!pUpdate->errorDetected)
        {
            ISource* pisMessage = updateInfo->pisMessage;
            int v14 = ISOURCE_Read(pisMessage, (char*)&pUpdate->updatebuffer.buffer[pUpdate->updatebuffer.bufferSize + 4], 512 - pUpdate->updatebuffer.bufferSize);
            if (v14 != -2)
            {
                if (v14 == -1)
                {
                    kpdebug::Print((char*)"UPD: HTTP READ ERROR");
                    pUpdate->errorDetected = 1;
                }
                if (!pUpdate->errorDetected)
                {
                    if (!v14)
                    {
                        if (!pUpdate->errorDetected && pUpdate->updatebuffer.fileCount == pUpdate->updatebuffer.currentFileIndex)
                        {
                            char* v4 = STRRCHR(pUpdate->pszURL, 47);
                            unsigned int v5 = (unsigned int)v4;
                            if (v4 && *v4)
                                v5 = (unsigned int)(v4 + 1);
                            for (i = (uint8*)v5; *i && *i != 95; ++i);
                            kphandset* v7 = (kphandset*)GETAPPINSTANCE();
                            if ((unsigned int)i > v5)
                            {
                                char* v13 = kphelpers::ReadFromRootSD(v7, "_version", ".txt");
                                kphelpers::UpdateFile(v7, v13, v5, (int)&i[-v5]);
                            }
                            kpupdate::Shutdown(pUpdate);
                            ISHELL_PostEvent(v7->m_pIShell, AEECLSID_KPHANDSET, 28713, 0, 0);
                        }
                    }
                    if (!pUpdate->errorDetected)
                    {
                        *(char**)pUpdate->updatebuffer.buffer = (char*)&pUpdate->updatebuffer.buffer[4];
                        pUpdate->updatebuffer.bufferSize += v14;
                        char* v9 = (char*)pUpdate->updatebuffer.buffer + pUpdate->updatebuffer.bufferSize;
                        for (int j = 1; j && !pUpdate->errorDetected && (char*)pUpdate->updatebuffer.buffer < v9 && pUpdate->ParseKPPFileCB; j = ((int(*)(kpupdate*))pUpdate->ParseKPPFileCB)(pUpdate))
                        {
                            ;
                        }
                        if (pUpdate->updatebuffer.bufferSize)
                            MEMMOVE(&pUpdate->updatebuffer.buffer[4], pUpdate->updatebuffer.buffer, pUpdate->updatebuffer.bufferSize);
                        kpscreen::RefreshDisplay(((kphandset*)GETAPPINSTANCE()));
                    }
                }
            }
            if (!pUpdate->errorDetected)
            {
                ISOURCE_Readable(pisMessage, &pUpdate->pResponseCallback);
            }
        }
        if (pUpdate->errorDetected)
        {
            kpdebug::Print((char*)"UPD: ERROR, REBOOTING");
            kpupdate::Shutdown(pUpdate);
            ISHELL_PostEvent(((kphandset*)GETAPPINSTANCE())->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_UPDATE_ERROR, 0, 0);
        }
    }
}

void kpupdate::Shutdown(kpupdate* pUpdate)
{
    CALLBACK_Cancel(&pUpdate->pResponseCallback);
    if (pUpdate->pIWeb)
    {
        IWEB_Release(pUpdate->pIWeb);
        pUpdate->pIWeb = 0;
    }
    if (pUpdate->pWebResp)
    {
        IWEBRESP_Release(pUpdate->pWebResp);
        pUpdate->pWebResp = 0;
    }
    if (pUpdate->updatebuffer.updateFile)
    {
        IFILE_Release(pUpdate->updatebuffer.updateFile);
        pUpdate->updatebuffer.updateFile = 0;
    }
}

void kpupdate::UpdateResponse_PFNWEBSTATUS(void* pNotifyData, WebStatus ws, void* pData)
{

}

kpupdate* kpupdate::InitializeUpdateScreen(kphandset* a1, const char* url)
{
    kpupdate* screen = (kpupdate*)kphandset::FetchScreen(a1);
    if (screen)
    {
        kpscreen::ExecuteCommand(screen);
        screen->InitPtr = (void (*)(kpscreen*, int))kpupdate::Init;
        screen->DrawPtr = (void (*)(kpscreen*))kpupdate::Draw;
        screen->HandleEventPtr = (bool (*)(kpscreen*, AEEEvent, uint16, uint32))kpupdate::HandleEvent;
        screen->ReleasePtr = (void (*)(kpscreen*))kpupdate::Release;
        kphelpers::NullTerminatedString(screen->pszURL, url, 128);
        screen->ui_waiting = kphelpers::LoadUIImages(a1, (char*)"ui_waiting");
        screen->recievedHttpResponse = 0;
        screen->errorDetected = 0;
        screen->ParseKPPFileCB = kpupdate::ParseKPPFile;
        CALLBACK_Init(&screen->pResponseCallback, kpupdate::WebResponseCB, screen);
        AEERect rect;
        if (!ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_STATIC, (void**)&screen->pUpdatingText))
        {
            rect.x = 0;
            rect.y = a1->pBitmapInfo.cy - 80;
            rect.dx = a1->pBitmapInfo.cx;
            rect.dy = 50;
            ISTATIC_SetRect(screen->pUpdatingText, &rect);
            ISTATIC_SetFont(screen->pUpdatingText, AEE_FONT_LARGE, AEE_FONT_LARGE);
            ISTATIC_SetProperties(screen->pUpdatingText, ST_ASCII | ST_CENTERTEXT);
            ISTATIC_SetText(screen->pUpdatingText, 0, (AECHAR*)"UPDATING\nDO NOT TURN OFF", AEE_FONT_LARGE, AEE_FONT_LARGE);
        }
        int v6 = ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_WEB, (void**)&screen->pIWeb);
        if (v6)
        {
            kpdebug::AssertLine("SYS: Error creating IWeb (0x%x)", v6);
            ISHELL_PostEvent(a1->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_UPDATE_ERROR, 0, 0);
        }
    }
    return (kpupdate*)screen;
}

bool kpupdate::HandleEvent(kpupdate* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam)
{
    return kpscreen::HandleEvent(pScreen, eCode, wParam, dwParam);
}

void kpupdate::Draw(kpupdate* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    AEERect rect;
    rect.y = 0;
    //IDISPLAY_DrawRect(instance->m_pIDisplay, 0, 0xFFFFFFFF, 0xFFFFFFFF, IDF_RECT_FILL);
    IDISPLAY_DrawRect(instance->m_pIDisplay, 0, RGBA_WHITE, RGBA_WHITE, IDF_RECT_FILL);
    if (pScreen->pUpdatingText)
    {
        ISTATIC_GetRect(pScreen->pUpdatingText, &rect);
        ISTATIC_Redraw(pScreen->pUpdatingText);
    }
    if (pScreen->ui_waiting)
        IIMAGE_Draw(pScreen->ui_waiting, 0, 0);
    rect.x = 0;
    rect.y = 0;
    rect.dx = instance->pBitmapInfo.cx;
    rect.dy = instance->pBitmapInfo.cy;
    rect.x = 5;
    rect.dx -= 10;
    rect.y = instance->pBitmapInfo.cy - 30;
    rect.dy = 10;
    //IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, 0xFF, IDF_RECT_FILL);
    IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, 0xFF, IDF_RECT_FILL);
    ++rect.x;
    rect.dx -= 2;
    ++rect.y;
    rect.dy -= 2;
    if (pScreen->updatebuffer.fileCount)
    {
        rect.dx = (rect.dx * pScreen->updatebuffer.currentFileIndex) / pScreen->updatebuffer.fileCount;
        //IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, 0x80E080FF, IDF_RECT_FILL);
        IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, MAKE_RGB(128, 224, 128), IDF_RECT_FILL);
    }
    rect.y += rect.dy + 5;
    rect.dx = instance->pBitmapInfo.cx - 10;
    rect.dy = 10;
    //IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, 0xFF, IDF_RECT_FILL);
    IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, 0xFF, IDF_RECT_FILL);
    ++rect.x;
    rect.dx -= 2;
    ++rect.y;
    rect.dy -= 2;
    if (pScreen->updatebuffer.selectedFileSize)
    {
        rect.dx = (rect.dx * pScreen->updatebuffer.currentSelectedFileSize) / pScreen->updatebuffer.selectedFileSize;
        //IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, 0x80E080FF, IDF_RECT_FILL);
        IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, MAKE_RGB(128, 224, 128), IDF_RECT_FILL);
    }
    kpscreen::Draw(pScreen);
}

void kpupdate::Release(kpupdate* pScreen)
{
    kpupdate::Shutdown(pScreen);
    if (pScreen->ui_waiting)
    {
        IIMAGE_Release(pScreen->ui_waiting);
        pScreen->ui_waiting = 0;
    }
    if (pScreen->pUpdatingText)
    {
        ISTATIC_Release(pScreen->pUpdatingText);
        pScreen->pUpdatingText = 0;
    }
    kpscreen::Release(pScreen);
}

void kpupdate::Init(kpupdate* pScreen, int initialize)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (initialize)
    {
        if (instance->adminMode)
        {
            kpdebug::Print((char*)"SYS: Exiting admin mode");
            instance->adminMode = 0;
        }
        kpnetwork::StartSocketPingTimer(instance, 0);
        instance->network.keepalive = instance->network.default_keepalive;
        kpnetwork::CancelConnection(instance);
        if (!pScreen->recievedHttpResponse && pScreen->pIWeb && instance->pFileMgr)
        {
            kpnetwork::StartSocketPingTimer(instance, 0);
            pScreen->recievedHttpResponse = 1;
            kpdebug::Assert("SYS: Updating from %s", pScreen->pszURL, 0);
            IWEB_GetResponse(pScreen->pIWeb, (pScreen->pIWeb, &pScreen->pWebResp, &pScreen->pResponseCallback, pScreen->pszURL, WEBOPT_HANDLERDATA, pScreen, WEBOPT_STATUSHANDLER, kpupdate::UpdateResponse_PFNWEBSTATUS, WEBOPT_END));
        }
    }
    if(initialize == 1)
        kptimeout::ClearTimeout(instance);
}