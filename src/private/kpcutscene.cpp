#include "kpcutscene.h"
#include "kpaudio.h"
#include "kpdebug.h"
#include "kphelpers.h"

kpscreen* kpcutscene::ExecuteCommand(kphandset* pApp, char* commandBuffer)
{
    kpcutscene* pScreen = (kpcutscene*)kphandset::FetchScreen(pApp);
    if (pScreen)
    {
        kpscreen::ExecuteCommand(pScreen);
        pScreen->InitPtr = (void (*)(kpscreen*, int))kpcutscene::Init;
        pScreen->DrawPtr = (void (*)(kpscreen*))kpcutscene::Draw;
        pScreen->HandleEventPtr = (bool (*)(kpscreen*, AEEEvent, uint16, uint32))kpcutscene::HandleEvent;
        pScreen->ReleasePtr = (void (*)(kpscreen*))kpcutscene::Release;
        char* ScriptBuf = kphelpers::ReadScriptBuf("$CUTSCENE_SKIP");
        pScreen->currentFilmstrip.skipCutscene = 1;
        if (ScriptBuf && *ScriptBuf && STRBEGINS("no", ScriptBuf))
            pScreen->currentFilmstrip.skipCutscene = 0;
        pScreen->cutsceneLoaded = 0;
        pScreen->scriptLoaded = 0;
        pScreen->currentFilmstrip.cutscenePlaying = 0;
        pScreen->currentFilmstrip.audioPlaying = 0;
        pScreen->currentFilmstrip.unk456456_3 = 0;
        unsigned int rand = 0;
        int allowReplay = 1;
        while (commandBuffer && **((uint8**)commandBuffer + 48))
        {
            const char* Command = kphandset::kpscr_GetCommand(commandBuffer);
            if (STRBEGINS("replay:", Command) && STRBEGINS("no", kphelpers::ReadScriptBuf(Command + 7)))
            {
                allowReplay = 0;
            }
            else if (STRBEGINS("skip:", Command))
            {
                char* skipBuf = kphelpers::ReadScriptBuf(Command + 5);
                if (STRBEGINS("no", skipBuf))
                {
                    pScreen->currentFilmstrip.skipCutscene = 0;
                }
                else
                {
                    char* skipYesBuf = kphelpers::ReadScriptBuf(Command + 5);
                    if (STRBEGINS("yes", skipYesBuf))
                        pScreen->currentFilmstrip.skipCutscene = 1;
                }
            }
            else if (STRBEGINS("rand:", Command))
            {
                char* randBuf = kphelpers::ReadScriptBuf(Command + 5);
                unsigned int randVal = ATOI(randBuf);
                if (randVal)
                {
                    GETRAND((byte*)&rand, 4);
                    int randDiv = (rand / randVal);
                    rand = randDiv;
                }
            }
            else
            {
                if (!rand || !pScreen->cutsceneFilePath[0])
                {
                    if (Command)
                    {
                        char* commandVal = kphelpers::ReadScriptBuf(Command);
                        STRNCPY(pScreen->cutsceneFilePath, commandVal, 32);
                        pScreen->cutsceneFilePath[31] = 0;
                        STRLOWER(pScreen->cutsceneFilePath);
                        if (!rand)
                            rand = 99999;
                    }
                }
                if (rand)
                    --rand;
            }
        }
        if (allowReplay)
            kpcutscene::SetReplayBuffer(pApp);
        if (pScreen->cutsceneFilePath[0])
        {
            kphandset::kpscr_InitBottomBar(&pApp->global_bottom, 0, "ui_bottom_clear");
        }
        else
        {
            kpscreen::Release(pScreen);
            return 0;
        }
    }
    return pScreen;
}

