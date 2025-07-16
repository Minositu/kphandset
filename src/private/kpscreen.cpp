#include "kpscreen.h"
#include "kphandset.h"
#include "kphelpers.h"

void kpscreen::ExecuteCommand(kpscreen* pScreen)
{
    pScreen->InitPtr = kpscreen::Init;
    pScreen->DrawPtr = kpscreen::Draw;
    pScreen->HandleEventPtr = kpscreen::HandleEvent;
    pScreen->ReleasePtr = kpscreen::Release;
    pScreen->help_buffer[0] = 0;
    pScreen->retrigger_buffer[0] = 0;
    pScreen->kpstartup_unk11_1 = 0;
}

void kpscreen::RefreshDisplay(kphandset* pApp)
{
    if (pApp->pSelectedStartup)
        pApp->pSelectedStartup->DrawPtr(pApp->pSelectedStartup);
    IDISPLAY_Update(pApp->m_pIDisplay);
}

void kpscreen::FetchSelectedScreen(kphandset* pApp, kpscreen* newScreen)
{
    if (pApp->pSelectedStartup != newScreen)
    {
        kpscreen* pSelectedStartup = pApp->pSelectedStartup;
        if (pSelectedStartup)
        {
            pSelectedStartup->InitPtr(pSelectedStartup, 0);
            pApp->pSelectedStartup = 0;
            pSelectedStartup->ReleasePtr(pSelectedStartup);
        }
        pApp->pSelectedStartup = newScreen;
        if (newScreen)
            newScreen->InitPtr(newScreen, 1);
        kpscreen::RefreshDisplay(pApp);
    }
}

void kpscreen::ClearStartup(kphandset* pApp, void* startup)
{
    if (startup == &pApp->kpstartup1)
    {
        MEMSET(startup, 205, 4564);
        pApp->kphandset_isStartup1 = 0;
    }
    else if (startup == &pApp->kpstartup2)
    {
        MEMSET(startup, 205, 4564);
        pApp->kphandset_isStartup2 = 0;
    }
}

bool kpscreen::HandleEvent(kpscreen* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam)
{
    if (eCode == EVT_KEY && wParam == AVK_SOFT2 && pScreen->help_buffer[0])
    {
        kphelpers::FetchGoToToken((kphandset*)GETAPPINSTANCE(), pScreen->help_buffer);
        kphandset::kpscr_func_1DDAC(pScreen);
    }
    else if (eCode == EVT_KEY && wParam == AVK_SOFT1 && pScreen->retrigger_buffer[0])
    {
        kphelpers::FetchGoToToken((kphandset*)GETAPPINSTANCE(), pScreen->retrigger_buffer);
        kphandset::kpscr_func_1DDAC(pScreen);
    }
    return 0;
}

void kpscreen::Draw(kpscreen* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pScreen->help_buffer[0] && instance->ui_help_Interface)
        IMODULE_CreateInstance((IModule*)instance->ui_help_Interface, 0, 0, 0);
    if (pScreen->retrigger_buffer[0])
    {
        if (instance->ui_retrigger_Interface)
            IMODULE_CreateInstance((IModule*)instance->ui_retrigger_Interface, 0, 0, 0);
    }
}

void kpscreen::Release(kpscreen* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    kpscreen::ClearStartup(instance, pScreen);
}

void kpscreen::Init(kpscreen* pScreen, int initialize)
{
    //Intentionally left empty
}