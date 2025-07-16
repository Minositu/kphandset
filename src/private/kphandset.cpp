/*===========================================================================

FILE: kphandset.c
  
SERVICES: Sample applet using AEE
    
DESCRIPTION
   This file contains a very simple sample application that displays the
   "Hello World" on the display.
      
PUBLIC CLASSES:  
   N/A
        
INITIALIZATION AND SEQUENCING REQUIREMENTS:
   The following explanation applies to this sample containing one applet which serves
   as a base for app developers to create their own applets using AEE Services:
   

   In the applet source file (like this one), include AEEAppGen.h.

   Mandatory Sections in Applet Source (this file):
   -----------------------------------------------
   Following Mandatory Sections are required for each applet source file. 
   (Search for "Mandatory" to identify these sections)

   Includes:     
      Copy this section as-is from the sample applet. It contains:
      AEEAppGen.h: For AEEApplet declaration

   Type Declarations:
		A data structure is usually defined to hold the app specific data. In this structure,
		the first element must be of type AEEApplet.  Note that this simple example does 
      not require any additional data, so this step has been removed.

   Functions: (For more details, see corresponding function description in this applet)

      App_HandleEvent(): This function is the Event Handler for the applet.
                         Copy the function outline from the sample applet and add app
                         specific code.

      AEEClsCreateInstance(): This function is called to create an instance of the applet.
                              It is called by AEEModGen when applet is being created.

   
   Important Notes:
   ---------------
   1. DO NOT use any "static data" in the applet. Always use the functions exported by
	   AEEStdlib or by IHeap services to dynamically allocate data and make it a member of 
		the applet structure. 
   2. DO NOT include and link "standard C library". Use AEE Memory Services (in AEEHeap.h) and Standard Library macros(in AEEStdLib.h).
      For example, use MALLOC() to allocate memory, WSTRCPY() to make a copy of Unicode (wide) string.
   3. BREW is Unicode(wide string) compliant ONLY (no ISOLATIN1/ANSI) except for file names which are ISOLATIN1/ANSI.
      ALWAYS USE AECHAR instead of "char". Use string services provided in AEEStdLib.h for string manipulation.
   4. It is always a good idea to DEFINE RESOURCES using BREW ResourceEditor. Make Strings, Bitmaps, Dialogs, etc. 
      as resources. ResourceEditor saves resources as .bri file, generates resource header file 
      and compiles .bri into a .bar binary file, which can be used by the applet.

   Miscellanoeus Notes:
   -------------------
   1. Make sure that the class ID used for the app is the same as that defined corresponding in .MIF file
   2. Always make sure that compiled resource (.bar) file and corresponding
      resource header (a) reside in app directory and (b) are included in the applet code.
      Define a constant APP_RES_FILE containing the name of the compiled resource file (with .bar extension).

   More than one applet:
   --------------------
   If more than one applet needs to be defined, then do the following
   (1) Follow the above description for each applet




   	   Copyright � 2000-2002 QUALCOMM Incorporated.
	                  All Rights Reserved.
                   QUALCOMM Proprietary/GTDR
===========================================================================*/


/*===============================================================================
INCLUDES AND VARIABLE DEFINITIONS
=============================================================================== */
#include <AEEAppGen.h>        // Applet helper file
#include <AEETelephone.h>
#include <AEESMS.h>
#include <nmdef.h>

#include "kphandset.bid"		// Applet-specific header that contains class ID
#include "kphandset.h"
#include "kpdebug.h"
#include "kphelpers.h"
#include "kpaudio.h"
#include "kpnetwork.h"
#include "kptimeout.h"
#include "kpcutscene.h"
#include "kpadmin.h"
#include "kpstill.h"
#include "kpwait.h"
#include "kpclient.h"

/*-------------------------------------------------------------------
Static function prototypes
-------------------------------------------------------------------*/
static boolean kphandset_HandleEvent(AEEApplet * pme, AEEEvent eCode,uint16 wParam, uint32 dwParam);

