// winmain.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Windows front end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cassert>
#include <process.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include <mmsystem.h>

#include <celmath/vecmath.h>
#include <celmath/quaternion.h>
#include <celmath/mathlib.h>
#include <celutil/debug.h>
#include <celutil/util.h>
#include <celutil/filetype.h>
#include <celengine/celestia.h>
#include <celengine/astro.h>

#include "../celengine/gl.h"
#include "../celengine/glext.h"
#include "celestiacore.h"
#include "imagecapture.h"
#include "avicapture.h"
#include "winstarbrowser.h"
#include "winssbrowser.h"
#include "wintourguide.h"
#include "wingotodlg.h"
#include "winviewoptsdlg.h"

#include "res/resource.h"

using namespace std;


char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;

static vector<DEVMODE>* displayModes = NULL;
static int currentScreenMode = 0;
static int newScreenMode = 0;
static int lastFullScreenMode = 0;
static RECT windowRect;

static HGLRC glContext;
static HDC deviceContext;

static bool bReady = false;

static LPTSTR CelestiaRegKey = "Software\\Shatters.net\\Celestia";

HINSTANCE appInstance;
HWND mainWindow = 0;

static SolarSystemBrowser* solarSystemBrowser = NULL;
static StarBrowser* starBrowser = NULL;
static TourGuide* tourGuide = NULL;
static GotoObjectDialog* gotoObjectDlg = NULL;
static ViewOptionsDialog* viewOptionsDlg = NULL;

static HMENU menuBar = 0;
static HACCEL acceleratorTable = 0;

// Joystick info
static bool useJoystick = false;
static bool joystickAvailable = false;
static JOYCAPS joystickCaps;

bool cursorVisible = true;
static POINT saveCursorPos;

static int MovieSizes[4][2] = { { 160, 120 },
                                { 320, 240 },
                                { 640, 480 },
                                { 720, 480 } };
static float MovieFramerates[5] = { 15.0f, 24.0f, 25.0f, 29.97f, 30.0f };

static int movieSize = 1;
static int movieFramerate = 1;


astro::Date newTime(0.0);

#define INFINITE_MOUSE
static int lastX = 0;
static int lastY = 0;

#define ROTATION_SPEED  6
#define ACCELERATION    20.0f

static LRESULT CALLBACK MainWindowProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);


#define MENU_CHOOSE_PLANET   32000


struct AppPreferences
{
    int winWidth;
    int winHeight;
    int winX;
    int winY;
    int renderFlags;
    int labelMode;
    float visualMagnitude;
    float ambientLight;
    int pixelShader;
    int vertexShader;
    int showLocalTime;
};

void SetMouseCursor(LPCTSTR lpCursor)
{
	HCURSOR hNewCrsr;
	
	if(hNewCrsr = LoadCursor(NULL, lpCursor))
	    SetCursor(hNewCrsr);
}

void ChangeDisplayMode()
{
    DEVMODE device_mode;
  
    memset(&device_mode, 0, sizeof(DEVMODE));

    device_mode.dmSize = sizeof(DEVMODE);

    device_mode.dmPelsWidth  = 800;
    device_mode.dmPelsHeight = 600;
    device_mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

    ChangeDisplaySettings(&device_mode, CDS_FULLSCREEN);
}

  
void RestoreDisplayMode()
{
    ChangeDisplaySettings(0, 0);
}


static void acronymify(char* words, int length)
{
    int n = 0;
    char lastChar = ' ';

    for (int i = 0; i < length; i++)
    {
        if (lastChar == ' ')
            words[n++] = words[i];
        lastChar = words[i];
    }
    words[n] = '\0';
}


static void ShowUniversalTime(CelestiaCore* appCore)
{
    appCore->setTimeZoneBias(0);
    appCore->setTimeZoneName("UTC");
}


static void ShowLocalTime(CelestiaCore* appCore)
{
    TIME_ZONE_INFORMATION tzi;
    DWORD dst = GetTimeZoneInformation(&tzi);
    if (dst != TIME_ZONE_ID_INVALID)
    {
        LONG dstBias = 0;
        WCHAR* tzName = NULL;

        if (dst == TIME_ZONE_ID_STANDARD)
        {
            dstBias = tzi.StandardBias;
            tzName = tzi.StandardName;
        }
        else if (dst == TIME_ZONE_ID_DAYLIGHT)
        {
            dstBias = tzi.DaylightBias;
            tzName = tzi.DaylightName;
        }

        // Convert from wide chars to a normal C string
        char timeZoneName[64];
        WideCharToMultiByte(CP_ACP, 0,
                            tzName, -1,
                            timeZoneName, 64,
                            NULL, NULL);

        // Not sure what GetTimeZoneInformation can return for the
        // time zone name.  On my XP system, it returns the string
        // "Pacific Standard Time", when we want PST . . .  So, we
        // call acronymify to do the conversion.  If the time zone
        // name doesn't contain any spaces, we assume that it's
        // already in acronym form.
        if (strchr(timeZoneName, ' ') != NULL)
            acronymify(timeZoneName, strlen(timeZoneName));

        appCore->setTimeZoneBias((tzi.Bias + dstBias) * -60);
        appCore->setTimeZoneName(timeZoneName);
    }
}


static bool BeginMovieCapture(const std::string& filename,
                              int width, int height,
                              float framerate)
{
    MovieCapture* movieCapture = new AVICapture();

    bool success = movieCapture->start(filename, width, height, framerate);
    if (success)
        appCore->initMovieCapture(movieCapture);
    else
        delete movieCapture;
    
    return success;
}

static bool ToggleMenuItem(HMENU menu, int id)
{
    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_STATE;
    if (GetMenuItemInfo(menu, id, FALSE, &menuInfo))
    {
        bool isChecked = ((menuInfo.fState & MFS_CHECKED) != 0);
        CheckMenuItem(menu, id, isChecked ? MF_UNCHECKED : MF_CHECKED);
        return !isChecked;
    }
    return false;
}


void AppendLocationToMenu(string name, int index)
{
    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_SUBMENU;
    if (GetMenuItemInfo(menuBar, 4, TRUE, &menuInfo))
    {
        HMENU locationsMenu = menuInfo.hSubMenu;

        menuInfo.cbSize = sizeof MENUITEMINFO;
        menuInfo.fMask = MIIM_TYPE | MIIM_ID;
        menuInfo.fType = MFT_STRING;
        menuInfo.wID = ID_LOCATIONS_FIRSTLOCATION + index;
        menuInfo.dwTypeData = const_cast<char*>(name.c_str());
        InsertMenuItem(locationsMenu, index + 2, TRUE, &menuInfo);
    }
}


bool LoadItemTextFromFile(HWND hWnd,
                          int item,
                          char* filename)
{
    // ifstream textFile(filename, ios::in | ios::binary);
    ifstream textFile(filename, ios::in);
    string s;

    if (!textFile.good())
    {
        SetDlgItemText(hWnd, item, "License file missing!\r\r\nSee http://www.gnu.org/copyleft/gpl.html");
        return true;
    }

    char c;
    while (textFile.get(c))
    {
        if (c == '\n')
            s += "\r\r\n";
        else
            s += c;
    }

    SetDlgItemText(hWnd, item, s.c_str());

    return true;
}


