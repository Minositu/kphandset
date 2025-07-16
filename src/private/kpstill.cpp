#include "kpstill.h"
#include "kphandset.h"
#include "kphelpers.h"

kpscreen* kpstill::ExecuteCommand(kphandset* pApp, char* commandBuffer, char* uiImage)
{
    //*(DWORD*)&imageInfo.cx = commandBuffer;
    //*(DWORD*)&imageInfo.nColors = uiImage;
    kpstill* pScreen = (kpstill*)kphandset::FetchScreen(pApp);
    if (pScreen)
    {
        kpscreen::ExecuteCommand(pScreen);
        pScreen->InitPtr = (void (*)(kpscreen*, int))kpstill::Init;
        pScreen->DrawPtr = (void (*)(kpscreen*))kpstill::Draw;
        pScreen->HandleEventPtr = (bool (*)(kpscreen*, AEEEvent, uint16, uint32))kpstill::HandleEvent;
        pScreen->ReleasePtr = (void (*)(kpscreen*))kpstill::Release;
        pScreen->advance_enabled = 1;
        pScreen->replay_enabled = 1;
        pScreen->audio_advance = 0;
        pScreen->looping = 0;
        char* text = 0;
        char* caption = 0;
        while (commandBuffer && **((uint8**)commandBuffer + 48))
        {
            const char* Command = kphandset::kpscr_GetCommand(commandBuffer);
            if (!STRBEGINS("png:", Command) || pScreen->still_Image)
            {
                if (STRBEGINS("text:", Command))
                {
                    char* ScriptBuf = kphelpers::ReadScriptBuf(Command + 5);
                    text = kphelpers::ParseTranslation(pApp, ScriptBuf);
                }
                else if (STRBEGINS("caption:", Command))
                {
                    char* v13 = kphelpers::ReadScriptBuf(Command + 8);
                    caption = kphelpers::ParseTranslation(pApp, v13);
                }
                else if (STRBEGINS("delay:", Command))
                {
                    char* v14 = kphelpers::ReadScriptBuf(Command + 6);
                    pScreen->serverTick = ATOI(v14);
                }
                else if (STRBEGINS("advance:", Command) && STRBEGINS("no", kphelpers::ReadScriptBuf(Command + 8)))
                {
                    pScreen->advance_enabled = 0;
                }
                else if (STRBEGINS("replay:", Command) && STRBEGINS("no", kphelpers::ReadScriptBuf(Command + 7)))
                {
                    pScreen->replay_enabled = 0;
                }
                else if (STRBEGINS("audio:", Command))
                {
                    char* v17 = kphelpers::ReadScriptBuf(Command + 6);
                    kphelpers::NullTerminatedString(pScreen->still_audioFile, v17, 32);
                }
                else if (STRBEGINS("audio-advance:", Command) && STRBEGINS("yes", kphelpers::ReadScriptBuf(Command + 14)))
                {
                    pScreen->audio_advance = 1;
                    pScreen->looping = 0;
                }
                else if (STRBEGINS("loop:", Command) && STRBEGINS("yes", kphelpers::ReadScriptBuf(Command + 5)))
                {
                    pScreen->audio_advance = 0;
                    pScreen->looping = 1;
                }
                else if (STRBEGINS("help:", Command))
                {
                    char* v20 = kphelpers::ReadScriptBuf(Command + 5);
                    kphelpers::NullTerminatedString(pScreen->help_buffer, v20, 20);
                }
                else if (STRBEGINS("retrigger:", Command))
                {
                    char* v21 = kphelpers::ReadScriptBuf(Command + 10);
                    kphelpers::NullTerminatedString(pScreen->retrigger_buffer, v21, 20);
                }
                else if (STRBEGINS("ping:", Command))
                {
                    char* v22 = kphelpers::ReadScriptBuf(Command + 5);
                    pScreen->still_pingTime = ATOI(v22);
                }
                else if (STRBEGINS("keepalive:", Command))
                {
                    char* v23 = kphelpers::ReadScriptBuf(Command + 10);
                    pScreen->still_keepalive = ATOI(v23);
                }
                else if (STRBEGINS("ignore:", Command))
                {
                    char* v24 = kphelpers::ReadScriptBuf(Command + 7);
                    kphelpers::NullTerminatedString(pApp->kphandset_unk98_1, v24, 32);
                }
            }
            else
            {
                char* v11 = kphelpers::ReadScriptBuf(Command + 4);
                pScreen->still_Image = kphelpers::LoadUIImages(pApp, v11);
            }
        }
        if (!pScreen->still_Image)
            pScreen->still_Image = kphelpers::LoadUIImages(pApp, uiImage);
        if (pScreen->still_Image)
        {
            pScreen->still_frameCount = 1;
            AEEImageInfo imageInfo;
            IIMAGE_GetInfo(pScreen->still_Image, &imageInfo);
            if (imageInfo.cx > pApp->pBitmapInfo.cx)
            {
                pScreen->still_frameCount = (imageInfo.cx / pApp->pBitmapInfo.cx);
                IIMAGE_SetParm(pScreen->still_Image, 2, pApp->pBitmapInfo.cx, 0);
            }
        }
        if (text && *text)
        {
            /*imageInfo.nColors = 3;
            *(WORD*)&imageInfo.bAnimated = 3;
            imageInfo.cxFrame = LOWORD(pApp->pBitmapInfo.cx) - 6;
            v27 = LOWORD(pApp->pBitmapInfo.cy) - 18;
            pScreen->kpscreen_still_unk3 = pApp->kphandset_unk156;
            kpscr_sub_30C80((int)pScreen->kpscreen_still_unk3, &imageInfo.nColors, v8);*/
        }
        if (caption && *caption)
        {
            pScreen->bottomScreen = &pApp->global_bottom;
            kphandset::kpscr_InitBottomBar(pScreen->bottomScreen, caption, 0);
        }
    }
    return pScreen;
}