/*===============================================================================
FUNCTION DEFINITIONS
=============================================================================== */
/*===========================================================================

FUNCTION: AEEClsCreateInstance

DESCRIPTION
	This function is invoked while the app is being loaded. All Modules must provide this 
	function. Ensure to retain the same name and parameters for this function.
	In here, the module must verify the ClassID and then invoke the AEEApplet_New() function
	that has been provided in AEEAppGen.c. 

   After invoking AEEApplet_New(), this function can do app specific initialization. In this
   example, a generic structure is provided so that app developers need not change app specific
   initialization section every time except for a call to InitAppData(). This is done as follows:
   InitAppData() is called to initialize AppletData instance. It is app developers 
   responsibility to fill-in app data initialization code of InitAppData(). App developer
   is also responsible to release memory allocated for data contained in AppletData -- this can be
   done in FreeAppData().

PROTOTYPE:
	int AEEAppCreateInstance(AEECLSID clsID, IShell* pIShell, IModule* pIModule,IApplet** ppApplet)

PARAMETERS:
	clsID: [in]: Specifies the ClassID of the applet which is being loaded

	pIShell: [in]: Contains pointer to the IShell interface. 

	pIModule: pin]: Contains pointer to the IModule interface to the current module to which
	this app belongs

	ppApplet: [out]: On return, *ppApplet must point to a valid AEEApplet structure. Allocation
	of memory for this structure and initializing the base data members is done by AEEApplet_New().

DEPENDENCIES
  none

RETURN VALUE
  SUCCESS: If the app needs to be loaded and if AEEApplet_New() invocation was successful
  EFAILED: If the app does not need to be loaded or if errors occurred in AEEApplet_New().
  If this function returns FALSE, the app will not be loaded.

SIDE EFFECTS
  none
===========================================================================*/
int AEEClsCreateInstance(AEECLSID ClsId,IShell * pIShell,IModule * pMod,void ** ppObj)
{
   DBGPRINTF("KPHandset starting...");
   *ppObj = NULL;
   ASSERT(ClsId != AEECLSID_KPHANDSET, "ClsId == AEECLSID_KPHANDSET");
   if(AEEApplet_New( sizeof(kphandset),                  // Size of our private class
                     ClsId,                              // Our class ID
                     pIShell,                            // Shell interface
                     pMod,                               // Module instance
                     (IApplet**)ppObj,                   // Return object
                     (AEEHANDLER)kphandset_HandleEvent,  // Our event handler
                     (PFNFREEAPPDATA)kphandset::FreeAppData))       // Special "cleanup" function
      return kphandset::Init((kphandset*)*ppObj);

	return (EFAILED);
}
void kphandset::FreeAppData(AEEApplet* pMe)
{
    kphandset* pApplet = (kphandset*)pMe;

    if (pApplet->pSelectedStartup)
        pApplet->pSelectedStartup->ReleasePtr(pApplet->pSelectedStartup);
    if (pApplet->pGraphics)
    {
        IGRAPHICS_Release(pApplet->pGraphics);
        pApplet->pGraphics = 0;
    }
    if (pApplet->pBacklight)
    {
        IBACKLIGHT_Release(pApplet->pBacklight);
        pApplet->pBacklight = 0;
    }
    if (pApplet->ui_help_Interface)
    {
        IIMAGE_Release(pApplet->ui_help_Interface);
        pApplet->ui_help_Interface = 0;
    }
    if (pApplet->ui_ok_Interface)
    {
        IIMAGE_Release(pApplet->ui_ok_Interface);
        pApplet->ui_ok_Interface = 0;
    }
    if (pApplet->ui_replay_Interface)
    {
        IIMAGE_Release(pApplet->ui_replay_Interface);
        pApplet->ui_replay_Interface = 0;
    }
    if (pApplet->ui_retrigger_Interface)
    {
        IIMAGE_Release(pApplet->ui_retrigger_Interface);
        pApplet->ui_retrigger_Interface = 0;
    }
    if (pApplet->ui_waiting_Interface)
    {
        IIMAGE_Release(pApplet->ui_waiting_Interface);
        pApplet->ui_waiting_Interface = 0;
    }
    if (pApplet->ui_full_Interface)
    {
        IIMAGE_Release(pApplet->ui_full_Interface);
        pApplet->ui_full_Interface = 0;
    }
    if (pApplet->ui_bottom_Interface)
    {
        IIMAGE_Release(pApplet->ui_bottom_Interface);
        pApplet->ui_bottom_Interface = 0;
    }
    if (pApplet->ui_bottom_clear_Interface)
    {
        IIMAGE_Release(pApplet->ui_bottom_clear_Interface);
        pApplet->ui_bottom_clear_Interface = 0;
    }
    for (int i = 0; i < 9; ++i)
    {
        if (pApplet->filmstripBuffer[i].Image)
        {
            IIMAGE_Release(pApplet->filmstripBuffer[i].Image);
            pApplet->filmstripBuffer[i].Image = 0;
        }
    }
    kphandset::kptranslator_Free(pApplet);
    kpnetwork::Shutdown(pApplet);
    kphandset::kpscript_Free(pApplet);
    kpaudio::Free(pApplet);
    if (pApplet->pIFont_User1)
    {
        IHFONT_Release(pApplet->pIFont_User1);
        pApplet->pIFont_User1 = 0;
    }
    if (pApplet->pIFont_Large)
    {
        IHFONT_Release(pApplet->pIFont_Large);
        pApplet->pIFont_Large = 0;
    }
    if (pApplet->pIFont_User2)
    {
        IHFONT_Release(pApplet->pIFont_User2);
        pApplet->pIFont_User2 = 0;
    }
    if (pApplet->pTypeface)
    {
        ITYPEFACE_Release(pApplet->pTypeface);
        pApplet->pTypeface = 0;
    }
    ISHELL_CancelTimer(pApplet->m_pIShell, 0, pApplet);
    if (pApplet->pFileMgr)
    {
        IFILEMGR_Release(pApplet->pFileMgr);
        pApplet->pFileMgr = 0;
    }
}
boolean kphandset::Init(kphandset* pApp)
{
    if (!pApp->pFileMgr)
    {
        ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_FILEMGR, (void**)&pApp->pFileMgr);
    }
    return AEE_SUCCESS;
}
int kphandset::DisplayMessageBox(kphandset* pApp, const char* messageText)
{
    AECHAR* pDest = (AECHAR*)pApp->kphandset_unk154;
    if (messageText == (char*)pApp->kphandset_unk154)
        pDest = (AECHAR*)pApp->scratch;
    *pDest = 0;
    if (messageText)
    {
        char* pszIn = kphelpers::ParseTranslation(pApp, messageText);
        STRTOWSTR(pszIn, pDest, 400);
    }
    return ISHELL_MessageBoxText(pApp->m_pIShell, 0, pDest);
}
int kphandset::CheckLowBattery(kphandset* pApp)
{
    int batteryInfo = 0;
    IBattery* batteryInterface;
    if (!ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_BATTERY, (void**)&batteryInterface))
    {
        boolean pbData;
        int nErr = IBATTERY_IsExternalPowerPresent(batteryInterface, &pbData);
        if (nErr || !pbData)
        {
            uint32 pdwData;
            AEEBatteryStatus pBatteryStatus;
            if (IBATTERY_GetBatteryLevel(batteryInterface, &pdwData) == 1)
            {
                pApp->lowBattery = 100 * pdwData < 70;
                batteryInfo = 1;
            }
            else if (IBATTERY_GetBatteryStatus(batteryInterface, &pBatteryStatus) == 1)
            {
                batteryInfo = 1;
                if (pBatteryStatus == 2)
                {
                    pApp->lowBattery = 1;
                }
                else if (pBatteryStatus)
                {
                    pApp->lowBattery = 0;
                }
                else
                {
                    batteryInfo = 0;
                }
            }
            else
            {
                batteryInfo = 0;
            }
        }
        else
        {
            pApp->lowBattery = 0;
            batteryInfo = 1;
        }
        if (batteryInterface)
        {
            IBATTERY_Release(batteryInterface);
            batteryInterface = 0;
        }
    }
    if (!batteryInfo)
    {
        kpdebug::Print((char*)"SYS: No Battery info - assuming OK");
        pApp->lowBattery = 0;
    }
    if (pApp->lowBattery)
        kpdebug::Print((char*)"SYS: LOW BATTERY!");
    return pApp->lowBattery;
}
char* kphandset::InitScriptMgr(kphandset* pApp)
{
    MEMSET(&pApp->script, 0, 4560);
    char* constantsBuf = kphelpers::ReadFromRootSD(pApp, (char*)"constants", (char*)".txt");
    char* result = kphelpers::LoadStringFromFile(pApp, constantsBuf, 0, &pApp->script.constantsLength);
    if (result)
    {
        char* resultBuf = result;
        pApp->script.constants = result;
        for (result = 0; pApp->script.constantsLength > (unsigned int)result; ++result)
        {
            if (*resultBuf == 10 || *resultBuf == 13)
                *resultBuf = 0;
            ++resultBuf;
        }
    }
    return result;
}
int kphandset::func_28178(kphandset* a1)
{
    if (a1->pBacklight)
        IBACKLIGHT_SetBrightnessLevel(a1->pBacklight, -1);
    IDISPLAY_Backlight(a1->m_pIDisplay, 1);
    return ISHELL_SetTimer(a1->m_pIShell, 3000, (PFNNOTIFY)kphandset::func_28178, a1);
}
bool kphandset::kp_CheckToken(int a1)
{
    return a1 == 32 || a1 == 10 || a1 == 13 || a1 == 9;
}
void kphandset::InitFonts(kphandset* a1)
{
    if (!a1->pTypeface)
    {
        int usingBTFE = 0;
        IFont* Font_Standard15 = 0;
        kpdebug::Print((char*)"Preparing AEE_FONT_NORMAL");
        ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_FONT_STANDARD15, (void**)&Font_Standard15);
        if (Font_Standard15)
            Font_Standard15 = IDISPLAY_SetFont(a1->m_pIDisplay, AEE_FONT_NORMAL, Font_Standard15);
        if (Font_Standard15)
        {
            IFONT_Release(Font_Standard15);
            Font_Standard15 = 0;
        }
        char* ScriptBuf_29130 = kphelpers::ReadScriptBuf("$USE_BTFE");
        if (!ScriptBuf_29130 || STRBEGINS("no", ScriptBuf_29130) == 0)
        {
            a1->pTypeface = 0;
            int Typeface_New = ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_TYPEFACE, (void**)&a1->pTypeface);
            if (Typeface_New)
                kpdebug::AssertLine("Typeface_New returned %lu", Typeface_New);
            if (a1->pTypeface)
            {
                kpdebug::Print((char*)"FONT: BTFE ITypeFace success");
                usingBTFE = 1;
                kpdebug::Print((char*)"FONT: Interface check");
                ITypeface* typefaceInterface = 0;
                int interfaceCheck = ITYPEFACE_QueryInterface(a1->pTypeface, AEEIID_TYPEFACE, (void**)&typefaceInterface);
                kpdebug::AssertLine("FONT: Interface check returned %lu", interfaceCheck);
                if (a1->pTypeface != typefaceInterface)
                    kpdebug::Print((char*)"FONT: Returned interface is not the same!");
                kpdebug::Print((char*)"FONT: Interface check complete");
                int typefaceRef_1 = ITYPEFACE_AddRef(a1->pTypeface);
                kpdebug::AssertLine("FONT: Refcount: %lu", typefaceRef_1);
                int typefaceRef_2 = ITYPEFACE_AddRef(a1->pTypeface);
                kpdebug::AssertLine("FONT: Refcount: %lu", typefaceRef_2);
                int typefaceRef_3 = ITYPEFACE_AddRef(a1->pTypeface);
                kpdebug::AssertLine("FONT: Refcount: %lu", typefaceRef_3);
                if (!a1->pIFont_User1)
                {
                    int Typeface_New_2 = ITYPEFACE_NewFontFromFile(a1->pTypeface, "clicker-cond-medium-sc.ttf", 0, -1, 16, &a1->pIFont_User1);
                    if (Typeface_New_2)
                        kpdebug::AssertLine("ERR: Typeface_New returned %lu", Typeface_New_2);
                    if (a1->pIFont_User1)
                    {
                        IHFONT_HandleEvent(a1->pIFont_User1, 2049, 4100, 255u);
                        Font_Standard15 = IDISPLAY_SetFont(a1->m_pIDisplay, AEE_FONT_USER_1, CAST(IFont*, a1->pIFont_User1));
                        if (Font_Standard15)
                        {
                            IFont_Release(Font_Standard15);
                            Font_Standard15 = 0;
                        }
                    }
                    else
                    {
                        kpdebug::Print((char*)"FONT: Error creating Font15");
                        usingBTFE = 0;
                    }
                }
                if (!a1->pIFont_Large)
                {
                    int font = ITYPEFACE_NewFontFromFile(a1->pTypeface, "clicker-cond-medium-sc.ttf", 0, -1, 19, &a1->pIFont_Large);
                    if (font)
                        kpdebug::AssertLine("ERR: Typeface_New returned %lu", font);
                    if (a1->pIFont_Large)
                    {
                        IHFONT_HandleEvent(a1->pIFont_Large, 2049, 4100, 255u);
                        Font_Standard15 = IDISPLAY_SetFont(a1->m_pIDisplay, AEE_FONT_LARGE, CAST(IFont*, a1->pIFont_Large));
                        if (Font_Standard15)
                        {
                            IFont_Release(Font_Standard15);
                            Font_Standard15 = 0;
                        }
                    }
                    else
                    {
                        kpdebug::Print((char*)"FONT: Error creating Font18");
                        usingBTFE = 0;
                    }
                }
                if (!a1->pIFont_User2)
                {
                    int font = ITYPEFACE_NewFontFromFile(a1->pTypeface, "clicker-cond-medium-sc.ttf", 0, -1, 26, &a1->pIFont_User2);
                    if (font)
                        kpdebug::AssertLine("ERR: Typeface_New returned %lu", font);
                    if (a1->pIFont_User2)
                    {
                        IHFONT_HandleEvent(a1->pIFont_User2, 2049, 4100, 255u);
                        Font_Standard15 = IDISPLAY_SetFont(a1->m_pIDisplay, AEE_FONT_USER_2, CAST(IFont*, a1->pIFont_User2));
                        if (Font_Standard15)
                        {
                            IFont_Release(Font_Standard15);
                            Font_Standard15 = 0;
                        }
                    }
                    else
                    {
                        kpdebug::Print((char*)"FONT: Error creating Font26");
                        usingBTFE = 0;
                    }
                }
                if (!usingBTFE)
                    kpdebug::Print((char*)"FONT: Clicker Font trouble");
            }
            else
            {
                kpdebug::Print((char*)"FONT: NO BTFE ITypeFace");
            }
        }
        if (!usingBTFE)
        {
            kpdebug::Print((char*)"FONT: Defaulting to system fonts");
            ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_FONT_STANDARD18B, (void**)&Font_Standard15);
            if (Font_Standard15)
                Font_Standard15 = IDISPLAY_SetFont(a1->m_pIDisplay, AEE_FONT_LARGE, Font_Standard15);
            if (Font_Standard15)
            {
                IFONT_Release(Font_Standard15);
                Font_Standard15 = 0;
            }
            ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_FONT_STANDARD15B, (void**)&Font_Standard15);
            if (Font_Standard15)
                Font_Standard15 = IDISPLAY_SetFont(a1->m_pIDisplay, AEE_FONT_USER_1, Font_Standard15);
            if (Font_Standard15)
            {
                IFONT_Release(Font_Standard15);
                Font_Standard15 = 0;
            }
            ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_FONT_STANDARD26B, (void**)&Font_Standard15);
            if (Font_Standard15)
                Font_Standard15 = IDISPLAY_SetFont(a1->m_pIDisplay, AEE_FONT_USER_2, Font_Standard15);
            if (Font_Standard15)
            {
                IFONT_Release(Font_Standard15);
                Font_Standard15 = 0;
            }
            IDISPLAY_SetColor(a1->m_pIDisplay, CLR_USER_BACKGROUND, RGBA_WHITE);
        }
    }
}
void kphandset::kpsys_PreloadUIImages(kphandset* pApp)
{
    pApp->ui_help_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_help");
    pApp->ui_ok_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_ok");
    pApp->ui_replay_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_replay");
    pApp->ui_retrigger_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_retrigger");
    pApp->ui_waiting_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_waiting");
}
void kphandset::func_1C9EC(kphandset* a1)
{
    int pdwLargest = 0;
    uint32 pdwTotal = 0;
    uint32 ramFree = GETRAMFREE(&pdwTotal, (uint32*)&pdwLargest);
    unsigned int ramRemainingPercent = (100 * ramFree) / pdwTotal;
    kpdebug::AssertLine("*** RAM REMAINING: %lu pct", ramRemainingPercent);
    //kpdebug_func_2776C((kphandset*)GETAPPINSTANCE(), "MEM: %lu/%lu (%lu)", ramFree, pdwTotal, pdwLargest);
    if (ramRemainingPercent < 7)
    {
        kpdebug::Print((char*)"*** FREE RAM BELOW 7 pct ***");
        //kphandset::kpscr_CheckFilmstripUpTime(a1);
        //kpkhandset::kpmem_PrintMemUsage();
    }
}