BOOL APIENTRY AboutProc(HWND hDlg,
                        UINT message,
                        UINT wParam,
                        LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY ControlsHelpProc(HWND hDlg,
                              UINT message,
                              UINT wParam,
                              LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        LoadItemTextFromFile(hDlg, IDC_TEXT_CONTROLSHELP, "controls.txt");
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY LicenseProc(HWND hDlg,
                          UINT message,
                          UINT wParam,
                          LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        LoadItemTextFromFile(hDlg, IDC_LICENSE_TEXT, "COPYING");
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY GLInfoProc(HWND hDlg,
                         UINT message,
                         UINT wParam,
                         LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            char* vendor = (char*) glGetString(GL_VENDOR);
            char* render = (char*) glGetString(GL_RENDERER);
            char* version = (char*) glGetString(GL_VERSION);
            char* ext = (char*) glGetString(GL_EXTENSIONS);
            string s;
            s += "Vendor: ";
            if (vendor != NULL)
                s += vendor;
            s += "\r\r\n";
            
            s += "Renderer: ";
            if (render != NULL)
                s += render;
            s += "\r\r\n";

            s += "Version: ";
            if (version != NULL)
                s += version;
            s += "\r\r\n";

            char buf[100];
            GLint simTextures = 1;
            if (ExtensionSupported("GL_ARB_multitexture"))
                glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &simTextures);
            sprintf(buf, "Max simultaneous textures: %d\r\r\n",
                    simTextures);
            s += buf;

            GLint maxTextureSize = 0;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
            sprintf(buf, "Max texture size: %d\r\r\n",
                    maxTextureSize);
            s += buf;

            s += "\r\r\nSupported Extensions:\r\r\n";
            if (ext != NULL)
            {
                string extString(ext);
                int pos = extString.find(' ', 0);
                while (pos != string::npos)
                {
                    extString.replace(pos, 1, "\r\r\n");
                    pos = extString.find(' ', pos);
                }
                s += extString;
            }

            SetDlgItemText(hDlg, IDC_GLINFO_TEXT, s.c_str());
        }
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


UINT CALLBACK ChooseMovieParamsProc(HWND hDlg, UINT message,
                                    WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            char buf[100];
            HWND hwnd = GetDlgItem(hDlg, IDC_COMBO_MOVIE_SIZE);
            int nSizes = sizeof MovieSizes / sizeof MovieSizes[0];

            int i;
            for (i = 0; i < nSizes; i++)
            {
                sprintf(buf, "%d x %d", MovieSizes[i][0], MovieSizes[i][1]);
                SendMessage(hwnd, CB_INSERTSTRING, -1,
                            reinterpret_cast<LPARAM>(buf));

            }
            SendMessage(hwnd, CB_SETCURSEL, movieSize, 0);

            hwnd = GetDlgItem(hDlg, IDC_COMBO_MOVIE_FRAMERATE);
            int nFramerates = sizeof MovieFramerates / sizeof MovieFramerates[0];
            for (i = 0; i < nFramerates; i++)
            {
                sprintf(buf, "%.2f", MovieFramerates[i]);
                SendMessage(hwnd, CB_INSERTSTRING, -1,
                            reinterpret_cast<LPARAM>(buf));
            }
            SendMessage(hwnd, CB_SETCURSEL, movieFramerate, 0);
        }
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_COMBO_MOVIE_SIZE)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwnd = reinterpret_cast<HWND>(lParam);
                int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                if (item != CB_ERR)
                    movieSize = item;
            }
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_COMBO_MOVIE_FRAMERATE)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwnd = reinterpret_cast<HWND>(lParam);
                int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                if (item != CB_ERR)
                    movieFramerate = item;
            }
            return TRUE;
        }
    }

    return FALSE;
}


BOOL APIENTRY FindObjectProc(HWND hDlg,
                             UINT message,
                             UINT wParam,
                             LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char buf[1024];
            int len = GetDlgItemText(hDlg, IDC_FINDOBJECT_EDIT, buf, 1024);
            if (len > 0)
            {
                Selection sel = appCore->getSimulation()->findObject(string(buf));
                if (!sel.empty())
                    appCore->getSimulation()->setSelection(sel);
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY AddLocationProc(HWND hDlg,
                              UINT message,
                              UINT wParam,
                              LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char buf[1024];
            int len = GetDlgItemText(hDlg, IDC_LOCATION_EDIT, buf, 1024);
            if (len > 0)
            {
                string name(buf);

                appCore->addFavorite(name);
                AppendLocationToMenu(name, appCore->getFavorites()->size() - 1);
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
        break;
    }

    return FALSE;
}

void UpdateSetTimeDlgDateTimeControls(HWND hDlg, astro::Date& newTime)
{
    HWND timeItem = NULL;
    HWND dateItem = NULL;
    SYSTEMTIME sysTime;

    sysTime.wYear = newTime.year;
    sysTime.wMonth = newTime.month;
    sysTime.wDay = newTime.day;
    sysTime.wDayOfWeek = ((int) ((double) newTime + 0.5) + 1) % 7;
    sysTime.wHour = newTime.hour;
    sysTime.wMinute = newTime.minute;
    sysTime.wSecond = (int) newTime.seconds;
    sysTime.wMilliseconds = 0;

    dateItem = GetDlgItem(hDlg, IDC_DATEPICKER);
    if (dateItem != NULL)
    {
        DateTime_SetFormat(dateItem, "dd' 'MMM' 'yyy");
        DateTime_SetSystemtime(dateItem, GDT_VALID, &sysTime);
    }
    timeItem = GetDlgItem(hDlg, IDC_TIMEPICKER);
    if (timeItem != NULL)
    {
        DateTime_SetFormat(timeItem, "HH':'mm':'ss' UT'");
        DateTime_SetSystemtime(timeItem, GDT_VALID, &sysTime);
    }
}


BOOL APIENTRY SetTimeProc(HWND hDlg,
                          UINT message,
                          UINT wParam,
                          LONG lParam)
{
    HWND timeItem = NULL;
    HWND dateItem = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
        {
            newTime = astro::Date(appCore->getSimulation()->getTime());
            UpdateSetTimeDlgDateTimeControls(hDlg, newTime);
        }
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
                appCore->getSimulation()->setTime((double) newTime);
            EndDialog(hDlg, 0);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_SETCURRENTTIME)
        {
            //Set newTime = current system time;
            newTime = astro::Date((double) time(NULL) / 86400.0 + (double) astro::Date(1970, 1, 1));

            //Force Date/Time controls to show current system time
            UpdateSetTimeDlgDateTimeControls(hDlg, newTime);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR) lParam;

            if (hdr->code == DTN_DATETIMECHANGE)
            {
                LPNMDATETIMECHANGE change = (LPNMDATETIMECHANGE) lParam;
                if (change->dwFlags == GDT_VALID)
                {
                    if (wParam == IDC_DATEPICKER)
                    {
                        newTime.year = change->st.wYear;
                        newTime.month = change->st.wMonth;
                        newTime.day = change->st.wDay;
                    }
                    else if (wParam == IDC_TIMEPICKER)
                    {
                        newTime.hour = change->st.wHour;
                        newTime.minute = change->st.wMinute;
                        newTime.seconds = change->st.wSecond + (double) change->st.wMilliseconds / 1000.0;
                    }
                }
            }
        }
    }

    return FALSE;
}


int selectedScreenMode = 0;
BOOL APIENTRY SelectDisplayModeProc(HWND hDlg,
                                    UINT message,
                                    UINT wParam,
                                    LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            char buf[100];
            HWND hwnd = GetDlgItem(hDlg, IDC_COMBO_RESOLUTION);

            // Add windowed mode as the first item on the menu
            SendMessage(hwnd, CB_INSERTSTRING, -1,
                        reinterpret_cast<LPARAM>("Windowed Mode"));

            for (vector<DEVMODE>::const_iterator iter= displayModes->begin();
                 iter != displayModes->end(); iter++)
            {
                sprintf(buf, "%d x %d x %d",
                        iter->dmPelsWidth, iter->dmPelsHeight,
                        iter->dmBitsPerPel);
                SendMessage(hwnd, CB_INSERTSTRING, -1,
                            reinterpret_cast<LPARAM>(buf));
            }
            SendMessage(hwnd, CB_SETCURSEL, currentScreenMode, 0);
        }
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            newScreenMode = selectedScreenMode;
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_COMBO_RESOLUTION)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwnd = reinterpret_cast<HWND>(lParam);
                int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                if (item != CB_ERR)
                    selectedScreenMode = item;
            }
            return TRUE;
        }
    }

    return FALSE;
}


HMENU CreateMenuBar()
{
    return LoadMenu(appInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));
}

static void setMenuItemCheck(int menuItem, bool checked)
{
    CheckMenuItem(menuBar, menuItem, checked ? MF_CHECKED : MF_UNCHECKED);
}


HMENU CreatePlanetarySystemMenu(const PlanetarySystem* planets)
{
    HMENU menu = CreatePopupMenu();
    
    for (int i = 0; i < planets->getSystemSize(); i++)
    {
        Body* body = planets->getBody(i);
        AppendMenu(menu, MF_STRING, MENU_CHOOSE_PLANET + i,
                   body->getName().c_str());
    }

    return menu;
}