void kpcutscene::LoadCutscene(kpcutscene* pScreen)
{
    int pingSocket = 0;
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pScreen->cutsceneLoaded)
    {
        if (pScreen->scriptLoaded)
        {
            if (pScreen->currentFilmstrip.preloadedFilmstripIndex >= pScreen->currentFilmstrip.index)
            {
                if (!pScreen->currentFilmstrip.cutscenePlaying)
                {
                    pScreen->currentFilmstrip.cutscenePlaying = 1;
                    pScreen->currentFilmstrip.audioPlaying = 0;
                    kpaudio::LoadAudioFile(pScreen->cutsceneFilePath);
                    ISHELL_SetTimer(instance->m_pIShell, 10, (PFNNOTIFY)kpcutscene::UpdateFrame, pScreen);
                    pingSocket = 0;
                }
            }
            else
            {
                IImage* filmstrip = kphelpers::PreloadFilmstrip(instance, pScreen->filmstripNames[pScreen->currentFilmstrip.preloadedFilmstripIndex]);
                if (filmstrip)
                {
                    IIMAGE_SetParm(filmstrip, IPARM_CXFRAME, instance->pBitmapInfo.cx, 0);
                    pScreen->filmstripImages[pScreen->currentFilmstrip.preloadedFilmstripIndex] = filmstrip;
                }
                ++pScreen->currentFilmstrip.preloadedFilmstripIndex;
                pingSocket = 1;
            }
        }
        else
        {
            kpcutscene::ParseKPSFile(instance, pScreen);
            pScreen->scriptLoaded = 1;
            pingSocket = 1;
        }
    }
    else
    {
        char* kpScriptPath = kphelpers::ReadToScratch(instance, pScreen->cutsceneFilePath, (char*)".kps");
        char* cutsceneFile = 0;
        unsigned int outSize = 2048;
        if (kpScriptPath && *kpScriptPath)
            cutsceneFile = kphelpers::LoadStringFromFile(instance, kpScriptPath, pScreen->cutsceneData, &outSize);
        if (!cutsceneFile)
        {
            kpdebug::Print((char*)"CUT: UNABLE TO LOAD CUTSCENE");
            pScreen->filmstripImages[0] = kphelpers::PreloadFilmstrip(instance, (char*)"temp_cutscene");
            char* translatedKey = kphelpers::ParseTranslation(instance, pScreen->cutsceneFilePath);
            kphandset::kpscr_subtitles_func_305F4(&instance->global_bottom, translatedKey);
            pScreen->scriptLoaded = 1;
        }
        pScreen->cutsceneLoaded = 1;
        pingSocket = 1;
    }
    if (pingSocket)
        ISHELL_PostEvent(instance->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_SOCKET_PING, 0, (uint32)pScreen);
}

void kpcutscene::StopCutscene(kphandset* pApp, kpcutscene* pScreen)
{
    kpaudio::StopMedia(&pApp->pAudio);
    pScreen->currentFilmstrip.cutscenePlaying = 0;
    ISHELL_CancelTimer(pApp->m_pIShell, 0, pScreen);
}

void kpcutscene::ParseKPSFile(kphandset* pApp, kpcutscene* pScreen)
{
    pScreen->currentFilmstrip.preloadedFilmstripIndex = 0;
    pScreen->currentFilmstrip.index = pScreen->cutsceneData[4];
    char* layer = &pScreen->cutsceneData[5];
    for (int i = 0; pScreen->currentFilmstrip.index > i; i++)
    {
        if (i < 5)
            pScreen->filmstripNames[i] = layer;
        while (*layer)
            ++layer;
        ++layer;
    }
    if (pScreen->currentFilmstrip.index > 5u)
    {
        kpdebug::Print("CUT: EXCEEDED MAXIMUM LAYER COUNT");
        pScreen->currentFilmstrip.index = 5;
    }
    pScreen->currentFilmstrip.subtitleCount = *layer;
    uint8* subtitle = (uint8*)(layer + 1);
    pScreen->subtitleStartFrame = subtitle;
    unsigned int subtitleIndex = 0;
    pScreen->currentFilmstrip.kpfilmstrip_unk6 = 0;
    while (pScreen->currentFilmstrip.subtitleCount > subtitleIndex)
    {
        uint8* subtitleKey = subtitle + 4;
        while (*++subtitleKey);
        subtitle = subtitleKey + 1;
        ++subtitleIndex;
    }
    pScreen->currentFilmstrip.frameCount = *subtitle + (subtitle[1] << 8);
    pScreen->currentFilmstrip.frames = subtitle + 2;
}

