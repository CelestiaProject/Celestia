// winmain.cpp
// 
// Copyright (C) 2000, Chris Laurel <claurel@shatters.net>
//
// Windows front end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include "gl.h"
#include "celestia.h"
#include "vecmath.h"
#include "quaternion.h"
#include "util.h"
#include "stardb.h"
#include "solarsys.h"
#include "asterism.h"
#include "mathlib.h"
#include "astro.h"
#include "overlay.h"
#include "config.h"
#include "favorites.h"
#include "simulation.h"
#include "execution.h"
#include "cmdparser.h"

#include "../res/resource.h"


//----------------------------------
// Skeleton functions and variables.
//-----------------
char AppName[] = "Celestia";

static string welcomeMessage("Welcome to Celestia 1.07");


//----------------------------------
// Timer info.
static LARGE_INTEGER TimerFreq;	// Timer Frequency.
static LARGE_INTEGER TimeStart;	// Time of start.
static LARGE_INTEGER TimeCur;	// Current time.
static double currentTime=0.0;
static double deltaTime=0.0;

static bool fullscreen;

// Mouse motion tracking
static int lastX = 0;
static int lastY = 0;
static int mouseMotion = 0;
static double mouseWheelTime = -1000.0;
static float mouseWheelMotion = 0.0f;

static bool upPress = false;
static bool downPress = false;
static bool leftPress = false;
static bool rightPress = false;
static bool pgupPress = false;
static bool pgdnPress = false;

static bool wireframe = false;

static bool paused = false;
static double timeScale = 0.0;

static bool textEnterMode = false;
static string typedText = "";
static bool editMode = false;

static int hudDetail = 1;

static CelestiaConfig* config = NULL;

static StarDatabase* starDB = NULL;
static StarNameDatabase* starNameDB = NULL;
static SolarSystemCatalog* solarSystemCatalog = NULL;
static GalaxyList* galaxies = NULL;
static AsterismList* asterisms = NULL;
static bool showAsterisms = false;

static FavoritesList* favorites = NULL;

static Simulation* sim = NULL;
static Renderer* renderer = NULL;
static Overlay* overlay = NULL;
static TexFont* font = NULL;

static CommandSequence* script = NULL;
static Execution* runningScript = NULL;

HINSTANCE appInstance;

static HMENU menuBar = 0;
static HACCEL acceleratorTable = 0;

bool cursorVisible = true;

astro::Date newTime(0.0);

#define INFINITE_MOUSE

#define ROTATION_SPEED  6
#define ACCELERATION    20.0f


#define MENU_CHOOSE_PLANET   32000


// Good 'ol generic drawing stuff.
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



bool ReadStars(string starsFileName, string namesFileName)
{
    ifstream starFile(starsFileName.c_str(), ios::in | ios::binary);
    if (!starFile.good())
    {
	cerr << "Error opening " << starsFileName << '\n';
        return false;
    }

    ifstream starNamesFile(namesFileName.c_str(), ios::in);
    if (!starNamesFile.good())
    {
	cerr << "Error opening " << namesFileName << '\n';
        return false;
    }

    starDB = StarDatabase::read(starFile);
    if (starDB == NULL)
    {
	cerr << "Error reading stars file\n";
        return false;
    }

    starNameDB = StarDatabase::readNames(starNamesFile);
    if (starNameDB == NULL)
    {
        cerr << "Error reading star names file\n";
        return false;
    }

    starDB->setNameDatabase(starNameDB);

    return true;
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


static void SetFaintest(float magnitude)
{
    renderer->setBrightnessBias(0.0f);
    renderer->setBrightnessScale(1.0f / (magnitude + 1.0f));
    sim->setFaintestVisible(magnitude);
}


static void WriteFavoritesFile()
{
    if (config->favoritesFile != "")
    {
        ofstream out(config->favoritesFile.c_str(), ios::out);
        if (out.good())
        WriteFavoritesList(*favorites, out);
    }
}

static void ActivateFavorite(FavoritesEntry& fav)
{
    sim->cancelMotion();
    sim->setTime(fav.jd);
    sim->getObserver().setPosition(fav.position);
    sim->getObserver().setOrientation(fav.orientation);
}

static void AddFavorite(string name)
{
    FavoritesEntry* fav = new FavoritesEntry();
    fav->jd = sim->getTime();
    fav->position = sim->getObserver().getPosition();
    fav->orientation = sim->getObserver().getOrientation();
    fav->name = name;
    favorites->insert(favorites->end(), fav);
    WriteFavoritesFile();
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
    ifstream textFile(filename, ios::in | ios::binary);
    string s;

    char c;
    while (textFile.get(c))
    {
        if (c == '\012' || c == '\014')
            s += "\r\r\n";
        else
            s += c;
    }

    if (textFile.bad())
        return false;

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


BOOL APIENTRY LicenseProc(HWND hDlg,
                          UINT message,
                          UINT wParam,
                          LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        LoadItemTextFromFile(hDlg, IDC_LICENSE_TEXT, "License.txt");
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
                sim->selectBody(string(buf));
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

                AddFavorite(name);
                AppendLocationToMenu(name, favorites->size() - 1);
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


BOOL APIENTRY SetTimeProc(HWND hDlg,
                          UINT message,
                          UINT wParam,
                          LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            SYSTEMTIME sysTime;
            newTime = astro::Date(sim->getTime());
            sysTime.wYear = newTime.year;
            sysTime.wMonth = newTime.month;
            sysTime.wDay = newTime.day;
            sysTime.wDayOfWeek = ((int) ((double) newTime + 0.5) + 1) % 7;
            sysTime.wHour = newTime.hour;
            sysTime.wMinute = newTime.minute;
            sysTime.wSecond = (int) newTime.seconds;
            sysTime.wMilliseconds = 0;

            HWND hwnd = GetDlgItem(hDlg, IDC_DATEPICKER);
            if (hwnd != NULL)
            {
                DateTime_SetFormat(hwnd, "dd' 'MMM' 'yyy");
                DateTime_SetSystemtime(hwnd, GDT_VALID, &sysTime);
            }
            hwnd = GetDlgItem(hDlg, IDC_TIMEPICKER);
            if (hwnd != NULL)
            {
                DateTime_SetFormat(hwnd, "HH':'mm':'ss' UT'");
                DateTime_SetSystemtime(hwnd, GDT_VALID, &sysTime);
            }
        }
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
                sim->setTime((double) newTime);
            EndDialog(hDlg, 0);
            return TRUE;
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
                    astro::Date date(change->st.wYear, change->st.wMonth, change->st.wDay);
                    newTime.year = change->st.wYear;
                    newTime.month = change->st.wMonth;
                    newTime.day = change->st.wDay;
                    newTime.hour = change->st.wHour;
                    newTime.minute = change->st.wMinute;
                    newTime.seconds = change->st.wSecond + (double) change->st.wMilliseconds / 1000.0;
                }
            }
        }
    }

    return FALSE;
}