VOID APIENTRY handlePopupMenu(HWND hwnd,
                              float x, float y,
                              const Selection& sel)
{
    HMENU hMenu;
    string name;

    hMenu = CreatePopupMenu();

    if (sel.body != NULL)
    {
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, sel.body->getName().c_str());
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, "&Goto");
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, "&Follow");
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_SYNCORBIT, "S&ync Orbit");
        AppendMenu(hMenu, MF_STRING, ID_INFO, "&Info");

        const PlanetarySystem* satellites = sel.body->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            HMENU satMenu = CreatePlanetarySystemMenu(satellites);
            AppendMenu(hMenu, MF_POPUP | MF_STRING, (DWORD) satMenu,
                       "&Satellites");
        }
    }
    else if (sel.star != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        name = sim->getUniverse()->getStarCatalog()->getStarName(*sel.star);
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, name.c_str());
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, "&Goto");
        AppendMenu(hMenu, MF_STRING, ID_INFO, "&Info");

        SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            HMENU planetsMenu = CreatePlanetarySystemMenu(solarSys->getPlanets());
            AppendMenu(hMenu,
                       MF_POPUP | MF_STRING,
                       (DWORD) planetsMenu,
                       "&Planets");
        }
    }

    POINT point;
    point.x = (int) x;
    point.y = (int) y;
    
    if (currentScreenMode == 0)
        ClientToScreen(hwnd, (LPPOINT) &point);

    appCore->getSimulation()->setSelection(sel);
    TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hwnd, NULL);

    // TODO: Do we need to explicitly destroy submenus or does DestroyMenu
    // work recursively?
    // According to the MSDN documentation, DestroyMenu() IS recursive. Clint 11/01.
    DestroyMenu(hMenu);
}


// This still needs a lot of work . . .
// TODO: spawn isn't the best way to launch IE--we don't want to start a new
//       process every time.
// TODO: the location of IE is assumed to be c:\program files\internet explorer
// TODO: get rid of fixed urls
void ShowWWWInfo(const Selection& sel)
{
    string url;
    if (sel.body != NULL)
    {
        string name = sel.body->getName();
        for (int i = 0; i < name.size(); i++)
            name[i] = tolower(name[i]);

        url = string("http://www.nineplanets.org/") + name + ".html";
    }
    else if (sel.star != NULL)
    {
        char name[32];
        sprintf(name, "HIP%d", sel.star->getCatalogNumber() & ~0xf0000000);

        url = string("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=") + name;
    }

    ShellExecute(mainWindow,
                 "open",
                 url.c_str(),
                 NULL,
                 NULL,
                 0);
}


void ContextMenu(float x, float y, Selection sel)
{
    handlePopupMenu(mainWindow, x, y, sel);
}