void kphandset::Sys_Init(kphandset* pApp)
{
    CALLBACK_Init(&pApp->pSystemCallback, kphandset::func_1C9EC, pApp);
    ISHELL_RegisterSystemCallback(pApp->m_pIShell, &pApp->pSystemCallback, 2);
    IBitmap* bitmap;
    IDISPLAY_GetDeviceBitmap(pApp->m_pIDisplay, &bitmap);
    if (bitmap)
    {
        IBITMAP_GetInfo(bitmap, &pApp->pBitmapInfo, 12);
        if (bitmap)
        {
            IBitmap_Release(bitmap);
            bitmap = 0;
        }
    }
    kpdebug::nullfunc4(pApp, "SYS: Display area %lu x %lu", pApp->pBitmapInfo.cx, pApp->pBitmapInfo.cy); //This looks to be a define because it tried to call GetAppInstance() first
    kpdebug::Print((char*)"SYS: Audio Init");
    kpaudio::InitAudioMgr(pApp);
    kpdebug::Print((char*)"SYS: Script Init");
    kphandset::InitScriptMgr(pApp);
    kpdebug::Print((char*)"SYS: Network Init");
    kpnetwork::InitNetMgr(pApp);
    kpdebug::Print((char*)"SYS: Translator Init");
    kphandset::kp_InitTranslatorMgr_func_27AD8(pApp);
    kpdebug::Print((char*)"SYS: Global Timeout Init");
    kptimeout::InitGlobalTimeoutMgr(pApp);
    pApp->pSelectedStartup = 0;
    pApp->pPop = 0;
    pApp->kphandset_unk98_1[0] = 0;
    pApp->lowBattery = 0;
    pApp->flippedOpen = 1;
    pApp->unk21_3 = 0;
    pApp->adminMode = 0;
    kphandset::CheckLowBattery(pApp);
    MEMSET(&pApp->handsetID, 0, 8);
    char* handsetID = kphelpers::ReadFromAppPath(pApp, (char*)"handsetid.txt", "");
    if (handsetID && *handsetID)
        kphelpers::NullTerminatedString(pApp->handsetID, (const char*)handsetID, 8);
    if (pApp->handsetID[0])
        kpdebug::Assert("Handset ID: %s", (const char*)&pApp->handsetID, 0);
    else
        kpdebug::Print((char*)"Missing handset ID");
    MEMSET(&pApp->contentVersion, 0, 8);
    char* versionBuf = kphelpers::ReadFromRootSD(pApp, "_version", ".txt");
    char* contentVersionBuf = kphelpers::ReadFromAppPath(pApp, versionBuf, "0");
    if (contentVersionBuf && *contentVersionBuf)
        kphelpers::NullTerminatedString((char*)&pApp->contentVersion, (const char*)contentVersionBuf, 8);
    if (!pApp->contentVersion)
        kphelpers::NullTerminatedString((char*)&pApp->contentVersion, "0", 8);
    kpdebug::Assert("Content v%s", (const char*)&pApp->contentVersion, 0);
    char* adminFile = kphelpers::ReadFromAppPath(pApp, (char*)"_admin.txt", 0);
    if (adminFile && *adminFile && !STRCMP(adminFile, (const char*) & pApp->handsetID))
        pApp->adminMode = 1;
    if (pApp->adminMode)
        kpdebug::Print((char*)"ADMIN MODE");
    pApp->lang[0] = 101;
    pApp->lang[1] = 110;
    pApp->episode[0] = 120;
    pApp->episode[1] = 120;
    ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_BACKLIGHT, (void**)&pApp->pBacklight);
    ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_GRAPHICS, (void**)&pApp->pGraphics);
    //kp_nullfunc_5(a1);
    pApp->kphandset_isStartup1 = 0;
    pApp->kphandset_isStartup2 = 0;
    ISHELL_SetTimer(pApp->m_pIShell, 3000, (PFNNOTIFY)kphandset::func_28178, pApp);
    kpdebug::Print((char*)"SYS: Initializing fonts");
    kphandset::InitFonts(pApp);
    kpdebug::Print((char*)"SYS: Fonts initialized");
    kphandset::kpsys_PreloadUIImages(pApp);
    pApp->timeSeconds = GETTIMESECONDS();
    if (pApp->adminMode)
    {
        kpscreen* screen = kpadmin::ExecuteCommand(pApp);
        kpscreen::FetchSelectedScreen(pApp, screen);
        FileInfo info;
        if (IFILEMGR_GetInfo(pApp->pFileMgr, "fs:/card0/jj", &info) != 1)
            ISHELL_SetTimer(pApp->m_pIShell, 10, (PFNNOTIFY)kphandset::kpsys_PickRandomSong, pApp);
    }
    else
    {
        kpscreen* screen = (kpscreen*)kpclient::ExecuteCommand(pApp);
        kpscreen::FetchSelectedScreen(pApp, screen);
    }
    ISHELL_RegisterNotify(pApp->m_pIShell, AEECLSID_KPHANDSET, AEECLSID_PHONENOTIFIER, AEET_NMASK_NEW_CALLDESC);
    ISHELL_RegisterNotify(pApp->m_pIShell, AEECLSID_KPHANDSET, AEECLSID_SMSNOTIFIER, NMASK_SMS_BROADCAST);
}
bool kphandset::SDCheck(kphandset* pApp)
{
    bool sdCheck = 0;
    if (!pApp->pFileMgr)
        ISHELL_CreateInstance(pApp->m_pIShell, AEECLSID_FILEMGR, (void**)&pApp->pFileMgr);
    FileInfo fileInfo;
    if (pApp->pFileMgr && !IFILEMGR_GetInfo(pApp->pFileMgr, "fs:/card0/ui_waiting.png", &fileInfo))
        sdCheck = 1;
    if (sdCheck)
    {
        ISHELL_CancelTimer(pApp->m_pIShell, (PFNNOTIFY)kphandset::SDCheck, pApp);
        kphandset::Sys_Init(pApp);
    }
    else
    {
        kphandset::DisplayMessageBox(pApp, "Please make sure the SD Card is fully inserted.");
        ISHELL_SetTimer(pApp->m_pIShell, 5000, (PFNNOTIFY)kphandset::SDCheck, pApp);
    }
    return sdCheck;
}