HMENU CreateMenuBar()
{
    return LoadMenu(appInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));
}


static void ToggleLabelState(int menuItem, int labelState)
{
    if ((GetMenuState(menuBar, menuItem, MF_BYCOMMAND) & MF_CHECKED) == 0)
    {
        renderer->setLabelMode(renderer->getLabelMode() | labelState);
        CheckMenuItem(menuBar, menuItem, MF_CHECKED);
    }
    else
    {
        renderer->setLabelMode(renderer->getLabelMode() & ~labelState);
        CheckMenuItem(menuBar, menuItem, MF_UNCHECKED);
    }
}


static bool ToggleMenuItem(int menuItem)
{
    if ((GetMenuState(menuBar, menuItem, MF_BYCOMMAND) & MF_CHECKED) == 0)
    {
        CheckMenuItem(menuBar, menuItem, MF_CHECKED);
        return true;
    }
    else
    {
        CheckMenuItem(menuBar, menuItem, MF_UNCHECKED);
        return false;
    }
}


VOID APIENTRY handlePopupMenu(HWND hwnd, POINT point,
                              const SolarSystem& solarSystem)
{
    HMENU hMenu;

    hMenu = CreatePopupMenu();
    PlanetarySystem* planets = solarSystem.getPlanets();
    for (int i = 0; i < planets->getSystemSize(); i++)
    {
        Body* body = planets->getBody(i);
        AppendMenu(hMenu, MF_STRING, MENU_CHOOSE_PLANET + i,
                   body->getName().c_str());
    }

    if (!fullscreen)
        ClientToScreen(hwnd, (LPPOINT) &point);

    TrackPopupMenu (hMenu, 0, point.x, point.y, 0, hwnd, NULL);

    DestroyMenu (hMenu);
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


VOID APIENTRY handlePopupMenu(HWND hwnd, POINT point,
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
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, "&Info");

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
        name = starDB->getStarName(sel.star->getCatalogNumber());
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, name.c_str());
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, "&Goto");
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, "&Info");

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

    ClientToScreen(hwnd, (LPPOINT) &point);

    sim->setSelection(sel);
    TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hwnd, NULL);

    // TODO: Do we need to explicitly destroy submenus or does DestroyMenu
    // work recursively?
    DestroyMenu(hMenu);
}


// Used in the super-secret edit mode
void ShowSelectionInfo(const Selection& sel)
{
    if (sel.galaxy != NULL)
    {
        cout << sel.galaxy->getName() << '\n';
        Vec3f axis;
        float angle;
        sel.galaxy->getOrientation().getAxisAngle(axis, angle);
        cout << "Orientation: " << '[' << axis.x << ',' << axis.y << ',' << axis.z << "], " << radToDeg(angle) << '\n';
    }
}