bool EnableFullScreen(const DEVMODE& dm)
{
    DEVMODE devMode;

    ZeroMemory(&devMode, sizeof devMode);
    devMode.dmSize = sizeof devMode;
    devMode.dmPelsWidth  = dm.dmPelsWidth;
    devMode.dmPelsHeight = dm.dmPelsHeight;
    devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

    if (ChangeDisplaySettings(&devMode, CDS_FULLSCREEN) != 
        DISP_CHANGE_SUCCESSFUL)
    {
	MessageBox(NULL,
                   "Unable to switch to full screen mode; running in window mode",
                   "Error",
                   MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}


void DisableFullScreen()
{
    ChangeDisplaySettings(0, 0);
}


// Select the pixel format for a given device context
void SetDCPixelFormat(HDC hDC)
{
    static PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	// Size of this structure
	1,				// Version of this structure
	PFD_DRAW_TO_WINDOW |		// Draw to Window (not to bitmap)
	PFD_SUPPORT_OPENGL |		// Support OpenGL calls in window
	PFD_DOUBLEBUFFER,		// Double buffered mode
	PFD_TYPE_RGBA,			// RGBA Color mode
	GetDeviceCaps(hDC, BITSPIXEL),	// Want the display bit depth		
	0,0,0,0,0,0,			// Not used to select mode
	0,0,				// Not used to select mode
	0,0,0,0,0,			// Not used to select mode
	16,				// Size of depth buffer
	0,				// Not used to select mode
	0,				// Not used to select mode
	PFD_MAIN_PLANE,			// Draw in main plane
	0,				// Not used to select mode
	0,0,0                           // Not used to select mode
    };

    // Choose a pixel format that best matches that described in pfd
    int nPixelFormat = ChoosePixelFormat(hDC, &pfd);

    // Set the pixel format for the device context
    SetPixelFormat(hDC, nPixelFormat, &pfd);
}


HWND CreateOpenGLWindow(int x, int y, int width, int height,
                        int mode, int& newMode)
{
    assert(mode >= 0 && mode < displayModes->size());
    if (mode != 0)
    {
        x = 0;
        y = 0;
        width = displayModes->at(mode - 1).dmPelsWidth;
        height = displayModes->at(mode - 1).dmPelsHeight;
    }

    // Set up and register the window class
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = appInstance;
    wc.hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_CELESTIA_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = AppName;
    if (RegisterClass(&wc) == 0)
    {
	MessageBox(NULL,
                   "Failed to register the window class.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
	return NULL;
    }

    newMode = currentScreenMode;
    if (mode != 0)
    {
        if (EnableFullScreen(displayModes->at(mode - 1)))
            newMode = mode;
    }
    else
    {
        DisableFullScreen();
        newMode = 0;
    }

    // Determine the proper window style to use
    DWORD dwStyle;
    if (newMode != 0)
    {
        dwStyle = WS_POPUP;
    }
    else
    {
        dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    }

    // Create the window
    HWND hwnd = CreateWindow(AppName,
                             AppName,
                             dwStyle,
                             x, y,
                             width, height,
                             NULL,
                             NULL,
                             appInstance,
                             NULL);

    if (hwnd == NULL)
        return NULL;

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    deviceContext = GetDC(hwnd);
    SetDCPixelFormat(deviceContext);
    if (glContext == NULL)
        glContext = wglCreateContext(deviceContext);
    wglMakeCurrent(deviceContext, glContext);

    if (newMode == 0)
        SetMenu(hwnd, menuBar);

    return hwnd;
}


void DestroyOpenGLWindow()
{
#if 0
    if (glContext != NULL)
    {
        wglMakeCurrent(NULL, NULL);
        if (!wglDeleteContext(glContext))
        {
            MessageBox(NULL,
                       "Releasing GL context failed.", "Error",
                       MB_OK | MB_ICONERROR);
        }
        glContext = NULL;
    }
#endif

    if (deviceContext != NULL)
    {
        if (!ReleaseDC(mainWindow, deviceContext))
        {
            MessageBox(NULL,
                       "Releasing device context failed.", "Error",
                       MB_OK | MB_ICONERROR);
        }
        deviceContext = NULL;
    }

    if (mainWindow != NULL)
    {
        SetMenu(mainWindow, NULL);
        DestroyWindow(mainWindow);
        mainWindow = NULL;
    }

    UnregisterClass(AppName, appInstance);
}


void handleKey(WPARAM key, bool down)
{
    int k = -1;
    switch (key)
    {
    case VK_UP:
        k = CelestiaCore::Key_Up;
        break;
    case VK_DOWN:
        k = CelestiaCore::Key_Down;
        break;
    case VK_LEFT:
        k = CelestiaCore::Key_Left;
        break;
    case VK_RIGHT:
        k = CelestiaCore::Key_Right;
        break;
    case VK_HOME:
        k = CelestiaCore::Key_Home;
        break;
    case VK_END:
        k = CelestiaCore::Key_End;
        break;
    case VK_F1:
        k = CelestiaCore::Key_F1;
        break;
    case VK_F2:
        k = CelestiaCore::Key_F2;
        break;
    case VK_F3:
        k = CelestiaCore::Key_F3;
        break;
    case VK_F4:
        k = CelestiaCore::Key_F4;
        break;
    case VK_F5:
        k = CelestiaCore::Key_F5;
        break;
    case VK_F6:
        k = CelestiaCore::Key_F6;
        break;
    case VK_F7:
        k = CelestiaCore::Key_F7;
        break;
    case VK_F8:
        if (joystickAvailable && down)
        {
            appCore->joystickAxis(CelestiaCore::Joy_XAxis, 0);
            appCore->joystickAxis(CelestiaCore::Joy_YAxis, 0);
            appCore->joystickAxis(CelestiaCore::Joy_ZAxis, 0);
            useJoystick = !useJoystick;
        }
        break;
    case VK_F11:
        k = CelestiaCore::Key_F11;
        break;
    case VK_F12:
        k = CelestiaCore::Key_F12;
        break;

    case VK_NUMPAD2:
        k = CelestiaCore::Key_NumPad2;
        break;
    case VK_NUMPAD4:
        k = CelestiaCore::Key_NumPad4;
        break;
    case VK_NUMPAD5:
        k = CelestiaCore::Key_NumPad5;
        break;
    case VK_NUMPAD6:
        k = CelestiaCore::Key_NumPad6;
        break;
    case VK_NUMPAD7:
        k = CelestiaCore::Key_NumPad7;
        break;
    case VK_NUMPAD8:
        k = CelestiaCore::Key_NumPad8;
        break;
    case VK_NUMPAD9:
        k = CelestiaCore::Key_NumPad9;
        break;
    case 'A':
    case 'Z':
        k = key;
        break;
    }

    if (k >= 0)
    {
        if (down)
            appCore->keyDown(k);
        else
            appCore->keyUp(k);
    }
}


static void BuildFavoritesMenu()
{
    // Add favorites to locations menu
    const FavoritesList* favorites = appCore->getFavorites();
    if (favorites != NULL)
    {
        MENUITEMINFO menuInfo;
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.fMask = MIIM_SUBMENU;
        if (GetMenuItemInfo(menuBar, 4, TRUE, &menuInfo))
        {
            HMENU locationsMenu = menuInfo.hSubMenu;

            menuInfo.cbSize = sizeof MENUITEMINFO;
            menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
            menuInfo.fType = MFT_SEPARATOR;
            menuInfo.fState = MFS_UNHILITE;
            InsertMenuItem(locationsMenu, 1, TRUE, &menuInfo);

            int index = 0;
            for (FavoritesList::const_iterator iter = favorites->begin();
                 iter != favorites->end();
                 iter++, index++)
            {
                menuInfo.cbSize = sizeof MENUITEMINFO;
                menuInfo.fMask = MIIM_TYPE | MIIM_ID;
                menuInfo.fType = MFT_STRING;
                // menuInfo.fState = MFS_DEFAULT;
                menuInfo.wID = ID_LOCATIONS_FIRSTLOCATION + index;
                menuInfo.dwTypeData = const_cast<char*>((*iter)->name.c_str());
                InsertMenuItem(locationsMenu, index + 2, TRUE, &menuInfo);
            }
        }
    }
}


static void syncMenusWithRendererState()
{
    int renderFlags = appCore->getRenderer()->getRenderFlags();
    int labelMode = appCore->getRenderer()->getLabelMode();
    float ambientLight = appCore->getRenderer()->getAmbientLightLevel();

    setMenuItemCheck(ID_RENDER_PIXEL_SHADERS,
                     appCore->getRenderer()->getFragmentShaderEnabled());
    setMenuItemCheck(ID_RENDER_VERTEX_SHADERS,
                     appCore->getRenderer()->getVertexShaderEnabled());

    if(abs(0.0 - (double)ambientLight) < 1.0e-3)
    {
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_CHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
    }
    else if(abs(0.1 - (double)ambientLight) < 1.0e-3)
    {
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_CHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
    }
    else if(abs(0.25 - (double)ambientLight) < 1.0e-3)
    {
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_CHECKED);
    }

    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_STATE;
    if (GetMenuItemInfo(menuBar, ID_TIME_SHOWLOCAL, FALSE, &menuInfo))
    {
        if(appCore->getTimeZoneBias() == 0)
            CheckMenuItem(menuBar, ID_TIME_SHOWLOCAL, MF_UNCHECKED);
        else
            CheckMenuItem(menuBar, ID_TIME_SHOWLOCAL, MF_CHECKED);
    }
}


class WinAlerter : public CelestiaCore::Alerter
{
private:
    HWND hwnd;

public:
    WinAlerter() : hwnd(NULL) {};
    ~WinAlerter() {};

    void fatalError(const std::string& msg)
    {
	    MessageBox(NULL,
                   msg.c_str(),
                   "Fatal Error",
                   MB_OK | MB_ICONERROR);
    }
};


static bool InitJoystick(JOYCAPS& caps)
{
    int nJoysticks = joyGetNumDevs();
    if (nJoysticks <= 0)
        return false;

    if (joyGetDevCaps(JOYSTICKID1, &caps, sizeof caps) != JOYERR_NOERROR)
    {
        cout << "Error getting joystick caps.\n";
        return false;
    }

    cout << "Using joystick: " << caps.szPname << '\n';

    return true;
}


static void HandleJoystick()
{
    JOYINFOEX info;

    info.dwSize = sizeof info;
    info.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS;
    MMRESULT err = joyGetPosEx(JOYSTICKID1, &info);

    if (err == JOYERR_NOERROR)
    {
        float x = (float) info.dwXpos / 32768.0f - 1.0f;
        float y = (float) info.dwYpos / 32768.0f - 1.0f;

        appCore->joystickAxis(CelestiaCore::Joy_XAxis, x);
        appCore->joystickAxis(CelestiaCore::Joy_YAxis, y);
        appCore->joystickButton(CelestiaCore::JoyButton1,
                                (info.dwButtons & 0x1) != 0);
        appCore->joystickButton(CelestiaCore::JoyButton2,
                                (info.dwButtons & 0x2) != 0);
        appCore->joystickButton(CelestiaCore::JoyButton7,
                                (info.dwButtons & 0x40) != 0);
        appCore->joystickButton(CelestiaCore::JoyButton8,
                                (info.dwButtons & 0x80) != 0);
    }
}

static bool GetRegistryValue(HKEY hKey, LPSTR cpValueName, LPVOID lpBuf, DWORD iBufSize)
{
/*
    Function retrieves a value from the registry.
    Key specified by open handle.

    hKey        - Handle of open key for which a key value is requested.

    cpValueName	- Name of Key Value to obtain value for.

    lpBuf      - Buffer to receive value of Key Value.

    iBufSize   - Size of buffer pointed to by lpBuf.

    ipDataSize - Optional. Receives size of data returned in lpBuf.

    ipDataType - Optional. Receives type of data.

    RETURN     - Boolean true if value was successfully retrieved, false otherwise.

    Remarks:        If requesting a string value, pass the character buffer to lpBuf.
                    If requesting a DWORD value, pass the DWORD variable's address to lpBuf.
                    If requesting binary data, pass the address of the binary buffer.

                    This function assumes you have an open registry key. Be sure to call
                    CloseKey() when finished.
*/
	DWORD dwValueType, dwDataSize=0;
	bool bRC=false;

	dwDataSize = iBufSize;
	if(RegQueryValueEx(hKey, cpValueName, NULL, &dwValueType,
		(LPBYTE)lpBuf, &dwDataSize) == ERROR_SUCCESS)
		bRC = true;

	return bRC;
}

static bool SetRegistryInt(HKEY key, LPCTSTR value, int intVal)
{
    LONG err = RegSetValueEx(key,
                             value,
                             0,
                             REG_DWORD,
                             reinterpret_cast<CONST BYTE*>(&intVal),
                             sizeof(DWORD));
    return err == ERROR_SUCCESS;
}

static bool SetRegistryBin(HKEY hKey, LPSTR cpValueName, LPVOID lpData, int iDataSize)
{
/*
    Function sets BINARY data in the registry.
    Key specified by open handle.

    hKey        - Handle of Key for which a value is to be set.

    cpValueName - Name of Key Value to obtain value for.

    lpData      - Address of binary data to store.

    iDataSize   - Size of binary data contained in lpData.

    RETURN      - Boolean true if value is successfully stored, false otherwise.

    Remarks:        This function assumes you have an open registry key. Be sure to call
                    CloseKey() when finished.
*/

	bool bRC=false;

	if(RegSetValueEx(hKey, cpValueName, 0, REG_BINARY, (LPBYTE)lpData, (DWORD)iDataSize) == ERROR_SUCCESS)
		bRC = true;

	return bRC;
}


static bool LoadPreferencesFromRegistry(LPTSTR regkey, AppPreferences& prefs)
{
    LONG err;
    HKEY key;

    DWORD disposition;
    err = RegCreateKeyEx(HKEY_CURRENT_USER,
                         regkey,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &key,
                         &disposition);
    if (err  != ERROR_SUCCESS)
    {
        cout << "Error opening registry key: " << err << '\n';
        return false;
    }
    cout << "Opened registry key\n";

    GetRegistryValue(key, "Width", &prefs.winWidth, sizeof(prefs.winWidth));
    GetRegistryValue(key, "Height", &prefs.winHeight, sizeof(prefs.winHeight));
    GetRegistryValue(key, "XPos", &prefs.winX, sizeof(prefs.winX));
    GetRegistryValue(key, "YPos", &prefs.winY, sizeof(prefs.winY));
    GetRegistryValue(key, "RenderFlags", &prefs.renderFlags, sizeof(prefs.renderFlags));
    GetRegistryValue(key, "LabelMode", &prefs.labelMode, sizeof(prefs.labelMode));
    GetRegistryValue(key, "VisualMagnitude", &prefs.visualMagnitude, sizeof(prefs.visualMagnitude));
    GetRegistryValue(key, "AmbientLight", &prefs.ambientLight, sizeof(prefs.ambientLight));
    GetRegistryValue(key, "PixelShader", &prefs.pixelShader, sizeof(prefs.pixelShader));
    GetRegistryValue(key, "VertexShader", &prefs.vertexShader, sizeof(prefs.vertexShader));
    GetRegistryValue(key, "ShowLocalTime", &prefs.showLocalTime, sizeof(prefs.showLocalTime));

    RegCloseKey(key);

    return true;
}


static bool SavePreferencesToRegistry(LPTSTR regkey, AppPreferences& prefs)
{
    LONG err;
    HKEY key;

    cout << "Saving preferences . . .\n";
    err = RegOpenKeyEx(HKEY_CURRENT_USER,
                       regkey,
                       0,
                       KEY_ALL_ACCESS,
                       &key);
    if (err  != ERROR_SUCCESS)
    {
        cout << "Error opening registry key: " << err << '\n';
        return false;
    }
    cout << "Opened registry key\n";

    SetRegistryInt(key, "Width", prefs.winWidth);
    SetRegistryInt(key, "Height", prefs.winHeight);
    SetRegistryInt(key, "XPos", prefs.winX);
    SetRegistryInt(key, "YPos", prefs.winY);
    SetRegistryInt(key, "RenderFlags", prefs.renderFlags);
    SetRegistryInt(key, "LabelMode", prefs.labelMode);
    SetRegistryBin(key, "VisualMagnitude", &prefs.visualMagnitude, sizeof(prefs.visualMagnitude));
    SetRegistryBin(key, "AmbientLight", &prefs.ambientLight, sizeof(prefs.ambientLight));
    SetRegistryInt(key, "PixelShader", prefs.pixelShader);
    SetRegistryInt(key, "VertexShader", prefs.vertexShader);
    SetRegistryInt(key, "ShowLocalTime", prefs.showLocalTime);

    RegCloseKey(key);

    return true;
}


static bool GetCurrentPreferences(AppPreferences& prefs)
{
    WINDOWPLACEMENT placement;

    placement.length = sizeof(placement);
    if (!GetWindowPlacement(mainWindow, &placement))
        return false;

    RECT rect = placement.rcNormalPosition;
    prefs.winX = rect.left;
    prefs.winY = rect.top;
    prefs.winWidth = rect.right - rect.left;
    prefs.winHeight = rect.bottom - rect.top;
    prefs.renderFlags = appCore->getRenderer()->getRenderFlags();
    prefs.labelMode = appCore->getRenderer()->getLabelMode();
    prefs.visualMagnitude = appCore->getSimulation()->getFaintestVisible();
    prefs.ambientLight = appCore->getRenderer()->getAmbientLightLevel();
    prefs.pixelShader = appCore->getRenderer()->getFragmentShaderEnabled()?1:0;
    prefs.vertexShader = appCore->getRenderer()->getVertexShaderEnabled()?1:0;
    prefs.showLocalTime = (appCore->getTimeZoneBias() != 0);

    return true;
}


static void HandleCaptureImage(HWND hWnd)
{
    // Display File SaveAs dialog to allow user to specify name and location of
    // of captured screen image.
    OPENFILENAME Ofn;
    char szFile[_MAX_PATH+1], szFileTitle[_MAX_PATH+1];

    szFile[0] = '\0';
    szFileTitle[0] = '\0';

    // Initialize OPENFILENAME
    ZeroMemory(&Ofn, sizeof(OPENFILENAME));
    Ofn.lStructSize = sizeof(OPENFILENAME);
    Ofn.hwndOwner = hWnd;
    Ofn.lpstrFilter = "JPEG - JFIF Compliant\0*.jpg;*.jif;*.jpeg\0Portable Network Graphics\0*.png\0";
    Ofn.lpstrFile= szFile;
    Ofn.nMaxFile = sizeof(szFile);
    Ofn.lpstrFileTitle = szFileTitle;
    Ofn.nMaxFileTitle = sizeof(szFileTitle);
    Ofn.lpstrInitialDir = (LPSTR)NULL;

    // Comment this out if you just want the standard "Save As" caption.
    Ofn.lpstrTitle = "Save As - Specify File to Capture Image";

    // OFN_HIDEREADONLY - Do not display read-only JPEG or PNG files
    // OFN_OVERWRITEPROMPT - If user selected a file, prompt for overwrite confirmation.
    Ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

    // Display the Save dialog box.
    if(GetSaveFileName(&Ofn))
    {
        // If you got here, a path and file has been specified.
        // Ofn.lpstrFile contains full path to specified file
        // Ofn.lpstrFileTitle contains just the filename with extension

        // Get the dimensions of the current viewport
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        bool success = false;

        DWORD nFileType=0;
        char defaultExtensions[][4] = {"jpg", "png"};
        if(Ofn.nFileExtension == 0)
        {
            //If no extension was specified, use the selection of filter to determine
            //which type of file should be created, instead of just defaulting to JPEG.
            nFileType = Ofn.nFilterIndex;
            strcat(Ofn.lpstrFile, ".");
            strcat(Ofn.lpstrFile, defaultExtensions[nFileType-1]);
        }
        else if(*(Ofn.lpstrFile + Ofn.nFileExtension) == '\0')
        {
            //If just a period was specified for the extension, use the selection of
            //filter to determine which type of file should be created, instead of
            //just defaulting to JPEG.
            nFileType = Ofn.nFilterIndex;
            strcat(Ofn.lpstrFile, defaultExtensions[nFileType-1]);
        }
        else
        {
            switch (DetermineFileType(Ofn.lpstrFile))
            {
            case Content_JPEG:
                nFileType = 1;
                break;
            case Content_PNG:
                nFileType = 2;
                break;
            default:
                nFileType = 0;
                break;
            }
        }

        if (nFileType == 1)
        {
            success = CaptureGLBufferToJPEG(string(Ofn.lpstrFile),
                                            viewport[0], viewport[1],
                                            viewport[2], viewport[3]);
        }
        else if (nFileType == 2)
        {
            success = CaptureGLBufferToPNG(string(Ofn.lpstrFile),
                                           viewport[0], viewport[1],
                                           viewport[2], viewport[3]);
        }
        else
        {
            // Invalid file extension specified.
            DPRINTF("WTF? Unknown file extension specified for screen capture.\n");
        }

        if (!success)
        {
            char errorMsg[64];

            if(nFileType == 0)
                sprintf(errorMsg, "Specified file extension is not recognized.");
            else
                sprintf(errorMsg, "Could not save image file.");

            MessageBox(hWnd, errorMsg, "Error", MB_OK | MB_ICONERROR);
        }
    }
}


static void HandleCaptureMovie(HWND hWnd)
{
    // TODO: The menu item should be disable so that the user doesn't even
    // have the opportunity to record two movies simultaneously; the only
    // thing missing to make this happen is notification when recording
    // is complete.
    if (appCore->isCaptureActive())
    {
        MessageBox(hWnd, "Stop current movie capture before starting another one.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // Display File SaveAs dialog to allow user to specify name and location of
    // of captured movie
    OPENFILENAME Ofn;
    char szFile[_MAX_PATH+1], szFileTitle[_MAX_PATH+1];

    szFile[0] = '\0';
    szFileTitle[0] = '\0';

    // Initialize OPENFILENAME
    ZeroMemory(&Ofn, sizeof(OPENFILENAME));
    Ofn.lStructSize = sizeof(OPENFILENAME);
    Ofn.hwndOwner = hWnd;
    Ofn.lpstrFilter = "Microsoft AVI\0*.avi\0";
    Ofn.lpstrFile= szFile;
    Ofn.nMaxFile = sizeof(szFile);
    Ofn.lpstrFileTitle = szFileTitle;
    Ofn.nMaxFileTitle = sizeof(szFileTitle);
    Ofn.lpstrInitialDir = (LPSTR)NULL;

    // Comment this out if you just want the standard "Save As" caption.
    Ofn.lpstrTitle = "Save As - Specify File to Capture Movie";

    // OFN_HIDEREADONLY - Do not display read-only video files
    // OFN_OVERWRITEPROMPT - If user selected a file, prompt for overwrite confirmation.
    Ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT  | OFN_EXPLORER | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_NOCHANGEDIR;

    Ofn.hInstance = appInstance;
    Ofn.lpTemplateName = MAKEINTRESOURCE(IDD_MOVIE_PARAMS_CHOOSER);
    Ofn.lpfnHook = ChooseMovieParamsProc;

    // Display the Save dialog box.
    if (GetSaveFileName(&Ofn))
    {
        // If you got here, a path and file has been specified.
        // Ofn.lpstrFile contains full path to specified file
        // Ofn.lpstrFileTitle contains just the filename with extension

        bool success = false;

        DWORD nFileType=0;
        char defaultExtensions[][4] = { "avi" };
        if (Ofn.nFileExtension == 0)
        {
            // If no extension was specified, use the selection of filter to
            // determine which type of file should be created, instead of
            // just defaulting to AVI.
            nFileType = Ofn.nFilterIndex;
            strcat(Ofn.lpstrFile, ".");
            strcat(Ofn.lpstrFile, defaultExtensions[nFileType-1]);
        }
        else if (*(Ofn.lpstrFile + Ofn.nFileExtension) == '\0')
        {
            // If just a period was specified for the extension, use
            // the selection of filter to determine which type of file
            // should be created, instead of just defaulting to AVI.
            nFileType = Ofn.nFilterIndex;
            strcat(Ofn.lpstrFile, defaultExtensions[nFileType-1]);
        }
        else
        {
            switch (DetermineFileType(Ofn.lpstrFile))
            {
            case Content_AVI:
                nFileType = 1;
                break;
            default:
                nFileType = 0;
                break;
            }
        }

        if (nFileType != 1)
        {
            // Invalid file extension specified.
            DPRINTF("Unknown file extension specified for movie capture.\n");
        }
        else
        {
            success = BeginMovieCapture(string(Ofn.lpstrFile),
                                        MovieSizes[movieSize][0],
                                        MovieSizes[movieSize][1],
                                        MovieFramerates[movieFramerate]);
        }

        if (!success)
        {
            char errorMsg[64];

            if(nFileType == 0)
                sprintf(errorMsg, "Specified file extension is not recognized.");
            else
                sprintf(errorMsg, "Could not capture movie.");

            MessageBox(hWnd, errorMsg, "Error", MB_OK | MB_ICONERROR);
        }
    }
}


bool operator<(const DEVMODE& a, const DEVMODE& b)
{
    if (a.dmBitsPerPel != b.dmBitsPerPel)
        return a.dmBitsPerPel < b.dmBitsPerPel;
    if (a.dmPelsWidth != b.dmPelsWidth)
        return a.dmPelsWidth < b.dmPelsWidth;
    if (a.dmPelsHeight != b.dmPelsHeight)
        return a.dmPelsHeight < b.dmPelsHeight;
    return a.dmDisplayFrequency < b.dmDisplayFrequency;
}

vector<DEVMODE>* EnumerateDisplayModes(int minBPP)
{
    vector<DEVMODE>* modes = new vector<DEVMODE>();
    if (modes == NULL)
        return NULL;

    DEVMODE dm;
    int i = 0;
    while (EnumDisplaySettings(NULL, i, &dm))
    {
        if (dm.dmBitsPerPel >= minBPP)
            modes->insert(modes->end(), dm);
        i++;
    }
    sort(modes->begin(), modes->end());

    // Go through the sorted list and eliminate modes that differ only
    // by refresh rates.
    vector<DEVMODE>::iterator keepIter = modes->begin();
    vector<DEVMODE>::const_iterator iter = modes->begin();
    iter++;
    for (; iter != modes->end(); iter++)
    {
        if (iter->dmPelsWidth != keepIter->dmPelsWidth ||
            iter->dmPelsHeight != keepIter->dmPelsHeight ||
            iter->dmBitsPerPel != keepIter->dmBitsPerPel)
        {
            *++keepIter = *iter;
        }
    }

    modes->resize(keepIter - modes->begin());

    // Select the default display mode--choose 640x480 at the current
    // pixel depth.  If for some bizarre reason that's not available,
    // fall back to the first mode in the list.
    lastFullScreenMode = 0;
    for (iter = modes->begin(); iter != modes->end(); iter++)
    {
        if (iter->dmPelsWidth == 640 && iter->dmPelsHeight == 480)
        {
            // Add one to the mode index, since mode 0 means windowed mode
            lastFullScreenMode = (iter - modes->begin()) + 1;
            break;
        }
    }
    if (lastFullScreenMode == 0 && modes->size() > 0)
        lastFullScreenMode = 1;

    return modes;
}



int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // Say we're not ready to render yet.
    bReady = false;

    appInstance = hInstance;

    //Specify some default values in case registry keys are not found.
    AppPreferences prefs;
    prefs.winWidth = 800;
    prefs.winHeight = 600;
    prefs.winX = CW_USEDEFAULT;
    prefs.winY = CW_USEDEFAULT;
    prefs.ambientLight = 0.1f;  // Low
    prefs.labelMode = 0;
    prefs.pixelShader = 0;
    prefs.renderFlags = Renderer::ShowAtmospheres | Renderer::ShowStars | Renderer::ShowPlanets;
    prefs.vertexShader = 0;
    prefs.visualMagnitude = 5.0f;   //Default specified in Simulation::Simulation()
    prefs.showLocalTime = 0;
    LoadPreferencesFromRegistry(CelestiaRegKey, prefs);

    // Adjust window dimensions for screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    if (prefs.winWidth > screenWidth)
        prefs.winWidth = screenWidth;
    if (prefs.winHeight > screenHeight)
        prefs.winHeight = screenHeight;
    if (prefs.winX != CW_USEDEFAULT && prefs.winY != CW_USEDEFAULT)
    {
        if (prefs.winX + prefs.winWidth > screenWidth)
            prefs.winX = screenWidth - prefs.winWidth;
        if (prefs.winY + prefs.winHeight > screenHeight)
            prefs.winY = screenHeight - prefs.winHeight;
    }

    bool startFullscreen;
    if (strstr(lpCmdLine, "-fullscreen"))
	    startFullscreen = true;
    else
	    startFullscreen = false;

    joystickAvailable = InitJoystick(joystickCaps);

    displayModes = EnumerateDisplayModes(16);

    appCore = new CelestiaCore();
    if (appCore == NULL)
    {
        MessageBox(NULL,
                   "Out of memory.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
        return false;
    }

    appCore->setAlerter(new WinAlerter());

    if (!appCore->initSimulation())
    {
        return 1;
    }

    menuBar = CreateMenuBar();
    acceleratorTable = LoadAccelerators(hInstance,
                                        MAKEINTRESOURCE(IDR_ACCELERATORS));

    HWND hWnd;
    if (startFullscreen)
    {
        hWnd = CreateOpenGLWindow(0, 0, 800, 600,
                                  lastFullScreenMode, currentScreenMode);
    }
    else
    {
        hWnd = CreateOpenGLWindow(prefs.winX, prefs.winY,
                                  prefs.winWidth, prefs.winHeight,
                                  0, currentScreenMode);
    }

    if (hWnd == NULL)
    {
        MessageBox(NULL,
	           "Failed to create the application window.",
	           "Fatal Error",
	           MB_OK | MB_ICONERROR);
        return FALSE;
    }

    mainWindow = hWnd;

    UpdateWindow(mainWindow);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    if (!appCore->initRenderer())
    {
        return 1;
    }

    // Set values saved in registry: renderFlags, visualMagnitude, labelMode and timezone bias.
    appCore->getSimulation()->setFaintestVisible(prefs.visualMagnitude);
    appCore->getRenderer()->setRenderFlags(prefs.renderFlags);
    appCore->getRenderer()->setLabelMode(prefs.labelMode);
    appCore->getRenderer()->setAmbientLightLevel(prefs.ambientLight);
    appCore->getRenderer()->setFragmentShaderEnabled(prefs.pixelShader == 1);
    appCore->getRenderer()->setVertexShaderEnabled(prefs.vertexShader == 1);
    if (prefs.showLocalTime == 1)
        ShowLocalTime(appCore);
    else
        ShowUniversalTime(appCore);

    BuildFavoritesMenu();
    syncMenusWithRendererState();

    //Gray-out Render menu options that hardware does not support.
    if(!appCore->getRenderer()->fragmentShaderSupported())
        EnableMenuItem(menuBar, ID_RENDER_PIXEL_SHADERS, MF_BYCOMMAND | MF_GRAYED);
    if(!appCore->getRenderer()->vertexShaderSupported())
        EnableMenuItem(menuBar, ID_RENDER_VERTEX_SHADERS, MF_BYCOMMAND | MF_GRAYED);

    appCore->setContextMenuCallback(ContextMenu);

    bReady = true;
    appCore->start((double) time(NULL) / 86400.0 +
                   (double) astro::Date(1970, 1, 1));

    MSG msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
        // Tick the simulation
        appCore->tick();

        // If Celestia is in an inactive state, we should use GetMessage
        // to avoid sucking CPU cycles--if time is paused, we can probably
        // avoid constantly rendering.
        if(PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            bool dialogMessage = false;

            if (starBrowser != NULL &&
                IsDialogMessage(starBrowser->hwnd, &msg))
                dialogMessage = true;
            if (solarSystemBrowser != NULL &&
                IsDialogMessage(solarSystemBrowser->hwnd, &msg))
                dialogMessage = true;
            if (tourGuide != NULL &&
                IsDialogMessage(tourGuide->hwnd, &msg))
                dialogMessage = true;
            if (gotoObjectDlg != NULL &&
                IsDialogMessage(gotoObjectDlg->hwnd, &msg))
                dialogMessage = true;
            if (viewOptionsDlg != NULL &&
                IsDialogMessage(viewOptionsDlg->hwnd, &msg))
                dialogMessage = true;

            // Translate and dispatch the message
            if (!dialogMessage)
            {
                if (!TranslateAccelerator(mainWindow, acceleratorTable, &msg))
                    TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            // And force a redraw
            InvalidateRect(mainWindow, NULL, FALSE);
        }

        if (useJoystick)
            HandleJoystick();

        if (currentScreenMode != newScreenMode)
        {
            if (currentScreenMode == 0)
                GetWindowRect(mainWindow, &windowRect);
            else
                lastFullScreenMode = currentScreenMode;
            DestroyOpenGLWindow();
            mainWindow = CreateOpenGLWindow(windowRect.left,
                                            windowRect.top,
                                            windowRect.right-windowRect.left,
                                            windowRect.bottom-windowRect.top,
                                            newScreenMode,
                                            currentScreenMode);
            UpdateWindow(mainWindow);
        }
    }

    // Save application preferences
    {
        AppPreferences prefs;
        if (GetCurrentPreferences(prefs))
            SavePreferencesToRegistry(CelestiaRegKey, prefs);
    }

    // Not ready to render anymore.
    bReady = false;

    // Clean up the window
    DestroyOpenGLWindow();
    if (currentScreenMode != 0)
        RestoreDisplayMode();

    if (appCore != NULL)
        delete appCore;

    return msg.wParam;
}


bool modifiers(WPARAM wParam, WPARAM mods)
{
    return (wParam & mods) == mods;
}


static void RestoreCursor()
{
    ShowCursor(TRUE);
    cursorVisible = true;
    SetCursorPos(saveCursorPos.x, saveCursorPos.y);
}


LRESULT CALLBACK MainWindowProc(HWND hWnd,
                                UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_CREATE:
        break;

    case WM_MOUSEMOVE:
        {
	    int x, y;
	    x = LOWORD(lParam);
	    y = HIWORD(lParam);
            if ((wParam & (MK_LBUTTON | MK_RBUTTON)) != 0)
            {
#ifdef INFINITE_MOUSE
                // A bit of mouse tweaking here . . .  we want to allow the
                // user to rotate and zoom continuously, without having to
                // pick up the mouse every time it leaves the window.  So,
                // once we start dragging, we'll hide the mouse and reset
                // its position every time it's moved.
                POINT pt;
                pt.x = lastX;
                pt.y = lastY;
                ClientToScreen(hWnd, &pt);

                // If the cursor is still visible, this is the first mouse
                // move message of this drag.  Hide the cursor and set the
                // cursor position to the center of the window.  Once the
                // drag is complete, we'll restore the cursor position and
                // make it visible again.
                if (cursorVisible)
                {
                    // Hide the cursor
                    ShowCursor(FALSE);
                    cursorVisible = false;

                    // Save the cursor position
                    saveCursorPos = pt;

                    // Compute the center point of the client area
                    RECT rect;
                    GetClientRect(hWnd, &rect);
                    POINT center;
                    center.x = (rect.right - rect.left) / 2;
                    center.y = (rect.bottom - rect.top) / 2;

                    // Set the cursor position to the center of the window
                    x = center.x + (x - lastX);
                    y = center.y + (y - lastY);
                    lastX = center.x;
                    lastY = center.y;

                    ClientToScreen(hWnd, &center);
                    SetCursorPos(center.x, center.y);
                }
                else
                {
                    if (x - lastX != 0 || y - lastY != 0)
                        SetCursorPos(pt.x, pt.y);
                }
#else
                lastX = x;
                lastY = y;
#endif // INFINITE_MOUSE
            }

            int buttons = 0;
            if ((wParam & MK_LBUTTON) != 0)
                buttons |= CelestiaCore::LeftButton;
            if ((wParam & MK_RBUTTON) != 0)
                buttons |= CelestiaCore::RightButton;
            if ((wParam & MK_MBUTTON) != 0)
                buttons |= CelestiaCore::MiddleButton;
            if ((wParam & MK_SHIFT) != 0)
                buttons |= CelestiaCore::ShiftKey;
            if ((wParam & MK_CONTROL) != 0)
                buttons |= CelestiaCore::ControlKey;
            appCore->mouseMove(x - lastX, y - lastY, buttons);
        }
        break;

    case WM_LBUTTONDOWN:
        lastX = LOWORD(lParam);
        lastY = HIWORD(lParam);
        appCore->mouseButtonDown(LOWORD(lParam), HIWORD(lParam),
                                 CelestiaCore::LeftButton);
        break;
    case WM_RBUTTONDOWN:
        lastX = LOWORD(lParam);
        lastY = HIWORD(lParam);
        appCore->mouseButtonDown(LOWORD(lParam), HIWORD(lParam),
                                 CelestiaCore::RightButton);
        break;
    case WM_MBUTTONDOWN:
        lastX = LOWORD(lParam);
        lastY = HIWORD(lParam);
        appCore->mouseButtonDown(LOWORD(lParam), HIWORD(lParam),
                                 CelestiaCore::MiddleButton);
        break;

    case WM_LBUTTONUP:
        if (!cursorVisible)
            RestoreCursor();
        appCore->mouseButtonUp(LOWORD(lParam), HIWORD(lParam),
                               CelestiaCore::LeftButton);
        break;

    case WM_RBUTTONUP:
        if (!cursorVisible)
            RestoreCursor();
        appCore->mouseButtonUp(LOWORD(lParam), HIWORD(lParam),
                               CelestiaCore::RightButton);
        break;

    case WM_MOUSEWHEEL:
        {
            int modifiers = 0;
            if ((wParam & MK_SHIFT) != 0)
                modifiers |= CelestiaCore::ShiftKey;
            appCore->mouseWheel((short) HIWORD(wParam) > 0 ? -1.0f : 1.0f,
                                modifiers);
        }
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            appCore->charEntered('\033');
            break;
        default:
            handleKey(wParam, true);
            break;
        }
        break;

    case WM_KEYUP:
        handleKey(wParam, false);
        break;

    case WM_CHAR:
        {
             // Bits 16-23 of lParam specify the scan code of the key pressed.
             // Ignore all keypad input, this will be handled by WM_KEYDOWN
             // messages.
             char cScanCode = (char) (HIWORD(lParam) & 0xFF);
             if((cScanCode >= 71 && cScanCode <= 73) ||
                (cScanCode >= 75 && cScanCode <= 77) ||
                (cScanCode >= 79 && cScanCode <= 83))
                 break;

            Renderer* r = appCore->getRenderer();
            int oldRenderFlags = r->getRenderFlags();
            int oldLabelMode = r->getLabelMode();
            bool oldFragmentShaderState = r->getFragmentShaderEnabled();
            bool oldVertexShaderState = r->getVertexShaderEnabled();
            appCore->charEntered((char) wParam);
            if (r->getRenderFlags() != oldRenderFlags ||
                r->getLabelMode() != oldLabelMode ||
                r->getFragmentShaderEnabled() != oldFragmentShaderState ||
                r->getVertexShaderEnabled() != oldVertexShaderState)
            {
                syncMenusWithRendererState();
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_NAVIGATION_CENTER:
            appCore->charEntered('C');
            break;
        case ID_NAVIGATION_GOTO:
            appCore->charEntered('G');
            break;
        case ID_NAVIGATION_FOLLOW:
            appCore->charEntered('F');
            break;
        case ID_NAVIGATION_SYNCORBIT:
            appCore->charEntered('Y');
            break;
        case ID_NAVIGATION_TRACK:
            appCore->charEntered('T');
            break;
        case ID_NAVIGATION_HOME:
            appCore->charEntered('H');
            break;
        case ID_NAVIGATION_SELECT:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_FINDOBJECT), hWnd, FindObjectProc);
            break;
        case ID_NAVIGATION_GOTO_OBJECT:
            if (gotoObjectDlg == NULL)
                gotoObjectDlg = new GotoObjectDialog(appInstance, hWnd, appCore);
            break;
        case IDCLOSE:
            if (reinterpret_cast<LPARAM>(gotoObjectDlg) == lParam &&
                gotoObjectDlg != NULL)
            {
                delete gotoObjectDlg;
                gotoObjectDlg = NULL;
            }
            else if (reinterpret_cast<LPARAM>(tourGuide) == lParam &&
                     tourGuide != NULL)
            {
                delete tourGuide;
                tourGuide = NULL;
            }
            else if (reinterpret_cast<LPARAM>(starBrowser) == lParam &&
                     starBrowser != NULL)
            {
                delete starBrowser;
                starBrowser = NULL;
            }
            else if (reinterpret_cast<LPARAM>(solarSystemBrowser) == lParam &&
                     solarSystemBrowser != NULL)
            {
                delete solarSystemBrowser;
                solarSystemBrowser = NULL;
            }
            else if (reinterpret_cast<LPARAM>(viewOptionsDlg) == lParam &&
                viewOptionsDlg != NULL)
            {
                delete viewOptionsDlg;
                viewOptionsDlg = NULL;
            }
            break;

        case ID_NAVIGATION_TOURGUIDE:
            if (tourGuide == NULL)
                tourGuide = new TourGuide(appInstance, hWnd, appCore);
            break;

        case ID_NAVIGATION_SSBROWSER:
            if (solarSystemBrowser == NULL)
                solarSystemBrowser = new SolarSystemBrowser(appInstance, hWnd, appCore);
            break;

        case ID_NAVIGATION_STARBROWSER:
            if (starBrowser == NULL)
                starBrowser = new StarBrowser(appInstance, hWnd, appCore);
            break;

        case ID_RENDER_DISPLAYMODE:
            newScreenMode = currentScreenMode;
            CreateDialogParam(appInstance,
                              MAKEINTRESOURCE(IDD_DISPLAYMODE),
                              hWnd,
                              SelectDisplayModeProc,
                              NULL);
            break;

        case ID_RENDER_FULLSCREEN:
            if (currentScreenMode == 0)
                newScreenMode = lastFullScreenMode;
            else
                newScreenMode = 0;
            break;

        case ID_RENDER_VIEWOPTIONS:
            if (viewOptionsDlg == NULL)
                viewOptionsDlg = new ViewOptionsDialog(appInstance, hWnd, appCore);
            break;

        case ID_RENDER_MORESTARS:
            appCore->charEntered(']');
            break;

        case ID_RENDER_FEWERSTARS:
            appCore->charEntered('[');
            break;

        case ID_RENDER_AMBIENTLIGHT_NONE:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_CHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
            appCore->getRenderer()->setAmbientLightLevel(0.0f);
            break;
        case ID_RENDER_AMBIENTLIGHT_LOW:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_CHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
            appCore->getRenderer()->setAmbientLightLevel(0.1f);
            break;
        case ID_RENDER_AMBIENTLIGHT_MEDIUM:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_CHECKED);
            appCore->getRenderer()->setAmbientLightLevel(0.25f);
            break;

        case ID_RENDER_PIXEL_SHADERS:
            appCore->charEntered('\020');
            syncMenusWithRendererState();
            break;
        case ID_RENDER_VERTEX_SHADERS:
            appCore->charEntered('\026');
            syncMenusWithRendererState();
            break;

        case ID_TIME_FASTER:
            appCore->charEntered('L');
            break;
        case ID_TIME_SLOWER:
            appCore->charEntered('K');
            break;
        case ID_TIME_REALTIME:
            appCore->charEntered('\\');
            break;

        case ID_TIME_FREEZE:
            appCore->charEntered(' ');
            break;
        case ID_TIME_REVERSE:
            appCore->charEntered('J');
            break;
        case ID_TIME_SETTIME:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_SETTIME), hWnd, SetTimeProc);
            break;
        case ID_TIME_SHOWLOCAL:
            if (ToggleMenuItem(menuBar, ID_TIME_SHOWLOCAL))
                ShowLocalTime(appCore);
            else
                ShowUniversalTime(appCore);
            break;

        case ID_LOCATIONS_ADDLOCATION:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_ADDLOCATION), hWnd, AddLocationProc);
            break;

        case ID_HELP_RUNDEMO:
            appCore->charEntered('D');
            break;
            
        case ID_HELP_CONTROLS:
            CreateDialogParam(appInstance,
                              MAKEINTRESOURCE(IDD_CONTROLSHELP),
                              hWnd,
                              ControlsHelpProc,
                              NULL);
            break;

        case ID_HELP_ABOUT:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutProc);
            break;

        case ID_HELP_GLINFO:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_GLINFO), hWnd, GLInfoProc);
            break;

        case ID_HELP_LICENSE:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_LICENSE), hWnd, LicenseProc);
            break;

        case ID_INFO:
            ShowWWWInfo(appCore->getSimulation()->getSelection());
            break;

        case ID_FILE_CAPTUREIMAGE:
            HandleCaptureImage(hWnd);
            break;

        case ID_FILE_CAPTUREMOVIE:
            HandleCaptureMovie(hWnd);
            break;

        case ID_FILE_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        default:
            {
                const FavoritesList* favorites = appCore->getFavorites();
                if (favorites != NULL &&
                    LOWORD(wParam) - ID_LOCATIONS_FIRSTLOCATION < favorites->size())
                {
                    int whichFavorite = LOWORD(wParam) - ID_LOCATIONS_FIRSTLOCATION;
                    appCore->activateFavorite(*(*favorites)[whichFavorite]);
                }
                else if (LOWORD(wParam) >= MENU_CHOOSE_PLANET && 
                         LOWORD(wParam) < MENU_CHOOSE_PLANET + 1000)
                {
                    Selection sel = appCore->getSimulation()->getSelection();
                    if(sel.star)
                    {
                        appCore->getSimulation()->selectPlanet(LOWORD(wParam) - MENU_CHOOSE_PLANET);
                    }
                    else if(sel.body)
                    {
                        PlanetarySystem* satellites = (PlanetarySystem*)sel.body->getSatellites();
                        appCore->getSimulation()->setSelection(Selection(satellites->getBody(LOWORD(wParam) - MENU_CHOOSE_PLANET)));
                    }
                    else if(sel.galaxy)
                    {
                        //Current Galaxy implementation does not have children to select.
                    }
                }
            }
            break;
        }
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        appCore->resize(LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_PAINT:
        if (bReady)
        {
            appCore->draw();
            SwapBuffers(deviceContext);
            ValidateRect(hWnd, NULL);
        }
        break;

    default:
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}