void kphandset::kpscr_func_1DDAC(kpscreen* a1)
{
    if (!a1->kpstartup_unk11_1)
    {
        a1->kpstartup_unk11_1 = 1;
        ISHELL_SetTimer(((kphandset*)GETAPPINSTANCE())->m_pIShell, 20, (PFNNOTIFY)kphandset::kp_func_2A334, a1);
    }
}

void kphandset::kp_func_2A334(kpscreen* a1)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    ISHELL_CancelTimer(instance->m_pIShell, 0, a1);
    ISHELL_PostEvent(instance->m_pIShell, AEECLSID_KPHANDSET, 28692u, 0, 0);
}

void kphandset::kpscript_Free(kphandset* pApp)
{
    kphandset::kpsys_ClearScript(pApp);
    if (pApp->script.constants)
    {
        FREE(pApp->script.constants);
        pApp->script.constants = 0;
    }
}




void* kphandset::FetchScreen(kphandset* pApp)
{
    void* p_kpstartup2;
    if (pApp->kphandset_isStartup1)
    {
        if (!pApp->kphandset_isStartup2)
        {
            p_kpstartup2 = pApp->kpstartup2;
            pApp->kphandset_isStartup2 = 1;
        }
    }
    else
    {
        p_kpstartup2 = pApp->kpstartup1;
        pApp->kphandset_isStartup1 = 1;
    }
    if (p_kpstartup2)
        MEMSET(p_kpstartup2, 0, 4564);
    return p_kpstartup2;
}

void kphandset::StartingApp(kphandset* pApp)
{
    kpdebug::Print((char*)"App Starting");
    kpdebug::Print((char*)"Software Version: 1004-22");
    if (kphandset::SDCheck(pApp))
    {
        if (!pApp->pSelectedStartup)
        {
            kpscreen* screen = (kpscreen*)kpclient::ExecuteCommand(pApp);
            kpscreen::FetchSelectedScreen(pApp, screen);
        }
    }
}

void kphandset::kpsys_SetLanguage(kphandset* pApp, const char* language)
{
    if (language && *language)
    {
        if (language[1])
        {
            kpdebug::Assert("SYS: Setting language '%s'", language, 0);
            pApp->lang[0] = *language;
            pApp->lang[1] = language[1];
            char* StringTranslationPath = kphelpers::CreateStringTranslationPath(pApp, (char*)"strings.txt", 0);
            kphandset::LoadStringTranslation(pApp, StringTranslationPath);
        }
    }
}

void kphandset::kpsys_SetPavilion(kphandset* pApp, const char* pavilion)
{
    if (pavilion && *pavilion && pavilion[1])
    {
        kpdebug::Assert("SYS: Setting pavilion '%s'", pavilion, 0);
        if (pApp->adminMode)
        {
            kpdebug::Print((char*)"SYS: Killing admin mode");
            pApp->adminMode = 0;
        }
        pApp->episode[0] = *pavilion;
        pApp->episode[1] = pavilion[1];
        char* StringTranslationPath = kphelpers::CreateStringTranslationPath(pApp, (char*)"strings.txt", 0);
        kphandset::LoadStringTranslation(pApp, StringTranslationPath);
        if (pApp->ui_full_Interface)
        {
            IIMAGE_Release(pApp->ui_full_Interface);
            pApp->ui_full_Interface = 0;
        }
        pApp->ui_full_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_full");
        if (pApp->ui_bottom_Interface)
        {
            IIMAGE_Release(pApp->ui_bottom_Interface);
            pApp->ui_bottom_Interface = 0;
        }
        pApp->ui_bottom_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_bottom");
        if (pApp->ui_bottom_clear_Interface)
        {
            IIMAGE_Release(pApp->ui_bottom_clear_Interface);
            pApp->ui_bottom_clear_Interface = 0;
        }
        pApp->ui_bottom_clear_Interface = kphelpers::LoadUIImages(pApp, (char*)"ui_bottom_clear");
        kphandset::kpsys_ClearScript(pApp);
    }
}