void handleKey(WPARAM key, bool down)
{
    switch (key) {
    case VK_UP:
	upPress = down;
	break;
    case VK_DOWN:
	downPress = down;
	break;
    case VK_LEFT:
	leftPress = down;
	break;
    case VK_RIGHT:
	rightPress = down;
	break;
    case VK_HOME:
	pgupPress = down;
	break;
    case VK_END:
	pgdnPress = down;
	break;
    }
}

void handleKeyPress(int c)
{
    if (textEnterMode)
    {
        if (c == ' ' || isalpha(c) || isdigit(c) || ispunct(c))
        {
            typedText += c;
        }
        else if (c == '\b')
        {
            if (typedText.size() > 0)
                typedText = string(typedText, 0, typedText.size() - 1);
        }
        return;
    }

    c = toupper(c);
    switch (c)
    {
    case 'A':
        if (sim->getTargetSpeed() == 0)
            sim->setTargetSpeed(0.000001f);
        else
            sim->setTargetSpeed(sim->getTargetSpeed() * 10.0f);
        break;
    case 'Z':
        sim->setTargetSpeed(sim->getTargetSpeed() * 0.1f);
        break;

    case 'S':
        sim->setTargetSpeed(0);
        break;

    case 'Q':
        sim->setTargetSpeed(-sim->getTargetSpeed());
        break;

    case 'X':
        sim->setTargetSpeed(sim->getTargetSpeed());
        break;

    case 'G':
        sim->gotoSelection();
        break;

    case 'C':
        sim->centerSelection();
        break;

    case 'F':
        sim->follow();
        break;

    case 'H':
        sim->selectStar(0);
        break;

    case 'V':
        sim->setHUDDetail((sim->getHUDDetail() + 1) % 2);
        hudDetail = 1 - hudDetail;
        break;

    case ',':
        if (renderer->getFieldOfView() > 1.0f)
            renderer->setFieldOfView(renderer->getFieldOfView() / 1.1f);
        break;

    case '.':
        if (renderer->getFieldOfView() < 120.0f)
            renderer->setFieldOfView(renderer->getFieldOfView() * 1.1f);
        break;

    case 'K':
        sim->setTimeScale(0.1 * sim->getTimeScale());
        break;

    case 'L':
        sim->setTimeScale(10.0 * sim->getTimeScale());
        break;

    case 'J':
        sim->setTimeScale(-sim->getTimeScale());
        break;

    case 'B':
        ToggleLabelState(ID_RENDER_SHOWSTARLABELS, Renderer::StarLabels);
        break;

    case 'N':
        ToggleLabelState(ID_RENDER_SHOWPLANETLABELS, Renderer::PlanetLabels);
        break;

    case 'O':
        ToggleLabelState(ID_RENDER_SHOWORBITS, Renderer::PlanetOrbits);
        break;

    case 'P':
        if (renderer->perPixelLightingSupported())
        {
            bool enabled = !renderer->getPerPixelLighting();
            CheckMenuItem(menuBar, ID_RENDER_PERPIXEL_LIGHTING,
                          enabled ? MF_CHECKED : MF_UNCHECKED);
            renderer->setPerPixelLighting(enabled);
        }
        break;

    case 'I':
        {
            bool enabled = !renderer->getCloudMapping();
            CheckMenuItem(menuBar, ID_RENDER_SHOWATMOSPHERES,
                          enabled ? MF_CHECKED : MF_UNCHECKED);
            renderer->setCloudMapping(enabled);
        }
        break;

    case '/':
        showAsterisms = !showAsterisms;
        CheckMenuItem(menuBar, ID_RENDER_SHOWCONSTELLATIONS,
                      showAsterisms ? MF_CHECKED : MF_UNCHECKED);
        renderer->showAsterisms(showAsterisms ? asterisms : NULL);
        break;

    case '~':
        editMode = !editMode;
        break;

    case '!':
        if (editMode)
            ShowSelectionInfo(sim->getSelection());
        break;

#if 0
    case 'Y':
        if (runningScript == NULL)
            runningScript = new Execution(*script, sim, renderer);
        break;
#endif

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        sim->selectPlanet(c - '1');
        break;

    case '0':
        sim->selectPlanet(-1);
        break;

    case 'W':
        wireframe = !wireframe;
        renderer->setRenderMode(wireframe ? GL_LINE : GL_FILL);
        break;

    case '[':
        if (sim->getFaintestVisible() > 1.0f)
            SetFaintest(sim->getFaintestVisible() - 0.5f);
        break;

    case ']':
        if (sim->getFaintestVisible() < 8.0f)
            SetFaintest(sim->getFaintestVisible() + 0.5f);
        break;

    case ' ':
        if (paused)
        {
            sim->setTimeScale(timeScale);
            CheckMenuItem(menuBar, ID_TIME_FREEZE, MF_UNCHECKED);
        }
        else
        {
            timeScale = sim->getTimeScale();
            sim->setTimeScale(0.0);
            CheckMenuItem(menuBar, ID_TIME_FREEZE, MF_CHECKED);
        }
        paused = !paused;
        break;
    }
}


