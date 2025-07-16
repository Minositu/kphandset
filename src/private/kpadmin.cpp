#include "kpadmin.h"
#include "kphelpers.h"
#include "kptimeout.h"

kpscreen* kpadmin::ExecuteCommand(kphandset* pApp)
{
    kpadmin* startup = (kpadmin*)kphandset::FetchScreen(pApp);
    if (startup)
    {
        int a;
        int d;
        AEERect rect;
        kphandset::kpscreen_Initialize(startup);
        startup->InitPtr = (void (*)(kpscreen*, int))kpadmin::Init;
        startup->DrawPtr = (void (*)(kpscreen*))kpadmin::Draw;
        startup->HandleEventPtr = (bool (*)(kpscreen*, AEEEvent, uint16, uint32))kpadmin::HandleEvent;
        startup->ReleasePtr = (void (*)(kpscreen*))kpadmin::Release;
        startup->pavillion[1] = 120;
        startup->pavillion[0] = 120;
        startup->kpstartupAdmin_unk2_2 = 1;
        IDISPLAY_GetFontMetrics(pApp->m_pIDisplay, AEE_FONT_USER_1, &a, &d);
        rect.x = 0;
        rect.y = 0;
        rect.dx = pApp->pBitmapInfo.cx;
        rect.dy = 2 * (a + d);
        if (!ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_STATIC, (void**)&startup->pHandsetID))
        {
            ISTATIC_SetRect(startup->pHandsetID, &rect);
            ISTATIC_SetFont(startup->pHandsetID, AEE_FONT_USER_1, AEE_FONT_USER_1);
            ISTATIC_SetProperties(startup->pHandsetID, ST_ASCII | ST_CENTERTEXT);
        }
        rect.y += rect.dy;
        rect.dy = pApp->pBitmapInfo.cy - rect.y;
        if (!ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_MENUCTL, (void**)&startup->pMenuCtl))
        {
            AEEItemStyle pNormal, pSel;
            IMENUCTL_SetRect(startup->pMenuCtl, &rect);
            IMENUCTL_GetStyle(startup->pMenuCtl, &pNormal, &pSel);
            pNormal.ft = AEE_FT_NONE;
            pNormal.xOffset = 0;
            pSel.ft = AEE_FT_RAISED;
            pSel.xOffset = 1;
            IMENUCTL_SetStyle(startup->pMenuCtl, &pNormal, &pSel);
        }
        char* AdminPath = kphelpers::ReadFromAdminPath(pApp, (char*)"admin", (char*)".txt");
        kpadmin::ParseMenu(startup, AdminPath);
    }
    return startup;
}

void kpadmin::CreateOutgoingMessage(kphandset* pApp, const char* message)
{
    kpnetwork::CreateOutgoingMessage(pApp, message, 0);
}

void kpadmin::CommandParser(kpadmin* pScreen, int unk2, char* command)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    char* commandVal = STRCHREND(command, 58);
    if (*commandVal)
        ++commandVal;
    if (STRBEGINS("trigger:", command))
    {
        SNPRINTF(instance->scratch, 400u, "<admin handset='%s' value='connect' />", instance->handsetID);
        kpadmin::CreateOutgoingMessage(instance, instance->scratch);
        SNPRINTF(instance->scratch, 400u, "<admin handset='%s' gag='%s' value='trigger' />", instance->handsetID, commandVal);
        kpadmin::CreateOutgoingMessage(instance, instance->scratch);
        SNPRINTF(instance->scratch, 400u, "<admin handset='%s' value='disconnect' />", instance->handsetID);
        kpadmin::CreateOutgoingMessage(instance, instance->scratch);
        SNPRINTF(instance->scratch, 400u, "Sending Trigger:\n%s", commandVal);
        kpadmin::RefreshDisplay(pScreen, instance->scratch, 1);
        kpnetwork::StartSocketPingTimer(instance, 5000);
    }
    if (STRBEGINS("mode:", command))
    {
        SNPRINTF(instance->scratch, 400u, "<admin handset='%s' value='connect' />", instance->handsetID);
        kpadmin::CreateOutgoingMessage(instance, instance->scratch);
        SNPRINTF(instance->scratch, 400u, "<admin handset='%s' mode='%s' value='trigger' />", instance->handsetID, commandVal);
        kpadmin::CreateOutgoingMessage(instance, instance->scratch);
        SNPRINTF(instance->scratch, 400u, "<admin handset='%s' value='disconnect' />", instance->handsetID);
        kpadmin::CreateOutgoingMessage(instance, instance->scratch);
        SNPRINTF(instance->scratch, 400u, "Sending Mode:\n%s", commandVal);
        kpadmin::RefreshDisplay(pScreen, instance->scratch, 1);
        kpnetwork::StartSocketPingTimer(instance, 5000);
    }
    else if (STRBEGINS("episode:", command))
    {
        kpnetwork::SetServer(instance, instance->network.ipAddress, 1234);
        SNPRINTF(instance->scratch, 400u, "<admin handset='%s' episode='%s' />", instance->handsetID, commandVal);
        kpadmin::CreateOutgoingMessage(instance, instance->scratch);
        *(char*)pScreen->pavillion = *(char*)commandVal;
        if (pScreen->pMenuCtl)
            pScreen->pMenuCtl->pvt->DeleteAll(pScreen->pMenuCtl);
        SNPRINTF(instance->scratch, 400u, "Connecting to Game Server\nEpisode: '%s'", commandVal);
        kpadmin::RefreshDisplay(pScreen, instance->scratch, 1);
        kpnetwork::StartSocketPingTimer(instance, 5000);
    }
    else if (STRBEGINS("menu:", command))
    {
        char* menuFile = kphelpers::ReadFromAdminPath(instance, commandVal, (char*)".txt");
        kpadmin::ParseMenu(pScreen, menuFile);
        pScreen->pavillion[0] = 120;
    }
}