void kphandset::kpsys_LoadScript(kphandset* pApp, char* script)
{
    kpdebug::Assert("SYS: Loading script '%s'", script, 0);
    kphandset::kpsys_ClearScript(pApp);
    const char* scriptBuf = (const char*)kphelpers::ReadToScratch(pApp, script, (char*)".txt");
    if (scriptBuf && *scriptBuf || !STRBEGINS("_bonus", script + 2))
    {
        if ((!scriptBuf || !*scriptBuf) && STRBEGINS("_cast", script + 2))
        {
            scriptBuf = (const char*)kphelpers::ReadToScratch(pApp, pApp->episode, (char*)"_cast.txt");
        }
    }
    else
    {
        scriptBuf = (const char*)kphelpers::ReadToScratch(pApp, pApp->episode, (char*)"_bonus.txt");
    }
    if (scriptBuf && *scriptBuf)
    {
        unsigned int outSize[8];
        outSize[0] = 4096;
        if (kphelpers::LoadStringFromFile(pApp, scriptBuf, pApp->script.pLabel, outSize))
            kphelpers::NullTerminatedString((char*)&pApp->script, script, 32);
    }
}

void kphandset::kpsys_ClearScript(kphandset* pApp)
{
    pApp->script.conditional = 0;
    pApp->script.pLabel[0] = 0;
    pApp->script.kpscript_unk20 = 0;
    pApp->script.tokenizer = 0;
    pApp->script.returnLocation = 0;
    pApp->script.replayBuffer = 0;
}

void kphandset::kpsys_ReadScript(kphandset* pApp, const char* script)
{
    if(kphelpers::FetchGoToToken(pApp, script))
        kphandset::kpscr_ParseScript(pApp);
}

