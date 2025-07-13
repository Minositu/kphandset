#include "kptimeout.h"
#include "kpdebug.h"
#include "kphelpers.h"




void kptimeout::InitGlobalTimeoutMgr(kphandset* pApp)
{
    pApp->global_one = 1020;
    pApp->global_two = 480;
    pApp->global_alert_count = 10;
    char* global_one = kphelpers::ReadScriptBuf("$GLOBAL_ONE");
    if (*global_one && *global_one != 36)
        pApp->global_one = ATOI(global_one);
    char* global_two = kphelpers::ReadScriptBuf("$GLOBAL_TWO");
    if (*global_two && *global_two != 36)
        pApp->global_two = ATOI(global_two);
    char* global_alert_count = kphelpers::ReadScriptBuf("$GLOBAL_ALERT_COUNT");
    if (*global_alert_count && *global_alert_count != 36)
        pApp->global_alert_count = ATOI(global_alert_count);
    pApp->global_timeout_remaining = 0;
    pApp->globalTimeoutState = 0;
}

void kptimeout::HandleTimeout(kphandset* pApp)
{
    const char* state;
    int global_alert_count;
    int globalTimeoutState = pApp->globalTimeoutState;
    if (globalTimeoutState == 1)
    {
        state = "s_timeout_one";
        global_alert_count = pApp->global_alert_count;
        pApp->global_timeout_remaining = pApp->global_two;
        pApp->globalTimeoutState = 2;
    }
    else if (pApp->globalTimeoutState == 3)
    {
        state = "s_atexit";
        global_alert_count = pApp->global_alert_count;
        pApp->global_timeout_remaining = pApp->global_two;
        pApp->globalTimeoutState = 4;
    }
    else
    {
        if (pApp->globalTimeoutState == 4)
            state = "s_atexit";
        else
            state = "s_timeout_two";
        global_alert_count = pApp->global_alert_count;
        pApp->global_timeout_remaining = 0;
        pApp->globalTimeoutState = 100;
    }
    //kpscreen_endgame* screen = kphandset::kpscr_Command_EndGame(pApp, state, global_alert_count, globalTimeoutState);
    //if (screen)
    //    kphandset::kpstartup_FetchSelectedStartup(pApp, screen);
    if (pApp->global_timeout_remaining)
        ISHELL_SetTimer(pApp->m_pIShell, 1000, (PFNNOTIFY)kptimeout::CheckTimeout, pApp);
    else
        kptimeout::ClearTimeout(pApp);
}

void kptimeout::CheckTimeout(kphandset* pApp)
{
    ASSERT(!pApp->globalTimeoutState, "pApp->globalTimeoutState != GLOBAL_TIMER_DISABLED"); //Line 66
    int checkTimeout = 0;
    if (pApp->global_timeout_remaining == 1)
    {
        pApp->global_timeout_remaining = 0;
        ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_TIMEOUT, 0, 0);
    }
    else if (pApp->global_timeout_remaining > 1)
    {
        --pApp->global_timeout_remaining;
        checkTimeout = 1;
    }
    if (checkTimeout)
       ISHELL_SetTimer(pApp->m_pIShell, 1000, (PFNNOTIFY)kptimeout::CheckTimeout, pApp);
}

void kptimeout::ClearTimeout(kphandset* pApp)
{
    ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kptimeout::CheckTimeout, pApp);
    pApp->global_timeout_remaining = 0;
    pApp->globalTimeoutState = 0;
}