void kpadmin::ParseMenu(kpadmin* pScreen, const char* path)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pScreen->pMenuCtl)
    {
        if (pScreen->pNotice)
        {
            FREE(pScreen->pNotice);
            pScreen->pNotice = 0;
        }
        IMENUCTL_DeleteAll(pScreen->pMenuCtl);
        pScreen->pNotice = kphelpers::LoadStringFromFile(instance, path, 0, 0);
        uint16 itemIndex = 0;
        char* pNotice = pScreen->pNotice;
        while (pNotice && *pNotice)
        {
            while (*pNotice && kphandset::kp_CheckToken((unsigned char)*pNotice))
                ++pNotice;
            char* noticeBuf = pNotice;
            while (*pNotice && *pNotice != 13 && *pNotice != 10)
                ++pNotice;
            if (*pNotice)
                *pNotice++ = 0;
            if (*noticeBuf != 35 && *noticeBuf)
            {
                if (STRBEGINS("title:", noticeBuf))
                {
                    kpadmin::RefreshDisplay(pScreen, noticeBuf + 6, 1);
                }
                else
                {
                    uint32 noticeVal = (uint32)noticeBuf;
                    while (*noticeBuf && !kphandset::kp_CheckToken((unsigned char)*noticeBuf))
                        ++noticeBuf;
                    if (*noticeBuf)
                        *noticeBuf++ = 0;
                    while (*noticeBuf && kphandset::kp_CheckToken((unsigned char)*noticeBuf))
                        ++noticeBuf;
                    ++itemIndex;
                    STRTOWSTR(noticeBuf, (AECHAR*)instance->scratch, 400);
                    IMENUCTL_AddItem(pScreen->pMenuCtl, NULL, 0, itemIndex, (AECHAR*)instance->scratch, noticeVal);
                }
            }
        }
        kpnetwork::StartSocketPingTimer(instance, 0);
        kphandset::kpscreen_RefreshDisplay(instance);
    }
}

void kpadmin::RefreshDisplay(kpadmin* pScreen, char* pText, char a3)
{
    if (pScreen->pHandsetID)
    {
        pScreen->kpstartupAdmin_unk2_2 = a3;
        ISTATIC_SetText(pScreen->pHandsetID, 0, (AECHAR*)pText, AEE_FONT_USER_1, AEE_FONT_USER_1);
        kphandset::kpscreen_RefreshDisplay((kphandset*)GETAPPINSTANCE());
    }
}

