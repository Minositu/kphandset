#include "kpclient.h"
#include "kpdebug.h"
#include "kptimeout.h"
#include "kpadmin.h"
#include "kphelpers.h"

kpscreen* kpclient::ExecuteCommand(kphandset* pApp)
{
    kpclient* client = (kpclient*)kphandset::FetchScreen(pApp);
    if (client)
    {
        kpscreen::ExecuteCommand(client);
        client->InitPtr = (void (*)(kpscreen*, int))kpclient::Init;
        client->DrawPtr = (void (*)(kpscreen*))kpclient::Draw;
        client->HandleEventPtr = (bool (*)(kpscreen*, AEEEvent, uint16, uint32))kpclient::HandleEvent;
        client->ReleasePtr = (void (*)(kpscreen*))kpclient::Release;
        AEERect rect;
        rect.x = 0;
        rect.y = 0;
        rect.dx = pApp->pBitmapInfo.cx;
        rect.dy = pApp->pBitmapInfo.cy;
        if (!ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_STATIC, (void**)&client->pHandsetID))
        {
            int d;
            int a;
            IDISPLAY_GetFontMetrics(pApp->m_pIDisplay, AEE_FONT_USER_2, &a, &d);
            rect.dy = a + d;
            rect.y = pApp->pBitmapInfo.cy - (a + d);
            ISTATIC_SetRect(client->pHandsetID, &rect);
            ISTATIC_SetProperties(client->pHandsetID, ST_ASCII | ST_CENTERTEXT);
            ISTATIC_SetText(client->pHandsetID, 0, (AECHAR*)pApp->handsetID, AEE_FONT_USER_2, AEE_FONT_USER_2);
        }
        if (!ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_STATIC, (void**)&client->pNotice))
        {
            rect.dy = rect.y - 30;
            rect.y = 30;
            ISTATIC_SetRect(client->pNotice, &rect);
            ISTATIC_SetProperties(client->pNotice, ST_ASCII | ST_CENTERTEXT);
            char* noticePath = kphelpers::ReadFromRootSD(pApp, "notice", ".txt");
            char* noticeChar = kphelpers::LoadStringFromFile(pApp, noticePath, 0, 0);
            ISTATIC_SetText(client->pNotice, 0, (AECHAR*)noticeChar, AEE_FONT_LARGE, AEE_FONT_LARGE);
            if (noticeChar)
                FREE(noticeChar);
        }
        if (!ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_STATIC, (void**)&client->pVersion))
        {
            SNPRINTF(pApp->scratch, 400u, "ver %s:%s", "1004-22", (const char*)&pApp->contentVersion);
            rect.dy = 30;
            rect.y = 0;
            ISTATIC_SetRect(client->pVersion, &rect);
            ISTATIC_SetProperties(client->pVersion, ST_ASCII | ST_CENTERTEXT);
            ISTATIC_SetText(client->pVersion, 0, (AECHAR*)pApp->scratch, AEE_FONT_LARGE, AEE_FONT_LARGE);
        }
    }
    return client;
}

bool kpclient::HandleEvent(kpclient* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    int initialized = 0;
    if (eCode == EVT_KEY && wParam == 57397)
    {
        if (instance->adminMode)
        {
            kpscreen* screen = kpadmin::ExecuteCommand(instance);
            kpscreen::FetchSelectedScreen(instance, screen);
        }
        else
        {
            kpnetwork::Initialize(instance);
        }
        initialized = 1;
    }
    else if (eCode == EVT_KPHANDSET_APP_NETWORK_INCOMING_MESSAGE)
    {
        ISHELL_CancelTimer(instance->m_pIShell, (PFNNOTIFY)kpnetwork::Initialize, instance);
    }
    else if (eCode == EVT_FLIP)
    {
        if (wParam == 1)
        {
            kpdebug::Print((char*)"STARTUP: Attempting connection");
            kpnetwork::StartSocketPingTimer(instance, instance->network.base_ping);
            kpnetwork::ClearExistingConnection(instance);
            ISHELL_SetTimer(instance->m_pIShell, 15000, (PFNNOTIFY)kpnetwork::Initialize, instance);
        }
        else
        {
            kpdebug::Print((char*)"STARTUP: Cancelling connection");
            kpnetwork::StartSocketPingTimer(instance, 0);
            kpnetwork::CancelConnection(instance);
            ISHELL_CancelTimer(instance->m_pIShell, (PFNNOTIFY)kpnetwork::Initialize, instance);
        }
    }
    return initialized || kpscreen::HandleEvent(pScreen, eCode, wParam, dwParam);
}

void kpclient::Draw(kpclient* pScreen)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    AEERect rect;
    rect.x = 0;
    rect.y = 0;
    rect.dx = instance->pBitmapInfo.cx;
    rect.dy = instance->pBitmapInfo.cy;
    if (pScreen->kpstartup_client_unk1)
        IIMAGE_Draw(pScreen->kpstartup_client_unk1, 0, 0);
    else
        IDISPLAY_DrawRect(instance->m_pIDisplay, &rect, -1, -1, 2u);
    if (pScreen->pVersion)
        ISTATIC_Redraw(pScreen->pVersion);
    if (pScreen->pNotice)
        ISTATIC_Redraw(pScreen->pNotice);
    if (pScreen->pHandsetID)
        ISTATIC_Redraw(pScreen->pHandsetID);
    kpscreen::Draw(pScreen);
}

void kpclient::Release(kpclient* pScreen)
{
    if (pScreen->kpstartup_client_unk1)
    {
        IIMAGE_Release(pScreen->kpstartup_client_unk1);
        pScreen->kpstartup_client_unk1 = 0;
    }
    if (pScreen->pHandsetID)
    {
        ISTATIC_Release(pScreen->pHandsetID);
        pScreen->pHandsetID = 0;
    }
    if (pScreen->pNotice)
    {
        ISTATIC_Release(pScreen->pNotice);
        pScreen->pNotice = 0;
    }
    if (pScreen->pVersion)
    {
        ISTATIC_Release(pScreen->pVersion);
        pScreen->pVersion = 0;
    }
    kpscreen::Release(pScreen);
}

void kpclient::Init(kpclient* pScreen, int initialize)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pScreen->pNotice)
        ISTATIC_SetActive(pScreen->pNotice, initialize);
    if (pScreen->pVersion)
        ISTATIC_SetActive(pScreen->pVersion, initialize);
    if (pScreen->pHandsetID)
    {
        ISTATIC_SetActive(pScreen->pHandsetID, initialize);
        if (instance->lowBattery)
        {
            ISTATIC_SetText(pScreen->pHandsetID, 0, (AECHAR*)"LOW BATTERY", AEE_FONT_USER_2, AEE_FONT_USER_2);
        }
    }
    if (initialize)
    {
        kpnetwork::ClearExistingConnection(instance);
        ISHELL_SetTimer(instance->m_pIShell, 15000, (PFNNOTIFY)kpnetwork::Initialize, instance);
    }
    else
    {
        ISHELL_CancelTimer(instance->m_pIShell, (PFNNOTIFY)kpnetwork::Initialize, instance);
    }
    if (initialize == 1)
        kptimeout::ClearTimeout(instance);
}