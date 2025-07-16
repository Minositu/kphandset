#include <AEEMediaUtil.h>

#include "kpaudio.h"
#include "kpdebug.h"
#include "kphelpers.h"
#include "kphandset.h"















void kpaudio::InitAudioMgr(kphandset* pApp)
{
	pApp->pAudio.songPath[0] = 0;
	kpaudio::Initialize(&pApp->pAudio);
}

void kpaudio::Free(kphandset* pApp)
{
    pApp->pAudio.songPath[0] = 0;
    kpaudio::Initialize(&pApp->pAudio);
}

void kpaudio::LoadAudioFile(const char* path)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    kpaudio* p_pAudio = &instance->pAudio;
    if (instance->pAudio.audioStatus != 2 || STRCMP((const char*)p_pAudio, path))
    {
        if (path && *path && (kphelpers::NullTerminatedString(p_pAudio->songPath, path, 32), !p_pAudio->audioStatus))
            kpaudio::PlayMedia(p_pAudio);
        else
            kpaudio::StopMedia(p_pAudio);
    }
}

void kpaudio::MediaNotify(kphandset* pApp, AEEMediaCmdNotify* pCmdNotify)
{
    kpaudio* p_pAudio = &pApp->pAudio;
    if (pCmdNotify->nCmd == 4)
    {
        int nStatus = pCmdNotify->nStatus;
        if (nStatus == MM_STATUS_ABORT)
        {
            kpaudio::StopMedia(&pApp->pAudio);
            kpaudio::Initialize(&pApp->pAudio);
            ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_USER, 0, 0);
            ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, 28674, 0, 0);
        }
        else if (nStatus > MM_STATUS_ABORT)
        {
            if (nStatus == MM_STATUS_PAUSE)
            {
                p_pAudio->pAbortCmdData = pCmdNotify->pCmdData;
                p_pAudio->PausedUpTimeMS = kpaudio::GetUpTimeMS(pApp);
                p_pAudio->UpTimeMS = p_pAudio->PausedUpTimeMS;
            }
            else if (nStatus == MM_STATUS_RESUME)
            {
                p_pAudio->pAbortCmdData = pCmdNotify->pCmdData;
                p_pAudio->UpTimeMS = GETUPTIMEMS();
            }
        }
        else if (nStatus == MM_STATUS_START)
        {
            p_pAudio->audioStatus = 2;
            p_pAudio->pAbortCmdData = 0;
            p_pAudio->PausedUpTimeMS = 0;
            p_pAudio->UpTimeMS = GETUPTIMEMS();
            IMEDIA_SetMediaParm(p_pAudio->pMedia, 4, 100, 0);
        }
        else if (nStatus == MM_STATUS_DONE)
        {
            p_pAudio->audioStatus = 3;
            ISHELL_SetTimer(pApp->m_pIShell, 10, (PFNNOTIFY)kpaudio::MediaTimer, p_pAudio);
        }
    }
    else if (pCmdNotify->nCmd == MM_CMD_GETTOTALTIME)
    {
        if (pCmdNotify->nStatus == MM_STATUS_DONE)
            pApp->pAudio.pTotalTimeCmdData = pCmdNotify->pCmdData;
        IMEDIA_Play(pCmdNotify->pIMedia);
    }
}

void kpaudio::Initialize(kpaudio* pAudio)
{
    pAudio->KPAudio_unk1_1 = 0;
    pAudio->audioBuffer[0] = 0;
    if (pAudio->pMedia)
    {
        IMEDIA_Release(pAudio->pMedia);
        pAudio->pMedia = 0;
    }
    pAudio->pMediaData.pData = 0;
    pAudio->pTotalTimeCmdData = 0;
    pAudio->pAbortCmdData = 0;
    pAudio->audioStatus = 0;
}

void kpaudio::MediaTimer(kpaudio* pAudio)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pAudio->audioStatus == 3)
    {
        ASSERT(!pAudio->pMedia, "pAudio->pMedia"); //Line 115
        boolean v3;
        if (IMEDIA_GetState(pAudio->pMedia, &v3) == 2)
        {
            kpaudio::Initialize(pAudio);
            kpaudio::PlayMedia(pAudio);
        }
        else
        {
            ISHELL_SetTimer(instance->m_pIShell, 10, (PFNNOTIFY)kpaudio::MediaTimer, pAudio);
        }
    }
}


int kpaudio::GetUpTimeMS(kphandset* pApp)
{
    boolean pbStateChanging;
    kpaudio* p_pAudio = &pApp->pAudio;
    int upTime = 0;
    if (pApp->pAudio.audioStatus == 2 && IMEDIA_GetState(pApp->pAudio.pMedia, &pbStateChanging) == 3)
        return p_pAudio->PausedUpTimeMS + GETUPTIMEMS() - p_pAudio->UpTimeMS;
    return upTime;
}


void kpaudio::StopMedia(kpaudio* pAudio)
{
    if (pAudio->audioStatus == 1 || pAudio->audioStatus == 2)
    {
        ASSERT(!pAudio->pMedia, "pAudio->pMedia"); //Line 145
        IMEDIA_Stop(pAudio->pMedia);
    }
}

void kpaudio::PlayMedia(kpaudio* pAudio)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pAudio && !pAudio->audioStatus && pAudio->songPath[0])
    {
        pAudio->audioStatus = 1;
        kphelpers::NullTerminatedString((char*)pAudio, pAudio->songPath, 32);
        pAudio->songPath[0] = 0;
        char* audioFile = kphelpers::ReadToScratch(instance, (char*)pAudio, ".mp3");
        if (audioFile)
            kphelpers::NullTerminatedString(pAudio->audioBuffer, audioFile, 64);
        pAudio->PausedUpTimeMS = 0;
        pAudio->UpTimeMS = GETUPTIMEMS();
        pAudio->pMediaData.clsData = 0;
        pAudio->pMediaData.pData = pAudio->audioBuffer;
        pAudio->pMediaData.dwSize = 0;
        int returnData = AEEMediaUtil_CreateMedia(instance->m_pIShell, &pAudio->pMediaData, &pAudio->pMedia);
        if (!returnData)
            returnData = IMEDIA_RegisterNotify(pAudio->pMedia, (PFNMEDIANOTIFY)kpaudio::MediaNotify, instance);
        if (!returnData)
        {
            returnData = IMEDIA_GetTotalTime(pAudio->pMedia);
            if (returnData)
                returnData = IMEDIA_Play(pAudio->pMedia);
        }
        if (returnData)
            kpaudio::Initialize(pAudio);
    }
    if (!pAudio->audioStatus)
    {
        ISHELL_PostEvent(instance->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_AUDIO_INIT, 0, 0);
    }
}