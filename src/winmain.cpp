// main.cpp
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
#include <cstdlib>
#include <cctype>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include "gl.h"
#include "celestia.h"
#include "vecmath.h"
#include "quaternion.h"
#include "stardb.h"
#include "solarsys.h"
#include "visstars.h"
#include "mathlib.h"
#include "astro.h"
#include "config.h"
#include "simulation.h"

#include "../res/resource.h"


//----------------------------------
// Skeleton functions and variables.
//-----------------
char szAppName[] = "Celestia";
float fTime=0.f, fDeltaTime=0.f;


//----------------------------------
// Timer info.
LARGE_INTEGER TimerFreq;	// Timer Frequency.
LARGE_INTEGER TimeStart;	// Time of start.
LARGE_INTEGER TimeCur;		// Current time.


static bool fullscreen;
static int lastX = 0, lastY = 0;
static int mouseMotion = 0;
float xrot = 0, yrot = 0;
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

static Point3f initialPosition(0, 0, 0);
static Point3f position;
static Vec3f velocity;
static float distanceFromCenter = 0;

static CelestiaConfig* config = NULL;

static StarDatabase* starDB = NULL;
static StarNameDatabase* starNameDB = NULL;
static SolarSystemCatalog* solarSystemCatalog = NULL;

static Simulation* sim = NULL;
static Renderer* renderer = NULL;

HINSTANCE appInstance;

static HMENU menuBar = 0;
static HACCEL acceleratorTable = 0;

bool cursorVisible = true;

#define INFINITE_MOUSE

#define ROTATION_SPEED  6
#define ACCELERATION    20.0f


#define MENU_CHOOSE_PLANET   32000


// Good 'ol generic drawing stuff.
LRESULT CALLBACK SkeletonProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );



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


BOOL APIENTRY SetTimeProc(HWND hDlg,
                          UINT message,
                          UINT wParam,
                          LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
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



HMENU CreateMenuBar()
{
    return LoadMenu(appInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));
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

    ClientToScreen (hwnd, (LPPOINT) &point);

    TrackPopupMenu (hMenu, 0, point.x, point.y, 0, hwnd, NULL);

    DestroyMenu (hMenu);
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
    bool shift = ((GetKeyState(VK_SHIFT) & 0x8000) != 0);
    if (textEnterMode)
    {
        if (c == ' ' || isalpha(c) || isdigit(c) || ispunct(c))
        {
            if (!shift && isalpha(c))
                c = tolower(c);
            sim->typeChar(c);
        }
        return;
    }

    switch (c)
    {
    case 'A':
        if (sim->getTargetSpeed() == 0)
            sim->setTargetSpeed(0.000001f);
        else
            sim->setTargetSpeed(sim->getTargetSpeed() * 10.0f);
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

    case 'N':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::PlanetLabels);
        break;

    case 'O':
        renderer->setLabelMode(renderer->getLabelMode() ^ Renderer::PlanetOrbits);
        break;

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
}


GLsizei g_w, g_h;
int bReady;


// Good ol' creation code.
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // Say we're not ready to render yet.
    bReady = 0;

    appInstance = hInstance;

    position = Point3f(0, 0, 0);
    velocity = Vec3f(0, 0, 0);

    // Setup the window class.
    WNDCLASS wc;
    HWND hWnd;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc	= (WNDPROC)SkeletonProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CELESTIA_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szAppName;

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

    sim = new Simulation();
    sim->setStarDatabase(starDB, solarSystemCatalog);

    // Set the simulation starting time to the current system time
    sim->setTime((double) time(NULL) + (double) astro::Date(1970, 1, 1) * 86400.0);

    if (!fullscreen)
    {
        
        menuBar = CreateMenuBar();
        acceleratorTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));

	hWnd = CreateWindow(szAppName,
			    szAppName,
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
	hWnd = CreateWindow(szAppName,
			    szAppName,
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
		   "Fatal Blow",
		   MB_OK | MB_ICONERROR);
	return FALSE;
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

    // We're now ready.
    bReady = 1;

    // Usual running around in circles bit...
    int bGotMsg;
    MSG  msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
	// Use PeekMessage() if the app is active, so we can use idle time to
	// render the scene. Else, use GetMessage() to avoid eating CPU time.
	bGotMsg = PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE );

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
    bReady = 0;

    // Nuke all applicable scene stuff.
    renderer->shutdown();

    return msg.wParam;
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