bool kpadmin::HandleEvent(kpadmin* pScreen, AEEEvent eCode, uint16 wParam, uint32 dwParam)
{
    int returnVal = 0;
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (pScreen->pMenuCtl)
        returnVal = IMENUCTL_HandleEvent(pScreen->pMenuCtl, eCode, wParam, dwParam) != 0;
    if (!returnVal && pScreen->pHandsetID)
        returnVal = ISTATIC_HandleEvent(pScreen->pHandsetID, eCode, wParam, dwParam) != 0;
    switch (eCode)
    {
    case EVT_COMMAND:
        kpadmin::CommandParser(pScreen, wParam, (char*)dwParam);
        returnVal = 1;
        return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
        break;
    case EVT_KPHANDSET_APP_NETWORK_CONNECTION_ERROR:
        kpnetwork::SetServer(instance, instance->network.ipAddress, 1234);
        if (pScreen->pavillion[0] != 120 || pScreen->pavillion[1] != 120)
        {
            pScreen->pavillion[1] = 120;
            pScreen->pavillion[0] = 120;
            kpadmin::ParseMenu(pScreen, kphelpers::ReadFromAdminPath(instance, (char*)"admin", (char*)".txt"));
        }
        kpadmin::RefreshDisplay(pScreen, (char*)"Network Connection Error", 0);
        kpnetwork::StartSocketPingTimer(instance, 0);
        return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
        break;
    case EVT_KPHANDSET_APP_NETWORK_WRITE_ERROR:
        kpadmin::RefreshDisplay(pScreen, (char*)"Network Write Error", 0);
        return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
        break;
    case EVT_KPHANDSET_APP_NETWORK_READ_ERROR:
        kpadmin::RefreshDisplay(pScreen, (char*)"Network Read Error", 0);
        return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
        break;
    }
    if (eCode != EVT_KPHANDSET_APP_NETWORK_INCOMING_MESSAGE)
        return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
    int refreshDisplay = 1;
    char* a2a = 0;
    if (STRBEGINS("<reconnect", (const char*)dwParam + 2))
    {
        returnVal = 1;
        char* ipBuf = kphandset::kphandset_ReadFromScratch((const char*)(dwParam + 2), "ip");
        if (ipBuf && *ipBuf)
        {
            kpnetwork::SetServer(instance, ipBuf, 0);
            kpadmin::ParseMenu(pScreen, kphelpers::ReadFromAdminPath(instance, pScreen->pavillion, (char*)".txt"));
        }
        kpnetwork::StartSocketPingTimer(instance, 0);
        if (a2a && *a2a)
            kpadmin::RefreshDisplay(pScreen, a2a, refreshDisplay);
        return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
    }
    if (!STRBEGINS("<state", (const char*)dwParam + 2) && !STRBEGINS("<admin", (const char*)dwParam + 2))
    {
        a2a = (char*)(dwParam + 2);
        if (a2a && *a2a)
            kpadmin::RefreshDisplay(pScreen, a2a, refreshDisplay);
        return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
    }
    returnVal = 1;
    a2a = kphandset::kphandset_ReadFromScratch((const char*)dwParam + 2, "value");
    if (a2a && STRBEGINS("error", a2a))
    {
        a2a = kphandset::kphandset_ReadFromScratch((const char*)dwParam + 2, "error");
        refreshDisplay = 0;
        kpnetwork::StartSocketPingTimer(instance, 0);
    }
    else if (a2a && STRBEGINS("gagComplete", a2a) || a2a && STRBEGINS("ok", a2a))
    {
        kpnetwork::StartSocketPingTimer(instance, 0);
    }
    if (a2a && *a2a)
        kpadmin::RefreshDisplay(pScreen, a2a, refreshDisplay);
    return returnVal || kphandset::kpscreen_HandleEvent(pScreen, eCode, wParam, dwParam);
}

void kpadmin::Draw(kpadmin* pScreen)
{
	kphandset* instance = (kphandset*)GETAPPINSTANCE();
    IDISPLAY_SetColor(instance->m_pIDisplay, CLR_USER_BACKGROUND, 0xFFFFFFFF);
    if (pScreen->pMenuCtl)
        IMENUCTL_Redraw(pScreen->pMenuCtl);
    if (pScreen->kpstartupAdmin_unk2_2)
        IDISPLAY_SetColor(instance->m_pIDisplay, CLR_USER_BACKGROUND, 0xFFFFFFFF);
    else
        IDISPLAY_SetColor(instance->m_pIDisplay, CLR_USER_BACKGROUND, 0x8080E0FF);
    if (pScreen->pHandsetID)
        ISTATIC_Redraw(pScreen->pHandsetID);
    IDISPLAY_SetColor(instance->m_pIDisplay, CLR_USER_BACKGROUND, 0xFFFFFFFF);
    kphandset::kpscreen_Draw(pScreen);
}

void kpadmin::Release(kpadmin* pScreen)
{
    if (pScreen->pHandsetID)
    {
        ISTATIC_Release(pScreen->pHandsetID);
        pScreen->pHandsetID = 0;
    }
    if (pScreen->pMenuCtl)
    {
        IMENUCTL_Release(pScreen->pMenuCtl);
        pScreen->pMenuCtl = 0;
    }
    if (pScreen->pNotice)
    {
        FREE(pScreen->pNotice);
        pScreen->pNotice = 0;
    }
    kphandset::kpscreen_Release(pScreen);
}

void kpadmin::Init(kpadmin* pScreen, int initialize)
{
    if (pScreen->pHandsetID)
        ISTATIC_SetActive(pScreen->pHandsetID, initialize);
    if (pScreen->pMenuCtl)
        IMENUCTL_SetActive(pScreen->pMenuCtl, initialize);
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (initialize == 1)
        instance->network.keepalive = 10000;
    else
        instance->network.keepalive = instance->network.default_keepalive;
    if (initialize == 1)
        kptimeout::ClearTimeout(instance);
}