// Select the pixel format for a given device context
void SetDCPixelFormat(HDC hDC)
{
    int nPixelFormat;

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
    nPixelFormat = ChoosePixelFormat(hDC, &pfd);

    // Set the pixel format for the device context
    SetPixelFormat(hDC, nPixelFormat, &pfd);
}


void ChangeSize(GLsizei w, GLsizei h)
{
    if (h == 0)
	h = 1;

    glViewport(0, 0, w, h);
    if (renderer != NULL)
        renderer->resize(w, h);
    if (overlay != NULL)
        overlay->setWindowSize(w, h);
}


GLsizei g_w, g_h;
bool bReady = false;


void RenderOverlay()
{
    if (font == NULL)
        return;

    overlay->begin();

    // Time and date
    if (hudDetail > 0)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        glTranslatef(g_w - 130, g_h - 15, 0);
        overlay->beginText();
        *overlay << astro::Date(sim->getTime()) << '\n';
        if (paused)
        {
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            *overlay << "Paused";
        }
        else
        {
            double timeScale = sim->getTimeScale();
            if (abs(timeScale - 1) < 1e-6)
                *overlay << "Real time";
            else if (timeScale > 1.0)
                *overlay << timeScale << "x faster";
            else
                *overlay << 1.0 / timeScale << "x slower";
        }
        overlay->endText();
        glPopMatrix();
    }

    // Speed
    if (hudDetail > 0)
    {
        glPushMatrix();
        glTranslatef(0, 20, 0);
        overlay->beginText();

        double speed = sim->getObserver().getVelocity().length();
        char* units;
        if (speed < astro::AUtoLightYears(1000))
        {
            if (speed < astro::kilometersToLightYears(10000000.0f))
            {
                speed = astro::lightYearsToKilometers(speed);
                units = "km/s";
            }
            else
            {
                speed = astro::lightYearsToAU(speed);
                units = "AU/s";
            }
        }
        else
        {
            units = "ly/s";
        }

        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        *overlay << "\nSpeed: " << speed << ' ' << units;

        overlay->endText();
        glPopMatrix();
    }

    // Field of view and camera mode
    if (hudDetail > 0)
    {
        float fov = renderer->getFieldOfView();
        // fov = ((int) (fov * 1000)) / 1000.0f;

        Simulation::ObserverMode mode = sim->getObserverMode();
        char* modeName = "";
        if (mode == Simulation::Travelling)
            modeName = "Travelling";
        else if (mode == Simulation::Following)
            modeName = "Following";

        glPushMatrix();
        glTranslatef(g_w - 130, 20, 0);
        overlay->beginText();
        glColor4f(0.6f, 0.6f, 1.0f, 1);
        *overlay << modeName << '\n';
        glColor4f(0.7f, 0.7f, 1.0f, 1.0f);
        overlay->printf("FOV: %6.2f\n", fov);
        overlay->endText();
        glPopMatrix();
    }

    // Text input
    if (textEnterMode)
    {
        glPushMatrix();
        glColor4f(0.7f, 0.7f, 1.0f, 0.2f);
        overlay->rect(0, 0, g_w, 70);
        glTranslatef(0, 50, 0);
        glColor4f(0.6f, 0.6f, 1.0f, 1);
        *overlay << "Target name: " << typedText;
        glPopMatrix();
    }

    // Intro message
    if (currentTime < 5.0)
    {
        int width = 0, maxAscent = 0, maxDescent = 0;
        txfGetStringMetrics(font, welcomeMessage, width, maxAscent, maxDescent);
        glPushMatrix();
        glTranslatef((g_w - width) / 2, g_h / 2, 0);

        float alpha = 1.0f;
        if (currentTime > 3.0)
            alpha = 0.5f * (float) (5.0 - currentTime);
        glColor4f(1, 1, 1, alpha);
        *overlay << welcomeMessage;
        glPopMatrix();
    }

    if (editMode)
    {
        int width = 0, maxAscent = 0, maxDescent = 0;
        txfGetStringMetrics(font, "Edit Mode", width, maxAscent, maxDescent);
        glPushMatrix();
        glTranslatef((g_w - width) / 2, g_h - 15, 0);
        glColor4f(1, 0, 1, 1);
        *overlay << "Edit Mode";
        glPopMatrix();
    }

    overlay->end();
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // Say we're not ready to render yet.
    bReady = false;

    appInstance = hInstance;

    // Setup the window class.
    WNDCLASS wc;
    HWND hWnd;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CELESTIA_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = AppName;

    if (strstr(lpCmdLine, "-fullscreen"))
	fullscreen = true;
    else
	fullscreen = false;

    if (RegisterClass(&wc) == 0)
    {
	MessageBox(NULL,
                   "Failed to register the window class.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
	return FALSE;
    }

    // Check for the presence of the license file--don't run unless it's there.
    {
        ifstream license("License.txt");
        if (!license.good())
        {
            MessageBox(NULL,
                       "License file 'License.txt' is missing!", "Fatal Error",
                       MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    config = ReadCelestiaConfig("celestia.cfg");
    if (config == NULL)
    {
        MessageBox(NULL,
                   "Error reading configuration file.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Set up favorites list
    if (config->favoritesFile != "")
    {
        ifstream in(config->favoritesFile.c_str(), ios::in);

        if (in.good())
        {
            favorites = ReadFavoritesList(in);
            if (favorites == NULL)
            {
                MessageBox(NULL,
                           "Error reading favorites file.", "Warning",
                           MB_OK | MB_ICONERROR);
            }
        }
    }

    // If we couldn't read the favorites list from a file, allocate
    // an empty list.
    if (favorites == NULL)
        favorites = new FavoritesList();

    if (!ReadStars(config->starDatabaseFile, config->starNamesFile))
    {
        MessageBox(NULL,
                   "Cannot read star database",
                   "Error",
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    solarSystemCatalog = new SolarSystemCatalog();
    {
        for (vector<string>::const_iterator iter = config->solarSystemFiles.begin();
             iter != config->solarSystemFiles.end();
             iter++)
        {
            ifstream solarSysFile(iter->c_str(), ios::in);
            if (!solarSysFile.good())
            {
                cout << "Error opening " << iter->c_str() << '\n';
            }
            else
            {
                ReadSolarSystems(solarSysFile, *starDB, *solarSystemCatalog);
            }
        }
    }

    if (config->galaxyCatalog != "")
    {
        ifstream galaxiesFile(config->galaxyCatalog.c_str(), ios::in);
        if (!galaxiesFile.good())
        {
            cout << "Error opening galaxies file " << config->galaxyCatalog << '\n';
        }
        else
        {
            galaxies = ReadGalaxyList(galaxiesFile);
        }
    }

    if (config->asterismsFile != "")
    {
        ifstream asterismsFile(config->asterismsFile.c_str(), ios::in);
        if (!asterismsFile.good())
        {
            cout << "Error opening asterisms file " << config->asterismsFile << '\n';
        }
        else
        {
            asterisms = ReadAsterismList(asterismsFile, *starDB);
        }
    }

    sim = new Simulation();
    sim->setStarDatabase(starDB, solarSystemCatalog, galaxies);
    sim->setFaintestVisible(config->faintestVisible);

    // Set the simulation starting time to the current system time
    sim->setTime((double) time(NULL) / 86400.0 + (double) astro::Date(1970, 1, 1));
    sim->update(0.0);

    if (!fullscreen)
    {
        
        menuBar = CreateMenuBar();
        acceleratorTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));

	hWnd = CreateWindow(AppName,
			    AppName,
			    WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
			    CW_USEDEFAULT, CW_USEDEFAULT,
			    800, 600,
			    NULL,
                            menuBar,
			    hInstance,
			    NULL );
    }
    else
    {
	hWnd = CreateWindow(AppName,
			    AppName,
			    WS_POPUP,
			    0, 0,
			    800, 600,
			    NULL, NULL,
			    hInstance,
			    NULL );
    }

    if (hWnd == NULL)
    {
	MessageBox(NULL,
		   "Failed to create the application window.",
		   "Fatal Error",
		   MB_OK | MB_ICONERROR);
	return FALSE;
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    // Reset the timer
    QueryPerformanceFrequency(&TimerFreq);
    QueryPerformanceCounter(&TimeStart);

    renderer = new Renderer();

    // Prepare the scene for rendering.
    if (!renderer->init((int) g_w, (int) g_h)) {
	MessageBox(hWnd,
		   "Failed to initialize",
		   "Fatal Error",
		   MB_OK | MB_ICONERROR);
	return FALSE;
    }

    if (renderer->perPixelLightingSupported())
    {
        renderer->setPerPixelLighting(true);
        CheckMenuItem(menuBar, ID_RENDER_PERPIXEL_LIGHTING, MF_CHECKED);
    }

    // Set up the star labels
    for (vector<string>::const_iterator iter = config->labelledStars.begin();
         iter != config->labelledStars.end();
         iter++)
    {
        Star* star = starDB->find(*iter);
        if (star != NULL)
            renderer->addLabelledStar(star);
    }

    renderer->setBrightnessBias(0.0f);
    renderer->setBrightnessScale(1.0f / (config->faintestVisible + 1.0f));

    // Set up the overlay
    overlay = new Overlay();
    overlay->setWindowSize(g_w, g_h);
    font = txfLoadFont("fonts\\default.txf");
    if (font != NULL)
    {
        txfEstablishTexture(font, 0, GL_FALSE);
        overlay->setFont(font);
    }

    // Add favorites to locations menu
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

    if (config->initScriptFile != "")
    {
        ifstream scriptfile(config->initScriptFile.c_str());
        CommandParser parser(scriptfile);
        script = parser.parse();
        if (script == NULL)
        {
            const vector<string>* errors = parser.getErrors();
            for_each(errors->begin(), errors->end(), printlineFunc<string>(cout));
        }
        else
        {
            runningScript = new Execution(*script, sim, renderer);
        }
    }

    bReady = true;

    // Usual running around in circles bit...
    int bGotMsg;
    MSG  msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
	// Use PeekMessage() if the app is active, so we can use idle time to
	// render the scene.  Else, use GetMessage() to avoid eating CPU time.
	bGotMsg = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);

	if (bGotMsg)
        {
	    // Translate and dispatch the message
            if (!TranslateAccelerator(hWnd, acceleratorTable, &msg))
                TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
        else
        {
	    InvalidateRect(hWnd, NULL, FALSE);
	}
    }

    // Not ready to render anymore.
    bReady = false;

    // Nuke all applicable scene stuff.
    renderer->shutdown();

    return msg.wParam;
}


bool modifiers(WPARAM wParam, WPARAM mods)
{
    return (wParam & mods) == mods;
}

LRESULT CALLBACK MainWindowProc(HWND hWnd,
                                UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{

    static HGLRC hRC;
    static HDC hDC;

    switch(uMsg) {
    case WM_CREATE:
	hDC = GetDC(hWnd);
	if(fullscreen)
	    ChangeDisplayMode();
	SetDCPixelFormat(hDC);
	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);
	break;

    case WM_MOUSEMOVE:
        if ((wParam & (MK_LBUTTON | MK_RBUTTON)) != 0)
	{
	    int x, y;
	    x = LOWORD(lParam);
	    y = HIWORD(lParam);

            if (editMode && modifiers(wParam, MK_LBUTTON | MK_SHIFT | MK_CONTROL))
            {
                // Rotate the selected object
                Selection sel = sim->getSelection();
                Quatf q(1);
                if (sel.galaxy != NULL)
                    q = sel.galaxy->getOrientation();

                q.yrotate((float) (x - lastX) / g_w);
                q.xrotate((float) (y - lastY) / g_h);

                if (sel.galaxy != NULL)
                    sel.galaxy->setOrientation(q);
            }
            else if (modifiers(wParam, MK_LBUTTON | MK_RBUTTON) ||
                     modifiers(wParam, MK_LBUTTON | MK_CONTROL))
            {
                float amount = (float) (lastY - y) / g_h;
                sim->changeOrbitDistance(amount * 5);
            }
            else if (modifiers(wParam, MK_LBUTTON | MK_SHIFT))
            {
                // Zoom control
                float amount = (float) (lastY - y) / g_h;
                float minFOV = 0.01f;
                float maxFOV = 120.0f;
                float fov = renderer->getFieldOfView();

                if (fov < minFOV)
                    fov = minFOV;

                // In order for the zoom to have the right feel, it should be
                // exponential.
                float newFOV = minFOV + (float) exp(log(fov - minFOV) + amount * 4);
                if (newFOV < minFOV)
                    newFOV = minFOV;
                else if (newFOV > maxFOV)
                    newFOV = maxFOV;
                renderer->setFieldOfView(newFOV);
            }
            else
            {
                Quatf q(1);
                // For a small field of view, rotate the camera more finely
                float coarseness = renderer->getFieldOfView() / 30.0f;
                q.yrotate((float) (x - lastX) / g_w * coarseness);
                q.xrotate((float) (y - lastY) / g_h * coarseness);
                if ((wParam & MK_RBUTTON) != 0)
                    sim->orbit(~q);
                else
                    sim->rotate(q);
            }

            mouseMotion += abs(x - lastX) + abs(y - lastY);

#ifdef INFINITE_MOUSE
            // A bit of mouse tweaking here . . .  we want to allow the user to
            // rotate and zoom continuously, without having to pick up the mouse
            // every time it leaves the window.  So, once we start dragging, we'll
            // hide the mouse and reset it's position every time it's moved.
            POINT pt;
            pt.x = lastX;
            pt.y = lastY;
            ClientToScreen(hWnd, &pt);
            if (x - lastX != 0 || y - lastY != 0)
                SetCursorPos(pt.x, pt.y);
            if (cursorVisible)
            {
                ShowCursor(FALSE);
                cursorVisible = false;
            }
#else
            lastX = x;
            lastY = y;
#endif // INFINITE_MOUSE
	}
	break;

    case WM_LBUTTONDOWN:
	lastX = LOWORD(lParam);
	lastY = HIWORD(lParam);
        mouseMotion = 0;
	break;

    case WM_LBUTTONUP:
        if (!cursorVisible)
        {
            ShowCursor(TRUE);
            cursorVisible = true;
        }
        if (mouseMotion < 3)
        {
            Vec3f pickRay = renderer->getPickRay(LOWORD(lParam),
                                                 HIWORD(lParam));
            Selection oldSel = sim->getSelection();
            Selection newSel = sim->pickObject(pickRay);
            sim->setSelection(newSel);
            if (!oldSel.empty() && oldSel == newSel)
                sim->centerSelection();
        }
	break;

    case WM_RBUTTONDOWN:
	lastX = LOWORD(lParam);
	lastY = HIWORD(lParam);
        mouseMotion = 0;
        break;

    case WM_RBUTTONUP:
        if (!cursorVisible)
        {
            ShowCursor(TRUE);
            cursorVisible = true;
        }
        if (mouseMotion < 3)
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            Vec3f pickRay = renderer->getPickRay(LOWORD(lParam),
                                                 HIWORD(lParam));
            Selection sel = sim->pickObject(pickRay);
            if (!sel.empty())
                handlePopupMenu(hWnd, pt, sel);
        }
        break;

    case WM_MOUSEWHEEL:
        // The mouse wheel controls the range to target
        {
            short wheelMove = (short) HIWORD(wParam);
            float factor = wheelMove > 0 ? -1.0f : 1.0f;
            mouseWheelTime = currentTime;
            mouseWheelMotion = factor * 0.25f;
        }
        break;

    case WM_MBUTTONDOWN:
        renderer->setFieldOfView(45.0f);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            if (runningScript != NULL)
            {
                delete runningScript;
                runningScript = NULL;
            }
            sim->cancelMotion();
            break;
        case VK_RETURN:
            if (textEnterMode)
            {
                if (typedText != "")
                {
                    sim->selectBody(typedText);
                    typedText = "";
                }
            }
            textEnterMode = !textEnterMode;
            break;
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_HOME:
        case VK_END:
            handleKey(wParam, true);
            break;

        case VK_F1:
            sim->setTargetSpeed(0);
            break;
        case VK_F2:
            sim->setTargetSpeed(astro::kilometersToLightYears(1.0));
            break;
        case VK_F3:
            sim->setTargetSpeed(astro::kilometersToLightYears(1000.0));
            break;
        case VK_F4:
            sim->setTargetSpeed(astro::kilometersToLightYears(1000000.0));
            break;
        case VK_F5:
            sim->setTargetSpeed(astro::AUtoLightYears(1));
            break;
        case VK_F6:
            sim->setTargetSpeed(1);
            break;
        }
	break;

    case WM_CHAR:
        handleKeyPress(wParam);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_NAVIGATION_CENTER:
            sim->centerSelection();
            break;
        case ID_NAVIGATION_GOTO:
            sim->gotoSelection();
            break;
        case ID_NAVIGATION_FOLLOW:
            sim->follow();
            break;
        case ID_NAVIGATION_HOME:
            sim->selectStar(0);
            break;
        case ID_NAVIGATION_SELECT:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_FINDOBJECT), hWnd, FindObjectProc);
            break;

        case ID_RENDER_SHOWHUDTEXT:
            {
                bool on = ToggleMenuItem(ID_RENDER_SHOWHUDTEXT);
                sim->setHUDDetail(on ? 1 : 0);
                hudDetail = on ? 1 : 0;
            }
            break;
        case ID_RENDER_SHOWPLANETLABELS:
            ToggleLabelState(ID_RENDER_SHOWPLANETLABELS, Renderer::PlanetLabels);
            break;
        case ID_RENDER_SHOWSTARLABELS:
            ToggleLabelState(ID_RENDER_SHOWSTARLABELS, Renderer::StarLabels);
            break;
        case ID_RENDER_SHOWORBITS:
            ToggleLabelState(ID_RENDER_SHOWORBITS, Renderer::PlanetOrbits);
            break;
        case ID_RENDER_SHOWCONSTELLATIONS:
            showAsterisms = !showAsterisms;
            CheckMenuItem(menuBar, ID_RENDER_SHOWCONSTELLATIONS,
                          showAsterisms ? MF_CHECKED : MF_UNCHECKED);
            renderer->showAsterisms(showAsterisms ? asterisms : NULL);
            break;
        case ID_RENDER_SHOWATMOSPHERES:
            {
                bool enabled = !renderer->getCloudMapping();
                CheckMenuItem(menuBar, ID_RENDER_SHOWATMOSPHERES,
                              enabled ? MF_CHECKED : MF_UNCHECKED);
                renderer->setCloudMapping(enabled);
            }
            break;

        case ID_RENDER_AMBIENTLIGHT_NONE:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_CHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
            renderer->setAmbientLightLevel(0.0f);
            break;
        case ID_RENDER_AMBIENTLIGHT_LOW:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_CHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
            renderer->setAmbientLightLevel(0.1f);
            break;
        case ID_RENDER_AMBIENTLIGHT_MEDIUM:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_CHECKED);
            renderer->setAmbientLightLevel(0.25f);
            break;
        case ID_RENDER_PERPIXEL_LIGHTING:
            if (renderer->perPixelLightingSupported())
            {
                bool enabled = !renderer->getPerPixelLighting();
                CheckMenuItem(menuBar, ID_RENDER_PERPIXEL_LIGHTING,
                              enabled ? MF_CHECKED : MF_UNCHECKED);
                renderer->setPerPixelLighting(enabled);
            }
            break;

        case ID_TIME_FASTER:
            sim->setTimeScale(10.0 * sim->getTimeScale());
            break;
        case ID_TIME_SLOWER:
            sim->setTimeScale(0.1 * sim->getTimeScale());
            break;
        case ID_TIME_REALTIME:
            sim->setTimeScale(1.0);
            break;
        case ID_TIME_FREEZE:
            if (paused)
            {
                sim->setTimeScale(timeScale);
                CheckMenuItem(menuBar, ID_TIME_FREEZE, MF_UNCHECKED);
            }
            else
            {
                timeScale = sim->getTimeScale();
                sim->setTimeScale(0.0);
                CheckMenuItem(menuBar, ID_TIME_FREEZE, MF_CHECKED);
            }
            paused = !paused;
            break;
        case ID_TIME_REVERSE:
            sim->setTimeScale(-sim->getTimeScale());
            break;
        case ID_TIME_SETTIME:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_SETTIME), hWnd, SetTimeProc);
            break;

        case ID_LOCATIONS_ADDLOCATION:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_ADDLOCATION), hWnd, AddLocationProc);
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

        case ID_FILE_EXIT:
	    DestroyWindow(hWnd);
            break;

        default:
            cout << LOWORD(wParam) - ID_LOCATIONS_FIRSTLOCATION << '\n';
            if (favorites != NULL &&
                LOWORD(wParam) - ID_LOCATIONS_FIRSTLOCATION < favorites->size())
            {
                int whichFavorite = LOWORD(wParam) - ID_LOCATIONS_FIRSTLOCATION;
                // if (runningScript != NULL)
                // delete runningScript;
                // runningScript = new Execution(*(*favorites)[whichFavorite]->cmdSeq, sim, renderer);
                ActivateFavorite(*(*favorites)[whichFavorite]);
            }
            else if (LOWORD(wParam) >= MENU_CHOOSE_PLANET && 
                     LOWORD(wParam) < MENU_CHOOSE_PLANET + 1000)
            {
                sim->selectPlanet(LOWORD(wParam) - MENU_CHOOSE_PLANET);
            }
        }
        break;

    case WM_KEYUP:
	handleKey(wParam, false);
	break;

    case WM_DESTROY:
	wglMakeCurrent(hDC, NULL);
	wglDeleteContext(hRC);
	if (fullscreen)
	    RestoreDisplayMode();
	PostQuitMessage( 0 );
	break;

    case WM_SIZE:
	g_w = LOWORD(lParam);
	g_h = HIWORD(lParam);
	ChangeSize(g_w, g_h);
	break;

    case WM_PAINT:
	if (bReady)
        {
	    // Get the current time, and update the time controller.
	    QueryPerformanceCounter(&TimeCur);
	    double lastTime = currentTime;
	    currentTime = (double) (TimeCur.QuadPart - TimeStart.QuadPart) / (double) TimerFreq.QuadPart;
	    double deltaTime = currentTime - lastTime;

            // Mouse wheel zoom
            if (mouseWheelMotion != 0.0f)
            {
                double mouseWheelSpan = 0.1;
                double fraction;
                
                if (currentTime - mouseWheelTime >= mouseWheelSpan)
                    fraction = (mouseWheelTime + mouseWheelSpan) - lastTime;
                else
                    fraction = deltaTime / mouseWheelSpan;

                sim->changeOrbitDistance(mouseWheelMotion * (float) fraction);
                if (currentTime - mouseWheelTime >= mouseWheelSpan)
                    mouseWheelMotion = 0.0f;
            }

            // Keyboard zoom
            if (pgupPress)
                sim->changeOrbitDistance(-deltaTime * 2);
            else if (pgdnPress)
                sim->changeOrbitDistance(deltaTime * 2);

            // Keyboard rotate
            Quatf q(1);
	    if (leftPress)
		q.zrotate((float) deltaTime * 2);
	    if (rightPress)
		q.zrotate((float) deltaTime * -2);
            if (downPress)
                q.xrotate((float) deltaTime * 2);
            if (upPress)
                q.xrotate((float) deltaTime * -2);
            sim->rotate(q);

            // cap the time step at 0.05 secs--extremely long time steps
            // may make the simulation unstable
            if (deltaTime > 0.05)
                deltaTime = 0.05;
            if (runningScript != NULL)
            {
                bool finished = runningScript->tick(deltaTime);
                if (finished)
                {
                    delete runningScript;
                    runningScript = NULL;
                }
            }
            sim->update(deltaTime);
	    sim->render(*renderer);
            RenderOverlay();
	    SwapBuffers(hDC);

	    ValidateRect(hWnd, NULL);
	}
	break;

    default:
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}