void kpcutscene::PingSocket(kpcutscene* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    ISHELL_PostEvent(instance->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_SOCKET_PING, 0, (uint32)pScreen);
}

void kpcutscene::SetReplayBuffer(kphandset* pApp)
{
    pApp->script.replayBuffer = pApp->script.tokenizer;
}

void kpcutscene::UpdateFrame(kpcutscene* pScreen)
{
    if (pScreen->currentFilmstrip.cutscenePlaying)
    {
        kphandset* instance = (kphandset*)GETAPPINSTANCE();
        int filmstripIndex = pScreen->currentFilmstrip.filmstripIndex;
        unsigned int UpTimeMS = kpaudio::GetUpTimeMS(instance);
        if (pScreen->currentFilmstrip.audioPlaying)
            UpTimeMS = instance->pAudio.PausedUpTimeMS + GETUPTIMEMS() - instance->pAudio.UpTimeMS;
        pScreen->currentFilmstrip.filmstripIndex = UpTimeMS / 100u;
        if (!UpTimeMS && pScreen->currentFilmstrip.filmstripIndex < filmstripIndex)
            pScreen->currentFilmstrip.filmstripIndex = filmstripIndex;
        if (pScreen->currentFilmstrip.audioPlaying && instance->unk21_3)
            pScreen->currentFilmstrip.filmstripIndex = pScreen->currentFilmstrip.frameCount;
        if (!pScreen->currentFilmstrip.audioPlaying || pScreen->currentFilmstrip.frameCount && pScreen->currentFilmstrip.filmstripIndex < pScreen->currentFilmstrip.frameCount - 1)
        {
            if (pScreen->currentFilmstrip.filmstripIndex >= (int)pScreen->currentFilmstrip.frameCount)
                pScreen->currentFilmstrip.filmstripIndex = pScreen->currentFilmstrip.frameCount - 1;
            ISHELL_SetTimer(instance->m_pIShell, 10, (PFNNOTIFY)kpcutscene::UpdateFrame, pScreen);
        }
        else
        {
            if (pScreen->currentFilmstrip.frameCount)
                pScreen->currentFilmstrip.filmstripIndex = pScreen->currentFilmstrip.frameCount - 1;
            pScreen->currentFilmstrip.cutscenePlaying = 0;
            ISHELL_CancelTimer(instance->m_pIShell, 0, pScreen);
            kphandset::kpscr_func_1DDAC(pScreen);
        }
        if (kpcutscene::SubtitleHandler(pScreen) || pScreen->currentFilmstrip.filmstripIndex != filmstripIndex)
            kpscreen::RefreshDisplay(instance);
    }
}