void kphandset::kpscr_ParseScript(kphandset* pApp)
{
    kpscreen* screen = 0;

    while (pApp->script.pLabel[0] && pApp->script.tokenizer && *pApp->script.tokenizer && *pApp->script.tokenizer != 58 && !screen)
    {
        char* tokenizer = pApp->script.tokenizer;
        kphelpers::ParseTokenizer(pApp->kphandset_unk99, tokenizer);
        int symbol = 0;
        if (*tokenizer == 35 || *tokenizer == 45)
        {
            symbol = 1;
        }
        else if (*tokenizer == 63 || *tokenizer == 33)
        {
            const char* commandBuf = kphandset::kpscr_GetCommand(pApp->kphandset_unk99);
            char* conditional = kphelpers::ReadScriptBuf(commandBuf + 1);
            if (STRBEGINS("lang:", conditional))
            {
                symbol = 1;
                bool isLang = conditional[5] == pApp->lang[0] && conditional[6] == pApp->lang[1];
                if (*tokenizer == 63 && isLang)
                {
                    symbol = 0;
                }
                else if (*tokenizer == 33 && !isLang)
                {
                    symbol = 0;
                }
            }
            else if ((unsigned char)*conditional < 49u || (unsigned char)*conditional > 57u)
            {
                kpdebug::Print((char*)"SCR: UNRECOGNIZED CONDITIONAL - SKIPPING");
                symbol = 1;
            }
            else
            {
                symbol = 1;
                int conditionalVal = (unsigned char)*conditional - 49;
                if (*tokenizer == 63 && pApp->script.kpscript_unk21[conditionalVal + 399])
                {
                    symbol = 0;
                }
                else if (*tokenizer == 33 && !pApp->script.kpscript_unk21[conditionalVal + 399])
                {
                    symbol = 0;
                }
            }
        }
        if (!symbol && pApp != (kphandset*)-6424 && !kphandset::kpscr_func_310B8(pApp->kphandset_unk99))
        {
            const char* command = kphandset::kpscr_GetCommand(pApp->kphandset_unk99);
            if (STRCMP("cutscene", command) == 0)
            {
                if (pApp->flippedOpen)
                    screen = kpcutscene::ExecuteCommand(pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("still", command) == 0)
            {
                screen = kpstill::ExecuteCommand(pApp, pApp->kphandset_unk99, (char*)"ui_full");
            }
            /*else if (STRCMP("webstill", command) == 0)
            {
                screen = kpscr_Command_Webstill(pApp, pApp->kphandset_unk99, "ui_full");
            }*/
            else if (STRCMP("enter", command) == 0)
            {
                //screen = kphandset::kpscr_Command_Enter(pApp, pApp->kphandset_unk99);
            }
            /*else if (STRCMP("bonus", command) == 0)
            {
                screen = kpscr_Command_Bonus(pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("camera", command) == 0)
            {
                screen = kpscr_Command_Camera(pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("target", command) == 0)
            {
                screen = kpscr_Command_Target(pApp, pApp->kphandset_unk99);
            }*/
            else if (STRCMP("wait", command) == 0)
            {
                screen = kpwait::ExecuteCommand(pApp, pApp->kphandset_unk99);
            }
            /*else if (STRCMP("missionmenu", command) == 0)
            {
                screen = kpscr_Command_MissionMenu(pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("endgame", command) == 0)
            {
                kptimeout::ClearTimeout(pApp);
                screen = kpscr_Command_EndGame(pApp, "s_timeout_two", pApp->global_alert_count, (BOOL(*)(kpstartup*, int, int, int))((char*)AEEMod_Load + 2));
                pApp->kphandset_unk163 = 0;
                pApp->globalTimeoutState = 100;
            }
            else if (STRCMP("zodiac_trigger", command) == 0)
            {
                kpscr_Command_ZodiacTrigger((int)pApp);
            }
            else if (STRCMP("zodiac", command) == 0)
            {
                screen = kpscr_Command_Zodiac(pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("audio", command) == 0)
            {
                kpscr_Command_Audio(pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("extract", command) == 0)
            {
                extractBuf = kpscr_GetCommand(pApp->kphandset_unk99);
                kpscr_Command_Extract((int)pApp, extractBuf);
            }
            else if (STRCMP("ifextract", command) == 0)
            {
                kpscr_Command_Ifextract(pApp, pApp->kphandset_unk99);
            }*/
            else if (STRCMP("goto", command) == 0)
            {
                const char* gotoBuf = kphandset::kpscr_GetCommand(pApp->kphandset_unk99);
                kphandset::kpscr_Command_Goto(pApp, gotoBuf);
            }
            /*else if (STRCMP("ignore", command) == 0)
            {
                ignoreBuf = kpscr_GetCommand(pApp->kphandset_unk99);
                kpscr_Command_Ignore((char*)pApp, ignoreBuf);
            }*/
            else if (STRCMP("message", command) == 0)
            {
                kphandset::kpscr_Command_Message(pApp, pApp->kphandset_unk99);
            }
            /*else if (STRCMP("prepdate", command) == 0)
            {
                kpscr_Command_PrepDate(pApp);
            }*/
            else if (STRCMP("preload", command) == 0)
            {
                char* preloadBuf = (char*)kphandset::kpscr_GetCommand(pApp->kphandset_unk99);
                kphandset::kpscr_Command_Preload(pApp, preloadBuf);
            }
            else if (STRCMP("vibrate", command) == 0)
            {
                kphandset::kpscr_Command_Vibrate(pApp, pApp->kphandset_unk99);
            }
            /*else if (STRCMP("randomize", command) == 0)
            {
                kpscr_Command_Randomize((int)pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("setentry", command) == 0)
            {
                kpscr_Command_SetEntry((int)pApp, pApp->kphandset_unk99);
            }
            else if (STRCMP("clear", command) == 0)
            {
                MEMSET(pApp->kphandset_unk150, 0, 9);
            }*/
            else if (STRCMP("mark", command) == 0)
            {
                kphandset::kpscr_Command_Mark(pApp);
            }
            else if (STRCMP("reboot", command) == 0)
            {
                kphandset::kpscr_Command_Reboot(pApp);
            }
            else if (STRCMP("return", command) == 0)
            {
                kphandset::kpscr_Command_Return(pApp);
            }
            else if (STRCMP("set", command) == 0)
            {
                char* setBuf = (char*)kphandset::kpscr_GetCommand(pApp->kphandset_unk99);
                kphandset::kpscr_Command_SetReset(pApp, setBuf, 1);
            }
            else if (STRCMP("reset", command) == 0)
            {
                char* resetBuf = (char*)kphandset::kpscr_GetCommand(pApp->kphandset_unk99);
                kphandset::kpscr_Command_SetReset(pApp, resetBuf, 0);
            }
            else
            {
                kpdebug::Assert("SCR: UNRECOGNIZED COMMAND '%s'", command, 0);
            }
        }
        char* mainToken = kphelpers::FetchMainToken(pApp->script.tokenizer);
        pApp->script.tokenizer = kphelpers::FetchSubToken(mainToken);
    }
    if (screen)
        kpscreen::FetchSelectedScreen(pApp, screen);
}

bool kphandset::kpscr_Command_Goto(kphandset* pApp, const char* script)
{
    char* label = kphelpers::ReadScriptBuf(script);
    if (*label != 58 && *label != 45)
    {
        kphelpers::NullTerminatedString(&pApp->scratch[1], label, 399);
        pApp->scratch[0] = 58;
        label = pApp->scratch;
    }
    kpdebug::Assert("SCR:Goto %s", label, 0);
    char* pLabel = pApp->script.pLabel;
    if (pApp != (kphandset*)-1460)
        pLabel = STRSTR(pApp->script.pLabel, label);
    while (pLabel && *pLabel && pLabel != pApp->script.pLabel && *(pLabel - 1) != 10)
        pLabel = STRSTR(pLabel + 1, label);
    if (pLabel)
    {
        pApp->script.tokenizer = pLabel;
        return 1;
    }
    else
    {
        kpdebug::Assert("SCR: Label not found %s", label, 0);
    }
    return 0;
}

const char* kphandset::kpscr_GetCommand(char* tokenizer)
{
    char* commandBuf = (char*)*((int*)tokenizer + 48);
    if (commandBuf)
    {
        while (**((uint8**)tokenizer + 48) && !kphelpers::CheckToken(**((unsigned char**)tokenizer + 48)))
            ++*((int*)tokenizer + 48);
        if (**((uint8**)tokenizer + 48))
        {
            *(uint8*)(*((int*)tokenizer + 48))++ = 0;
            while (**((uint8**)tokenizer + 48) && kphelpers::CheckToken(**((unsigned char**)tokenizer + 48)))
                ++*((int*)tokenizer + 48);
        }
    }
    return commandBuf;
}

bool kphandset::kpscr_func_310B8(char* a1)
{
    return **((uint8**)a1 + 48) == 0;
}

void kphandset::kpsys_PickRandomSong(kphandset* pApp)
{
    unsigned int song;
    GETRAND((byte*)&song, 1);
    song /= 7;
    if (song)
    {
        switch (song)
        {
        case 1:
            kpaudio::LoadAudioFile("jj/song2");
            break;
        case 2:
            kpaudio::LoadAudioFile("jj/song3");
            break;
        case 3:
            kpaudio::LoadAudioFile("jj/song4");
            break;
        case 4:
            kpaudio::LoadAudioFile("jj/song5");
            break;
        case 5:
            kpaudio::LoadAudioFile("jj/song6");
            break;
        case 6:
            kpaudio::LoadAudioFile("jj/song7");
            break;
        }
    }
    else
    {
        kpaudio::LoadAudioFile("jj/song1");
    }
}

void kphandset::kpscr_func_2AB8C(kphandset* pApp)
{
    pApp->global_timeout_remaining = pApp->global_one;
    pApp->globalTimeoutState = 1;
    ISHELL_SetTimer(pApp->m_pIShell, 1000, (PFNNOTIFY)kptimeout::CheckTimeout, pApp);
}

void kphandset::kpscr_func_23C18(kphandset* pApp)
{
    if (pApp->script.replayBuffer)
    {
        pApp->script.tokenizer = pApp->script.replayBuffer;
        kphandset::kpscr_func_1DDAC(pApp->pSelectedStartup);
    }
}

void kphandset::kpscr_RenderSubtitles(kpBottom* a1)
{
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    if (a1->interface)
        IIMAGE_Draw(a1->interface, 0, a1->bottomBarHeight);
    int subtitleSplitIndex = a1->subtitleSplitIndex;
    if (subtitleSplitIndex > 3)
        subtitleSplitIndex = 3;
    if (a1->subtitleSplitIndex - a1->kpBottom_unk11_1 < subtitleSplitIndex && a1->kpBottom_unk11_1 < (int)a1->subtitleSplitIndex)
    {
        subtitleSplitIndex = (unsigned short)(a1->subtitleSplitIndex - a1->kpBottom_unk11_1);
    }
    if (a1->kpBottom_unk11_1 >= (int)a1->subtitleSplitIndex)
        subtitleSplitIndex = 0;
    if (subtitleSplitIndex > 0)
    {
        int isSplitOne = subtitleSplitIndex == 1;
        signed int index = 0;
        while (index < subtitleSplitIndex)
        {
            int bottomIndex = (unsigned char)(a1->kpBottom_unk11_1 + index);
            int nl = a1->kpBottom_unk10[bottomIndex + 1] - a1->kpBottom_unk10[bottomIndex];
            AEERect rect;
            rect.x = (short)(instance->pBitmapInfo.cx - a1->SubtitleWidth[isSplitOne]) / 2;
            rect.y = a1->SubtitleHeight[isSplitOne];
            rect.dx = a1->SubtitleWidth[isSplitOne];
            rect.dy = instance->pBitmapInfo.cy - a1->SubtitleHeight[isSplitOne];
            IDISPLAY_DrawText(instance->m_pIDisplay, a1->textFont, &a1->kpBottom_unk9[a1->kpBottom_unk10[bottomIndex]], nl, rect.x, rect.y, &rect, 0x8020u);
            index = (index + 1) & 0xFFFEFFFF;
            isSplitOne = (unsigned char)(isSplitOne + 1);
        }
    }
}
void kphandset::kpscr_InitBottomBar(kpBottom* a1, const char* a2, const char* a3)
{
    AEEImageInfo imageInfo;
    //*(int*)&imageInfo.cx = a2;
    //*(int*)&imageInfo.nColors = a3;
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    a1->subtitleSplitIndex = 0;
    a1->kpBottom_unk11_1 = 0;
    a1->kpBottom_unk9[0] = 0;
    a1->textFont = -32762;
    if (!a3 || STRCMP("ui_bottom_clear", a3))
    {
        a1->textFont = -32762;
        a1->SubtitleHeight[0] = 0;
        a1->SubtitleHeight[1] = 14;
        a1->SubtitleHeight[2] = 28;
        a1->SubtitleWidth[0] = -86;
        a1->SubtitleWidth[1] = -86;
        a1->SubtitleWidth[2] = -86;
        a1->interface = instance->ui_bottom_Interface;
    }
    else
    {
        a1->textFont = -32766;
        a1->SubtitleHeight[0] = 3;
        a1->SubtitleHeight[1] = 21;
        a1->SubtitleHeight[2] = 39;
        a1->SubtitleWidth[0] = -86;
        a1->SubtitleWidth[1] = -86;
        a1->SubtitleWidth[2] = -86;
        a1->interface = (IImage*)instance->ui_bottom_clear_Interface;
    }
    char cy = 59;
    if (a1->interface)
    {
        IIMAGE_GetInfo(a1->interface, &imageInfo);
        cy = imageInfo.cy;
    }
    a1->bottomBarHeight = instance->pBitmapInfo.cy - cy;
    for (int i = 0; i < 3; ++i)
    {
        a1->SubtitleHeight[i] = a1->SubtitleHeight[i] + a1->bottomBarHeight - 3;
    }
    if (a2)
        kphandset::kpscr_subtitles_func_305F4(a1, a2);
}

void kphandset::kpscr_subtitles_func_305F4(kpBottom* a1, const char* a2)
{
    const char* stringBuf = a2;
    int strLength = STRLEN(a2);
    if (strLength >= 512)
    {
        stringBuf = "ERROR: STRING TOO LONG!";
        strLength = STRLEN("ERROR: STRING TOO LONG!");
    }
    STRTOWSTR(stringBuf, a1->kpBottom_unk9, 2 * (strLength + 1));
    AECHAR* kpBottom_unk9 = a1->kpBottom_unk9;
    a1->subtitleSplitIndex = 0;
    kphandset* instance = (kphandset*)GETAPPINSTANCE();
    while (*kpBottom_unk9 && a1->subtitleSplitIndex < 34)
    {
        int widthIndex = (a1->subtitleSplitIndex / 3);
        int width = a1->SubtitleWidth[widthIndex];
        a1->kpBottom_unk10[a1->subtitleSplitIndex++] = kpBottom_unk9 - a1->kpBottom_unk9;
        int nChars;
        IDISPLAY_MeasureTextEx(instance->m_pIDisplay, (AEEFont)a1->textFont, kpBottom_unk9, -1, width, &nChars);
        AECHAR* i;
        for (i = &kpBottom_unk9[nChars]; *i && i > kpBottom_unk9 && !kphandset::kpscr_Tokenizer(*i); --i)
            ;
        if (i == kpBottom_unk9)
        {
            i = &kpBottom_unk9[nChars];
        }
        else
        {
            while (kphandset::kpscr_Tokenizer(*i))
                ++i;
        }
        kpBottom_unk9 = i;
    }
    a1->kpBottom_unk10[a1->subtitleSplitIndex] = kpBottom_unk9 - a1->kpBottom_unk9;
}
bool kphandset::kpscr_Tokenizer(int a1)
{
    return a1 == 32 || a1 == 10 || a1 == 13 || a1 == 9;
}
void kphandset::kpscr_Command_Preload(kphandset* pApp, char* pFilmstrip)
{
    IImage* image = kphelpers::PreloadFilmstrip(pApp, pFilmstrip);
    if (image)
        IIMAGE_Release(image);
    else
        kpdebug::Assert("SCR: Preload Failed: %s", pFilmstrip, 0);
}
void kphandset::kpscr_Command_Message(kphandset* a1, char* a2)
{
    const char* Command = kphandset::kpscr_GetCommand(a2);
    char* ScriptBuf = kphelpers::ReadScriptBuf(Command);
    if (ScriptBuf)
        kpnetwork::CreateOutgoingMessage(a1, ScriptBuf, 1);
}
void kphandset::kpscr_Command_Vibrate(kphandset* a1, char* a2)
{
    int vibrateIntensity = 500;
    const char* Command = kphandset::kpscr_GetCommand(a2);
    char* ScriptBuf = kphelpers::ReadScriptBuf(Command);
    if (ScriptBuf)
        vibrateIntensity = ATOI(ScriptBuf);
    if (vibrateIntensity > 0)
    {
        kpdebug::AssertLine("SCR: Vibrate: ", vibrateIntensity);
        ISound* sound = 0;
        if (!ISHELL_CreateInstance(a1->m_pIShell, AEECLSID_SOUND, (void**)&sound))
            ISOUND_Vibrate(sound, vibrateIntensity);
        if (sound)
            ISOUND_Release(sound);
    }
}
void kphandset::kpscr_Command_Mark(kphandset* pApp)
{
    pApp->script.returnLocation = pApp->script.tokenizer;
}
void kphandset::kpscr_Command_Reboot(kphandset* pApp)
{
    ISHELL_PostEvent(pApp->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_RESTART, 0, 0);
}
void kphandset::kpscr_Command_Return(kphandset* pApp)
{
    if (pApp->script.returnLocation)
        pApp->script.tokenizer = pApp->script.returnLocation;
    else
        kpdebug::Print("SCR: Return to invalid return location");
}
void kphandset::kpscr_Command_SetReset(kphandset* pApp, char* a2, char isSet)
{
    char* ScriptBuf = a2;
    if (a2 && *a2)
        ScriptBuf = kphelpers::ReadScriptBuf(a2);
    if (ScriptBuf && *ScriptBuf)
    {
        int scriptVal = ATOI(ScriptBuf);
        if (scriptVal > 9)
            kpdebug::AssertLine("SCR: Invalid flag index: ", scriptVal);
        else
            pApp->script.kpscript_unk21[scriptVal + 398] = isSet;
    }
}
//

void kphandset::kp_InitTranslatorMgr_func_27AD8(kphandset* pApp)
{
    MEMSET(&pApp->pTranslatorMgr, 0, 8);
}

void kphandset::LoadStringTranslation(kphandset* pApp, const char* path)
{
    unsigned int outSize = 0;
    char* stringBuf = kphelpers::LoadStringFromFile(pApp, path, 0, &outSize);
    if (stringBuf)
    {
        kphandset::kptranslator_Free(pApp);
        pApp->pTranslatorMgr.buffer = stringBuf;
        pApp->pTranslatorMgr.length = outSize;
        for (unsigned int i = 0; i < outSize; ++i)
        {
            if (*stringBuf == 10 || *stringBuf == 13)
                *stringBuf = 0;
            ++stringBuf;
        }
    }
    else
    {
        kpdebug::DBGPrintF(GETAPPINSTANCE(), (char*)"SYS: STRING TRANSLATION LOAD FAILED!");
    }
}

void kphandset::kptranslator_Free(kphandset* pApp)
{
    if (pApp->pTranslatorMgr.buffer)
    {
        FREE(pApp->pTranslatorMgr.buffer);
        pApp->pTranslatorMgr.buffer = 0;
    }
    kphandset::kp_InitTranslatorMgr_func_27AD8(pApp);
}

char* kphandset::kphandset_ReadFromScratch(const char* haystack, const char* needle)
{
    char* scratch = ((kphandset*)GETAPPINSTANCE())->scratch;
    *scratch = 0;
    char* scratchBuf = STRISTR(haystack, needle);
    if (scratchBuf)
    {
        const char* scratchVal = STRCHREND(&scratchBuf[STRLEN(needle)], 61);
        int key = 39;
        char* end = STRCHREND(scratchVal, 39);
        if (!*end)
        {
            key = 34;
            end = STRCHREND(scratchVal, 34);
        }
        if (*end)
        {
            const char* terminated = end + 1;
            int index = STRCHREND(terminated, key) - (char*)terminated;
            if (index > 0 && index < 399)
            {
                STRNCPY(scratch, terminated, index);
                scratch[index] = 0;
            }
        }
    }
    return scratch;
}
//

/*===========================================================================

FUNCTION kphandset_HandleEvent
  
DESCRIPTION
   This is the EventHandler for this app. All events to this app are handled in this
   function. All APPs must supply an Event Handler.  

   Note - The switch statement in the routine is to demonstrate how event handlers are 
   generally structured.  However, realizing the simplicity of the example, the code could
   have been reduced as follows:

   if(eCode == EVT_APP_START){
      IDISPLAY_DrawText();
      IDISPLAY_Update();
      return(TRUE);
   }
   return(FALSE);

   However, while doing so would have demonstrated how BREW apps can be written in about 8
   lines of code (including the app creation function), it might have confused those who wanted 
   a bit more practical example.

   Also note that the use of "szText" below is provided only for demonstration purposes.  As 
   indicated in the documentation, a more practical approach is to load text resources
   from the applicaton's resource file.

   Finally, note that the ONLY event that an applet must process is EVT_APP_START.  Failure
   to return TRUE from this event will indicate that the app cannot be started and BREW
   will close the applet.
    
PROTOTYPE:
   boolean kphandset_HandleEvent(IApplet * pi, AEEEvent eCode, uint16 wParam, uint32 dwParam)
      
PARAMETERS:
   pi: Pointer to the AEEApplet structure. This structure contains information specific
   to this applet. It was initialized during the AppCreateInstance() function.
        
   ecode: Specifies the Event sent to this applet
          
   wParam, dwParam: Event specific data.
            
DEPENDENCIES
   none
              
RETURN VALUE
   TRUE: If the app has processed the event
   FALSE: If the app did not process the event
                
SIDE EFFECTS
   none
===========================================================================*/
static boolean kphandset_HandleEvent(AEEApplet* pMe, AEEEvent eCode, uint16 wParam, uint32 dwParam)
{
    kphandset* pApplet = (kphandset*)pMe;
    ASSERT(!pApplet, "pApplet");            //Line 855
    int returnCode = 0;
    if (pApplet->pSelectedStartup)
        returnCode = pApplet->pSelectedStartup->HandleEventPtr(pApplet->pSelectedStartup, eCode, wParam, dwParam);
    if (eCode == EVT_BUSY) //1028
    {
        return 1;
    }
    if (eCode <= EVT_BUSY) //1028
    {
        if (eCode == EVT_APP_MESSAGE) //8
        {
            return 1;
        }
        else if (eCode > EVT_APP_MESSAGE) //8
        {
            switch (eCode)
            {
                case EVT_APP_TERMINATE: //10
                    kpdebug::Print((char*)"App Terminating");
                break;
                case EVT_KEY: //256
                    if (pApplet->adminMode && wParam == AVK_STAR)
                        kpaudio::StopMedia(&pApplet->pAudio);
                    if (wParam >= AVK_0 && wParam <= AVK_CLR)
                    {
                        MEMMOVE(&pApplet->kphandset_unk148[1], pApplet->kphandset_unk148, 7);
                        pApplet->kphandset_unk148[7] = 0;
                        pApplet->kphandset_unk148[0] = wParam + 32;
                    }
                    if (wParam == AVK_END)
                    {
                        int close = 0;
                        char* ScriptBuf_29130 = kphelpers::ReadScriptBuf("$NHIEIH");
                        if (!ScriptBuf_29130 || !*ScriptBuf_29130 || *ScriptBuf_29130 == 36)
                            ScriptBuf_29130 = (char*)"NHIEIH";
                        if (!STRNCMP((const char*)&pApplet->kphandset_unk148, ScriptBuf_29130, 6))
                            close = 1;
                        if (close)
                        {
                            ISHELL_CloseApplet(pApplet->m_pIShell, 0);
                        }
                        else if (!STRNCMP((const char*)&pApplet->kphandset_unk148, "NIDHDH", 6))
                        {
                            ISHELL_PostEvent(pApplet->m_pIShell, AEECLSID_KPHANDSET, EVT_KPHANDSET_APP_RESTART, 0, 0);
                        }
                        if (!STRNCMP((const char*)&pApplet->kphandset_unk148, "NGECHD", 6))
                            pApplet->unk21_3 = 1;
                    }
                    returnCode = 1;
                break;
                case EVT_NOTIFY: //1025
                    int startApplet = 0;
                    if (dwParam)
                    {
                        if (((AEENotify*)dwParam)->cls == AEECLSID_TAPI && (((AEENotify*)dwParam)->dwMask & NMASK_TAPI_STATUS) == NMASK_TAPI_STATUS)
                        {
                            void* pData = ((AEENotify*)dwParam)->pData;
                            if (pData)
                            {
                                if (((AEESMSMsg*)pData)->nMessages == 1)
                                {
                                    ISHELL_RegisterNotify(pApplet->m_pIShell, AEECLSID_KPHANDSET, AEECLSID_TAPI, 0);
                                    returnCode = 1;
                                    ((AEENotify*)dwParam)->st = 1;
                                    startApplet = 1;
                                }
                            }
                        }
                    }
                    if (dwParam)
                    {
                        if (((AEENotify*)dwParam)->cls == AEECLSID_PHONENOTIFIER && (((AEENotify*)dwParam)->dwMask & AEET_NMASK_NEW_CALLDESC) == AEET_NMASK_NEW_CALLDESC)
                        {
                            AEETNotifyInfo* pData = (AEETNotifyInfo*)((AEENotify*)dwParam)->pData;
                            if (pData)
                            {
                                if (pData->event == AEET_EVENT_CALL_INCOM)
                                {
                                    kpdebug::Print((char*)"*** INCOMING CALL");
                                    ICallMgr* callMgr = 0;
                                    if (!ISHELL_CreateInstance(pApplet->m_pIShell, AEECLSID_CALLMGR, (void**)&callMgr))
                                    {
                                        ICall* Call = 0;
                                        ICALLMGR_GetCall(callMgr, pData->event_data.call.cd, &Call);
                                        if (Call)
                                            ICALL_End(Call);
                                        if (callMgr)
                                        {
                                            ICALLMGR_Release(callMgr);
                                            callMgr = 0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (dwParam && dwParam == AEECLSID_SMSNOTIFIER && (((AEENotify*)dwParam)->dwMask & AEET_NMASK_DATA_CALL) == AEET_NMASK_DATA_CALL)
                        kpdebug::Print((char*)"*** INCOMING SMS");
                    uint32 pdwTotal, pdwFree;
                    FileInfo pInfo;
                    if (!IFILEMGR_GetFreeSpaceEx(pApplet->pFileMgr,"fs:/card0/",&pdwTotal,&pdwFree) && startApplet && (!pApplet->pFileMgr || IFILEMGR_GetInfo(pApplet->pFileMgr, "fs:/card0/noautorun.txt", &pInfo) != 1))
                    {
                        startApplet = 0;
                    }
                    if (startApplet)
                        ISHELL_StartApplet(pApplet->m_pIShell, AEECLSID_KPHANDSET);
                break;
            }
        }
        else if (eCode)
        {
            switch (eCode)
            {
                case EVT_APP_STOP: //1
                    kpdebug::Print((char*)"App Stopping");
                break;
                case EVT_APP_SUSPEND: //2
                    ((AEESuspendInfo*)dwParam)->bCloseDialogs = 0;
                    returnCode = 1;
                break;
                case EVT_APP_RESUME: //3
                    returnCode = 1;
                break;
            }
        }
        else
        {
            if (!kphandset::Init(pApplet))
            {
                kpdebug::Print((char*)"App Starting on delay");
                ISHELL_SetTimer(pApplet->m_pIShell, 500, (PFNNOTIFY)kphandset::StartingApp, pApplet);
            }
            return 1;
        }
        return returnCode;
    }
    if (eCode == EVT_KPHANDSET_APP_NETWORK_INCOMING_MESSAGE) //Custom Defined Event Code
    {
        kpnetwork::LogIncomingTraffic(pApplet, pApplet->pPop);
        pApplet->pPop = (NetworkMessage*)dwParam;
        if ( !returnCode )
          kpnetwork::ParseIncomingMessage(pApplet, (const char*)(dwParam + 2));
        returnCode = 1;
        ASSERT(kpnetwork::IsMessageReady(pApplet) != (NetworkMessage*)dwParam, "pPop == (NetworkMessage*)dwParam");
    }
    else
    {
        if (eCode - EVT_APP_NO_CLOSE <= 27658)
        {
            if (eCode != EVT_APP_NO_SLEEP) //1029
            {
                if (eCode == EVT_FLIP) //1280
                {
                    //nullfunc_5();
                    if (wParam == 1)
                    {
                        kpdebug::Print((char*)"FLIP open");
                        pApplet->flippedOpen = 1;
                        ISHELL_SetTimer(pApplet->m_pIShell, 3000, (PFNNOTIFY)kphandset::func_28178, pApplet);
                    }
                    else
                    {
                        kpdebug::Print((char*)"FLIP closed");
                        pApplet->flippedOpen = 0;
                        ISHELL_CancelTimer(pApplet->m_pIShell, (PFNNOTIFY)kphandset::func_28178, pApplet);
                    }
                }
                else if (eCode != EVT_KEYGUARD) //1282
                {
                    return returnCode;
                }
                return 1;
            }
            return 1;
        }
        switch (eCode)
        {
            case EVT_KPHANDSET_APP_LOAD_SCRIPT: //28692
                kphandset::kpscr_ParseScript(pApplet);
                returnCode = 1;
            break;
            case EVT_KPHANDSET_APP_RESTART: //28713
                kpdebug::Print((char*)"App Restart");
                kphandset::FreeAppData(pApplet);
                if (!kphandset::Init(pApplet))
                    kphandset::SDCheck(pApplet);
            break;
            case EVT_KPHANDSET_APP_TIMEOUT: //28716
                kptimeout::HandleTimeout(pApplet);
            break;
        }
    }
    return returnCode;
}