void kpstill::RefreshDisplay(kpstill* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    kpscreen::RefreshDisplay(instance);
    ISHELL_SetTimer(instance->m_pIShell, 100, (PFNNOTIFY)kpstill::RefreshDisplay, pScreen);
}

void kpstill::func_2DB80(kpstill* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    kphandset::kpscr_func_1DDAC(pScreen);
}

bool kpstill::HandleEvent(kpstill* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam)
{
    int handled = 0;
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (eCode == EVT_KEY && wParam == AVK_SELECT)
    {
        if (pScreen->advance_enabled)
            kpstill::func_2DB80(pScreen);
        handled = 1;
    }
    else if (eCode == EVT_KEY && wParam == AVK_SOFT1)
    {
        if (!pScreen->retrigger_buffer[0] && pScreen->replay_enabled)
        {
            kphandset::kpscr_func_23C18(instance);
            handled = 1;
        }
    }
    else if (eCode == EVT_KPHANDSET_APP_AUDIO_INIT)
    {
        if (pScreen->audio_advance)
        {
            kpstill::func_2DB80(pScreen);
        }
        else if (pScreen->looping && pScreen->still_audioFile[0])
        {
            kpaudio::LoadAudioFile(pScreen->still_audioFile);
        }
        handled = 1;
    }
    return handled || kpscreen::HandleEvent(pScreen, eCode, wParam, dwParam);
}

void kpstill::Draw(kpstill* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pScreen->still_Image)
    {
        if (pScreen->still_frameCount <= 1u)
        {
            IIMAGE_Draw(pScreen->still_Image, 0, 0);
        }
        else
        {
            unsigned int v3 = GETTIMEMS();
            unsigned int v4 = (v3 / 100u);
            unsigned int v5 = (v4 % pScreen->still_frameCount);
            DBGPRINTF("%d, %d, %d", v3, v4, v5);
            IIMAGE_DrawFrame(pScreen->still_Image, v5, 0, 0);
        }
    }
    //if (pScreen->kpscreen_still_unk3)
    //    kpscr_sub_30F18((int)pScreen->kpscreen_still_unk3);
    if (pScreen->bottomScreen)
        kphandset::kpscr_RenderSubtitles(pScreen->bottomScreen);
    if (pScreen->advance_enabled && !pScreen->serverTick && (!pScreen->still_audioFile[0] || !pScreen->audio_advance) && instance->ui_ok_Interface)
    {
        IIMAGE_Draw(instance->ui_ok_Interface, 0, 0);
    }
    if (!pScreen->retrigger_buffer[0] && pScreen->replay_enabled && instance->ui_replay_Interface)
        IIMAGE_Draw(instance->ui_replay_Interface, 0, 0);
    kpscreen::Draw(pScreen);
}

void kpstill::Release(kpstill* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    ISHELL_CancelTimer(instance->m_pIShell, 0, pScreen);
    if (pScreen->still_Image)
    {
        IIMAGE_Release(pScreen->still_Image);
        pScreen->still_Image = 0;
    }
    kpscreen::Release(pScreen);
}

void kpstill::Init(kpstill* pScreen, int initialize)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pScreen->still_pingTime)
    {
        if (initialize)
            kpnetwork::StartSocketPingTimer(instance, pScreen->still_pingTime);
        else
            kpnetwork::StartSocketPingTimer(instance, instance->network.base_ping);
    }
    if (pScreen->still_keepalive)
    {
        if (initialize)
            instance->network.keepalive = pScreen->still_keepalive;
        else
            instance->network.keepalive = instance->network.default_keepalive;
    }
    if (pScreen->serverTick && initialize)
        ISHELL_SetTimer(instance->m_pIShell, pScreen->serverTick, (PFNNOTIFY)kpstill::func_2DB80, pScreen);
    if (initialize)
    {
        if (pScreen->still_frameCount > 1u)
            ISHELL_SetTimer(instance->m_pIShell, 100, (PFNNOTIFY)kpstill::RefreshDisplay, pScreen);
    }
    else
    {
        ISHELL_CancelTimer(instance->m_pIShell, 0, pScreen);
    }
    if (initialize && pScreen->still_audioFile[0])
        kpaudio::LoadAudioFile(pScreen->still_audioFile);
    if (initialize == 1)
        kphandset::kpscr_func_2AB8C(instance);
}