boolean kpcutscene::SubtitleHandler(kpcutscene* pScreen)
{
    if (!pScreen->subtitleStartFrame)
        return 0;
    boolean returnData = 0;
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    while (!pScreen->currentFilmstrip.kpfilmstrip_unk6 || pScreen->currentFilmstrip.subtitleIndex < (int)pScreen->currentFilmstrip.subtitleCount && pScreen->currentFilmstrip.filmstripIndex >(int)pScreen->currentFilmstrip.kpfilmstrip_unk7_2)
    {
        returnData = 1;
        if (pScreen->currentFilmstrip.kpfilmstrip_unk6)
        {
            for (pScreen->currentFilmstrip.kpfilmstrip_unk6 += 4; *pScreen->currentFilmstrip.kpfilmstrip_unk6; ++pScreen->currentFilmstrip.kpfilmstrip_unk6)
            {
                ;
            }
            ++pScreen->currentFilmstrip.kpfilmstrip_unk6;
            ++pScreen->currentFilmstrip.subtitleIndex;
        }
        else
        {
            pScreen->currentFilmstrip.kpfilmstrip_unk6 = (char*)pScreen->subtitleStartFrame;
        }
        if (pScreen->currentFilmstrip.subtitleIndex >= (int)pScreen->currentFilmstrip.subtitleCount)
        {
            pScreen->currentFilmstrip.kpfilmstrip_unk7_1 = pScreen->currentFilmstrip.frameCount;
            pScreen->currentFilmstrip.kpfilmstrip_unk7_2 = pScreen->currentFilmstrip.frameCount;
        }
        else
        {
            pScreen->currentFilmstrip.kpfilmstrip_unk7_1 = *(unsigned char*)pScreen->currentFilmstrip.kpfilmstrip_unk6 + (*((unsigned char*)pScreen->currentFilmstrip.kpfilmstrip_unk6 + 1) << 8);
            pScreen->currentFilmstrip.kpfilmstrip_unk7_2 = *((unsigned char*)pScreen->currentFilmstrip.kpfilmstrip_unk6 + 2) + (*((unsigned char*)pScreen->currentFilmstrip.kpfilmstrip_unk6 + 3) << 8);
        }
        pScreen->currentFilmstrip.unk456456_3 = 0;
    }
    if (!pScreen->currentFilmstrip.unk456456_3 && pScreen->currentFilmstrip.filmstripIndex >= (int)pScreen->currentFilmstrip.kpfilmstrip_unk7_1)
    {
        pScreen->currentFilmstrip.unk456456_3 = 1;
        returnData = 1;
    }
    if (returnData)
    {
        if (pScreen->currentFilmstrip.filmstripIndex < (int)pScreen->currentFilmstrip.kpfilmstrip_unk7_1 || pScreen->currentFilmstrip.filmstripIndex >(int)pScreen->currentFilmstrip.kpfilmstrip_unk7_2 || pScreen->currentFilmstrip.kpfilmstrip_unk7_1 == pScreen->currentFilmstrip.kpfilmstrip_unk7_2)
        {
            char tempBuf[4] = { 0,0,0,0 };
            kphandset::kpscr_subtitles_func_305F4(&instance->global_bottom, tempBuf);
        }
        else
        {
            char* translation = kphelpers::ParseTranslation(instance, (const char*)pScreen->currentFilmstrip.kpfilmstrip_unk6 + 4);
            kphandset::kpscr_subtitles_func_305F4(&instance->global_bottom, translation);
        }
    }
    if (pScreen->currentFilmstrip.filmstripIndex >= (int)pScreen->currentFilmstrip.kpfilmstrip_unk7_1 && pScreen->currentFilmstrip.filmstripIndex < (int)pScreen->currentFilmstrip.kpfilmstrip_unk7_2 && pScreen->currentFilmstrip.kpfilmstrip_unk7_1 < (int)pScreen->currentFilmstrip.kpfilmstrip_unk7_2)
    {
        short amount = ((100 * (pScreen->currentFilmstrip.filmstripIndex - pScreen->currentFilmstrip.kpfilmstrip_unk7_1)) / (pScreen->currentFilmstrip.kpfilmstrip_unk7_2 - pScreen->currentFilmstrip.kpfilmstrip_unk7_1));
        if (kpcutscene::SplitSubtitles(&instance->global_bottom, amount))
            return 1;
    }
    return returnData;
}