LRESULT CALLBACK SkeletonProc(HWND hWnd,
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

            if ((wParam & (MK_LBUTTON | MK_RBUTTON)) == (MK_LBUTTON|MK_RBUTTON))
            {
                float amount = (float) (lastY - y) / g_h;
                sim->changeOrbitDistance(amount * 5);
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
                    sim->setOrientation(sim->getOrientation() * q);
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
            SolarSystem* solarsys = sim->getNearestSolarSystem();
            if (solarsys != NULL)
                handlePopupMenu(hWnd, pt, *solarsys);
        }
        break;

    case WM_MOUSEWHEEL:
        // The mouse wheel controls the zoom
        {
            short wheelMove = (short) HIWORD(wParam);
            float factor = 1.0f;
            if (wheelMove > 0)
            {
                if (renderer->getFieldOfView() > 0.1f)
                    renderer->setFieldOfView(renderer->getFieldOfView() / 1.1f);
            }
            else if (wheelMove < 0)
            {
                if (renderer->getFieldOfView() < 120.0f)
                    renderer->setFieldOfView(renderer->getFieldOfView() * 1.1f);
            }
        }
        break;

    case WM_MBUTTONDOWN:
        renderer->setFieldOfView(45.0f);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
	    DestroyWindow(hWnd);
            break;
        case VK_RETURN:
            if (textEnterMode)
                sim->typeChar('\n');
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
        default:
            handleKeyPress(wParam);
        }
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
            sim->setHUDDetail(ToggleMenuItem(ID_RENDER_SHOWHUDTEXT) ? 1 : 0);
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
        case ID_TIME_SETTIME:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_SETTIME), hWnd, SetTimeProc);
            break;

        case ID_HELP_ABOUT:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutProc);
            break;

        case ID_HELP_LICENSE:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_LICENSE), hWnd, LicenseProc);
            break;

        case ID_FILE_EXIT:
	    DestroyWindow(hWnd);
            break;

        default:
            printf("Unhandled command: %d\n", LOWORD(wParam) - MENU_CHOOSE_PLANET);
            if (LOWORD(wParam) >= MENU_CHOOSE_PLANET && 
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
	if (bReady) {
	    // Get the current time, and update the time controller.
	    QueryPerformanceCounter(&TimeCur);
	    float fOldTime = fTime;
	    fTime = (float)((double)(TimeCur.QuadPart-TimeStart.QuadPart)/(double)TimerFreq.QuadPart);
	    fDeltaTime = fTime - fOldTime;

            Quatf q(1);
	    if (leftPress)
		q.zrotate(fDeltaTime * 2);
	    if (rightPress)
		q.zrotate(fDeltaTime * -2);
            if (downPress)
                q.xrotate(fDeltaTime * 2);
            if (upPress)
                q.xrotate(fDeltaTime * -2);
            sim->setOrientation(sim->getOrientation() * q);
            position = Point3f(0, 0, distanceFromCenter) * conjugate(sim->getOrientation()).toMatrix4();

	    velocity *= 0.9f;

            // cap the time step at 0.05 secs--extremely long time steps
            // may make the simulation unstable
            if (fDeltaTime > 0.05f)
                fDeltaTime = 0.05f;
            sim->update((double) fDeltaTime);
	    sim->render(*renderer);
	    SwapBuffers(hDC);

	    ValidateRect(hWnd, NULL);
	}
	break;

    default:
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}