boolean kpcutscene::SplitSubtitles(kpBottom* pBottom, int amount)
{
    int kpBottom_unk11_1 = pBottom->kpBottom_unk11_1;
    pBottom->kpBottom_unk11_1 = 0;
    if (pBottom->subtitleSplitIndex > 3u)
    {
        uint16 splitIndexVal = (((pBottom->subtitleSplitIndex + 1) * amount) / 101u);
        pBottom->kpBottom_unk11_1 = 3 * (splitIndexVal / 3u);
        if (pBottom->kpBottom_unk11_1 >= (int)pBottom->subtitleSplitIndex)
            pBottom->kpBottom_unk11_1 = pBottom->subtitleSplitIndex;
    }
    return pBottom->kpBottom_unk11_1 != kpBottom_unk11_1;
}

bool kpcutscene::HandleEvent(kpcutscene* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam)
{
    if (eCode == EVT_KEY && wParam == AVK_SELECT && pScreen->currentFilmstrip.cutscenePlaying && pScreen->currentFilmstrip.filmstripIndex > 5u && pScreen->currentFilmstrip.skipCutscene)
    {
        kphandset* instance = (kphandset*)GETAPPINSTANCE();
        kpcutscene::StopCutscene(instance, pScreen);
        kphandset::kpscr_func_1DDAC(pScreen);
        return 1;
    }
    else
    {
        if (eCode == EVT_KPHANDSET_APP_AUDIO_INIT)
        {
            pScreen->currentFilmstrip.audioPlaying = 1;
        }
        else if (eCode == EVT_KPHANDSET_APP_SOCKET_PING)
        {
            kpcutscene::LoadCutscene((kpcutscene*)dwParam);
        }
        return kpscreen::HandleEvent(pScreen, eCode, wParam, dwParam);
    }
}

void kpcutscene::Draw(kpcutscene* pScreen)
{
    if (pScreen->currentFilmstrip.cutscenePlaying)
    {
        if (pScreen->currentFilmstrip.frames)
        {
            if (pScreen->currentFilmstrip.filmstripIndex < (int)pScreen->currentFilmstrip.frameCount)
            {
                uint8 frameData = pScreen->currentFilmstrip.frames[pScreen->currentFilmstrip.filmstripIndex];
                uint8 frameIndex = frameData & 0x1F;
                uint8 imageIndex = ((frameData & 0xE0) >> 5) % 5; //turns out, if it's the remainder, its modulo not divide!
                if (pScreen->filmstripImages[imageIndex])
                    IIMAGE_DrawFrame(pScreen->filmstripImages[imageIndex], frameIndex, 0, 0);
            }
        }
        if (!pScreen->currentFilmstrip.frames && pScreen->filmstripImages[0])
            IIMAGE_DrawFrame(pScreen->filmstripImages[0], 0, 0, 0);
        kphandset* instance = (kphandset*)GETAPPINSTANCE();
        kphandset::kpscr_RenderSubtitles(&instance->global_bottom);
    }
    kpscreen::Draw(pScreen);
}

void kpcutscene::Release(kpcutscene* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    ISHELL_CancelTimer(instance->m_pIShell, 0, pScreen);
    for (int i = 0; i < 5; ++i)
    {
        if (pScreen->filmstripImages[i])
        {
            pScreen->filmstripImages[i]->pvt->Release(pScreen->filmstripImages[i]);
            pScreen->filmstripImages[i] = 0;
        }
    }
    kpscreen::Release(pScreen);
}

void kpcutscene::Init(kpcutscene* pScreen, int initialize)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (initialize)
    {
        kpcutscene::PingSocket(pScreen);
        kpnetwork::StartSocketPingTimer(instance, 0);
        instance->network.keepalive = instance->network.default_keepalive;
        kpnetwork::CancelConnection(instance);
    }
    else
    {
        kpcutscene::StopCutscene(instance, pScreen);
        kpnetwork::StartSocketPingTimer(instance, instance->network.base_ping);
        instance->network.keepalive = instance->network.default_keepalive;
    }
    if (initialize == 1)
        kphandset::kpscr_func_2AB8C(instance);
}