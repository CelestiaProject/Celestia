// winmain.cpp
//
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
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
#include <celutil/winutil.h>
#include <celutil/filetype.h>
#include <celengine/celestia.h>
#include <celengine/astro.h>
#include <celengine/cmdparser.h>
#include <celengine/axisarrow.h>
#include <celengine/planetgrid.h>

#include <GL/glew.h>
#include "celestia/celestiacore.h"
#include "celestia/imagecapture.h"
#include "celestia/avicapture.h"
#include "celestia/url.h"
#include "winstarbrowser.h"
#include "winssbrowser.h"
#include "wintourguide.h"
#include "wingotodlg.h"
#include "winviewoptsdlg.h"
#include "winlocations.h"
#include "winbookmarks.h"
#include "wineclipses.h"
#include "winhyperlinks.h"
#include "wintime.h"
#include "winsplash.h"
#include "odmenu.h"
#include "celestia/scriptmenu.h"

#include "res/resource.h"
#include "wglext.h"

#include <locale.h>

using namespace std;

typedef pair<int,string> IntStrPair;
typedef vector<IntStrPair> IntStrPairVec;

char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;

// Display modes for full screen operation
static vector<DEVMODE>* displayModes = NULL;

// Display mode indices
static int currentScreenMode = 0;
static int newScreenMode = 0;

// The last fullscreen mode set; saved and restored from the registry
static int lastFullScreenMode = 0;
// A fullscreen mode guaranteed to work
static int fallbackFullScreenMode = 0;
static RECT windowRect;

static HGLRC glContext;
static HDC deviceContext;

static bool bReady = false;

static LPTSTR CelestiaRegKey = "Software\\Shatters.net\\Celestia";

HINSTANCE appInstance;
HMODULE hRes;
HWND mainWindow = 0;

static SolarSystemBrowser* solarSystemBrowser = NULL;
static StarBrowser* starBrowser = NULL;
static TourGuide* tourGuide = NULL;
static GotoObjectDialog* gotoObjectDlg = NULL;
static ViewOptionsDialog* viewOptionsDlg = NULL;
static EclipseFinderDialog* eclipseFinder = NULL;
static LocationsDialog* locationsDlg = NULL;
static SplashWindow* s_splash = NULL;

static HMENU menuBar = 0;
ODMenu odAppMenu;
static HACCEL acceleratorTable = 0;
static bool hideMenuBar = false;

// Joystick info
static bool useJoystick = false;
static bool joystickAvailable = false;
static JOYCAPS joystickCaps;

static HCURSOR hDefaultCursor = 0;
bool cursorVisible = true;
static POINT saveCursorPos;
static POINT lastMouseMove;
class WinCursorHandler;
WinCursorHandler* cursorHandler = NULL;

static int MovieSizes[8][2] = {
                                { 160, 120 },
                                { 320, 240 },
                                { 640, 480 },
                                { 720, 480 },
                                { 720, 576 },
                                { 1024, 768 },
                                { 1280, 720 },
                                { 1920, 1080 }
                              };

static float MovieFramerates[5] = { 15.0f, 24.0f, 25.0f, 29.97f, 30.0f };

static int movieSize = 1;
static int movieFramerate = 1;


astro::Date newTime(0.0);

#define REFMARKS 1

#define INFINITE_MOUSE
static int lastX = 0;
static int lastY = 0;
static bool ignoreNextMoveEvent = false;

static const WPARAM ID_GOTO_URL = 62000;

HWND hBookmarkTree;
char bookmarkName[33];

static const string ScriptsDirectory = "scripts";
static vector<ScriptMenuItem>* ScriptMenuItems = NULL;


static LRESULT CALLBACK MainWindowProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);


#define MENU_CHOOSE_PLANET   32000
#define MENU_CHOOSE_SURFACE  31000


struct AppPreferences
{
    int winWidth;
    int winHeight;
    int winX;
    int winY;
    int renderFlags;
    int labelMode;
    int locationFilter;
    int orbitMask;
    float visualMagnitude;
    float ambientLight;
    float galaxyLightGain;
    int showLocalTime;
    int dateFormat;
    int hudDetail;
    int fullScreenMode;
    uint32 lastVersion;
    string altSurfaceName;
    uint32 textureResolution;
    Renderer::StarStyle starStyle;
    GLContext::GLRenderPath renderPath;
    bool renderPathSet;
};

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

//
// A very minimal IDropTarget interface implementation
//

class CelestiaDropTarget : public IDropTarget
{
public:
    CelestiaDropTarget();
    ~CelestiaDropTarget();

    STDMETHOD  (QueryInterface)(REFIID idd, void** ppvObject);
    STDMETHOD_ (ULONG, AddRef) (void);
    STDMETHOD_ (ULONG, Release) (void);

    // IDropTarget methods
    STDMETHOD (DragEnter)(LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD (DragOver) (DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);
    STDMETHOD (DragLeave)(void);
    STDMETHOD (Drop)     (LPDATAOBJECT pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

private:
    ULONG refCount;
};

static CelestiaDropTarget* dropTarget = NULL;

CelestiaDropTarget::CelestiaDropTarget() :
    refCount(0)
{
}

CelestiaDropTarget::~CelestiaDropTarget()
{
}

HRESULT CelestiaDropTarget::QueryInterface(REFIID iid, void** ppvObject)
{
    if (iid == IID_IUnknown || iid == IID_IDropTarget)
    {
        *ppvObject = this;
        AddRef();
        return ResultFromScode(S_OK);
    }
    else
    {
        *ppvObject = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }
}

ULONG CelestiaDropTarget::AddRef(void)
{
    return ++refCount;
}

ULONG CelestiaDropTarget::Release(void)
{
    if (--refCount == 0)
    {
        delete this;
        return 0;
    }

    return refCount;
}

STDMETHODIMP
CelestiaDropTarget::DragEnter(IDataObject* pDataObject,
                              DWORD grfKeyState,
                              POINTL pt,
                              DWORD* pdwEffect)
{
    return S_OK;
}

STDMETHODIMP
CelestiaDropTarget::DragOver(DWORD grfKeyState,
                             POINTL pt,
                             DWORD* pdwEffect)
{
    return S_OK;
}

STDMETHODIMP
CelestiaDropTarget::DragLeave(void)
{
    return S_OK;
}


STDMETHODIMP
CelestiaDropTarget::Drop(IDataObject* pDataObject,
                         DWORD grfKeyState,
                         POINTL pt,
                         DWORD* pdwEffect)
{

    IEnumFORMATETC* enumFormat = NULL;
    HRESULT hr = pDataObject->EnumFormatEtc(DATADIR_GET, &enumFormat);
    if (FAILED(hr) || enumFormat == NULL)
        return E_FAIL;

    FORMATETC format;
    ULONG nFetched;
    while (enumFormat->Next(1, &format, &nFetched) == S_OK)
    {
        char buf[512];
        if (GetClipboardFormatName(format.cfFormat, buf, 511) != 0 &&
            !strcmp(buf, "UniformResourceLocator"))
        {
            STGMEDIUM medium;
            if (pDataObject->GetData(&format, &medium) == S_OK)
            {
                if (medium.tymed == TYMED_HGLOBAL && medium.hGlobal != 0)
                {
                    char* s = (char*) GlobalLock(medium.hGlobal);
					appCore->goToUrl(s);
                    GlobalUnlock(medium.hGlobal);
                    break;
                }
            }
        }
    }

    enumFormat->Release();

    return E_FAIL;
}


// Cursor handler callback class for Windows.  We pass an instance to
// the app core which the calls the setCursorShape method to change
// the cursor icon.
class WinCursorHandler : public CelestiaCore::CursorHandler
{
public:
    WinCursorHandler(HCURSOR _defaultCursor);
    virtual ~WinCursorHandler();
    virtual void setCursorShape(CelestiaCore::CursorShape);
    virtual CelestiaCore::CursorShape getCursorShape() const;

private:
    CelestiaCore::CursorShape shape;
    HCURSOR defaultCursor;
    HCURSOR sizeVertical;
    HCURSOR sizeHorizontal;
};


WinCursorHandler::WinCursorHandler(HCURSOR _defaultCursor) :
    shape(CelestiaCore::ArrowCursor),
    defaultCursor(_defaultCursor)
{
    sizeVertical   = LoadCursor(NULL, IDC_SIZENS);
    sizeHorizontal = LoadCursor(NULL, IDC_SIZEWE);
}


WinCursorHandler::~WinCursorHandler()
{
}


void WinCursorHandler::setCursorShape(CelestiaCore::CursorShape _shape)
{
    shape = _shape;
    switch (shape)
    {
    case CelestiaCore::SizeVerCursor:
        SetCursor(sizeVertical);
        break;
    case CelestiaCore::SizeHorCursor:
        SetCursor(sizeHorizontal);
        break;
    default:
        SetCursor(defaultCursor);
        break;
    }
}


CelestiaCore::CursorShape WinCursorHandler::getCursorShape() const
{
    return shape;
}

// end WinCursorHandler methods

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

        appCore->setTimeZoneName("   ");
        appCore->setTimeZoneBias((tzi.Bias + dstBias) * -60);
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


static bool CopyStateURLToClipboard()
{
    BOOL b;
    b = OpenClipboard(mainWindow);
    if (!b)
        return false;

    CelestiaState appState;
    appState.captureState(appCore);
    
    Url url(appState, Url::CurrentVersion);
    string urlString = url.getAsString();

    char* s = const_cast<char*>(urlString.c_str());
    HGLOBAL clipboardDataHandle = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
                                              strlen(s) + 1);
    char* clipboardData = (char*) GlobalLock(clipboardDataHandle);
    if (clipboardData != NULL)
    {
        strcpy(clipboardData, s);
        GlobalUnlock(clipboardDataHandle);
        EmptyClipboard();
        HANDLE h = SetClipboardData(CF_TEXT, clipboardDataHandle);
        CloseClipboard();
        return h != NULL;
    }
    else
    {
        CloseClipboard();
        return false;
    }
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

    SetDlgItemText(hWnd, item, UTF8ToCurrentCP(s).c_str());

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
        MakeHyperlinkFromStaticCtrl(hDlg, IDC_CELESTIALINK);
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_CELESTIALINK)
        {
            char urlBuf[256];
            HWND hCtrl = GetDlgItem(hDlg, IDC_CELESTIALINK);
            if (hCtrl)
            {
                if (GetWindowText(hCtrl, urlBuf, sizeof(urlBuf)) > 0)
                {
                    ShellExecute(hDlg, "open", urlBuf, NULL, NULL, SW_SHOWNORMAL);
                    return TRUE;
                }
            }
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
        LoadItemTextFromFile(hDlg, IDC_TEXT_CONTROLSHELP, const_cast<char*>(LocaleFilename("controls.txt").c_str()));
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
        LoadItemTextFromFile(hDlg, IDC_LICENSE_TEXT, const_cast<char*>(LocaleFilename("COPYING").c_str()));
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


BOOL APIENTRY GLInfoProc(HWND hDlg,
                         UINT message,
                         UINT wParam,
                         LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            const char* vendor = (char*) glGetString(GL_VENDOR);
            const char* render = (char*) glGetString(GL_RENDERER);
            const char* version = (char*) glGetString(GL_VERSION);
            const char* ext = (char*) glGetString(GL_EXTENSIONS);
            string s;
            s += UTF8ToCurrentCP(_("Vendor: "));
            if (vendor != NULL)
                s += vendor;
            s += "\r\r\n";

            s += UTF8ToCurrentCP(_("Renderer: "));
            if (render != NULL)
                s += render;
            s += "\r\r\n";

            s += UTF8ToCurrentCP(_("Version: "));
            if (version != NULL)
                s += version;
            s += "\r\r\n";

            if (GLEW_ARB_shading_language_100)
            {
                const char* versionString = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION_ARB);
                if (versionString != NULL)
                {
                    s += UTF8ToCurrentCP(_("GLSL version: "));
                    s += versionString;
                    s += "\r\r\n";
                }
            }

            char buf[1024];
            GLint simTextures = 1;
            if (GLEW_ARB_multitexture)
                glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &simTextures);
            sprintf(buf, "%s%d\r\r\n",
                    UTF8ToCurrentCP(_("Max simultaneous textures: ")).c_str(),
                    simTextures);
            s += buf;

            GLint maxTextureSize = 0;
            glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
            sprintf(buf, "%s%d\r\r\n",
                    UTF8ToCurrentCP(_("Max texture size: ")).c_str(),
                    maxTextureSize);
            s += buf;

            if (GLEW_EXT_texture_cube_map)
            {
                GLint maxCubeMapSize = 0;
                glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &maxCubeMapSize);
                sprintf(buf, "%s%d\r\r\n",
                        UTF8ToCurrentCP(_("Max cube map size: ")).c_str(),
                        maxTextureSize);
                s += buf;
            }

            GLfloat pointSizeRange[2];
            glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange);
            sprintf(buf, "%s%f - %f\r\r\n",
                    UTF8ToCurrentCP(_("Point size range: ")).c_str(),
                    pointSizeRange[0], pointSizeRange[1]);
            s += buf;

            s += "\r\r\n";
            s += UTF8ToCurrentCP(_("Supported Extensions:")).c_str();
            s += "\r\r\n";

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
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
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
            char buf[1024], out[1024];
            wchar_t wbuff[1024];
            int len = GetDlgItemText(hDlg, IDC_FINDOBJECT_EDIT, buf, sizeof(buf));
            if (len > 0)
            {
                int wlen = MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuff, sizeof(wbuff));
                WideCharToMultiByte(CP_UTF8, 0, wbuff, wlen, out, sizeof(out), NULL, NULL);
                Selection sel = appCore->getSimulation()->findObject(string(out), true);
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


BOOL APIENTRY AddBookmarkFolderProc(HWND hDlg,
                                    UINT message,
                                    UINT wParam,
                                    LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Center dialog directly over parent
            HWND hParent = GetParent(hDlg);
            CenterWindow(hParent, hDlg);

            // Limit text of folder name to 32 chars
            HWND hEdit = GetDlgItem(hDlg, IDC_BOOKMARKFOLDER);
            SendMessage(hEdit, EM_LIMITTEXT, 32, 0);

            // Set initial button states
            HWND hOK = GetDlgItem(hDlg, IDOK);
            HWND hCancel = GetDlgItem(hDlg, IDCANCEL);
            EnableWindow(hOK, FALSE);
            RemoveButtonDefaultStyle(hOK);
            AddButtonDefaultStyle(hCancel);
        }
        return TRUE;

    case WM_COMMAND:
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                HWND hOK = GetDlgItem(hDlg, IDOK);
                HWND hCancel = GetDlgItem(hDlg, IDCANCEL);

                if (hOK && hCancel)
                {
                    // If edit control contains text, enable OK button
                    char name[33];
                    GetWindowText((HWND)lParam, name, sizeof(name));
                    if (name[0])
                    {
                        // Remove Cancel button default style
                        RemoveButtonDefaultStyle(hCancel);

                        // Enable OK button
                        EnableWindow(hOK, TRUE);

                        // Make OK button default
                        AddButtonDefaultStyle(hOK);
                    }
                    else
                    {
                        // Disable OK button
                        EnableWindow(hOK, FALSE);

                        // Remove OK button default style
                        RemoveButtonDefaultStyle(hOK);

                        // Make Cancel button default
                        AddButtonDefaultStyle(hCancel);
                    }
                }
            }
        }

        if (LOWORD(wParam) == IDOK)
        {
            // Get text entered in Folder Name Edit box
            char name[33];
            HWND hEdit = GetDlgItem(hDlg, IDC_BOOKMARKFOLDER);
            if (hEdit)
            {
                if (GetWindowText(hEdit, name, sizeof(name)))
                {
                    // Create new folder in parent dialog tree control.
                    AddNewBookmarkFolderInTree(hBookmarkTree, appCore, name);
                }
            }

            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
    }

    return FALSE;
}

BOOL APIENTRY AddBookmarkProc(HWND hDlg,
                              UINT message,
                              UINT wParam,
                              LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
        RECT dlgRect, treeRect;
        HWND hCtrl;
        if (GetWindowRect(hDlg, &dlgRect))
        {
            if (hCtrl = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE))
            {
                if (GetWindowRect(hCtrl, &treeRect))
                {
                    int width = dlgRect.right - dlgRect.left;
                    int height = treeRect.top - dlgRect.top;
                    SetWindowPos(hDlg, HWND_TOP, 0, 0, width, height,
                                 SWP_NOMOVE | SWP_NOZORDER);
                }

                HTREEITEM hParent;
                if (hParent = PopulateBookmarkFolders(hCtrl, appCore, appInstance))
                {
                    //Expand bookmarks item
                    TreeView_Expand(hCtrl, hParent, TVE_EXPAND);
                }
            }
        }

        //Set initial button states
        HWND hOK = GetDlgItem(hDlg, IDOK);
        HWND hCancel = GetDlgItem(hDlg, IDCANCEL);
        EnableWindow(hOK, FALSE);
        RemoveButtonDefaultStyle(hOK);
        AddButtonDefaultStyle(hCancel);

        // Set bookmark text to selection text
        if (hCtrl = GetDlgItem(hDlg, IDC_BOOKMARK_EDIT))
        {
            //If this is a body, set the text.
            Selection sel = appCore->getSimulation()->getSelection();
            switch (sel.getType())
            {
            case Selection::Type_Body:
                {
                    string name = UTF8ToCurrentCP(sel.body()->getName(true));
                    SetWindowText(hCtrl, (char*)name.c_str());
                }
                break;

            default:
                break;
            }
        }

        return(TRUE);
        }

    case WM_COMMAND:
        {
        if (HIWORD(wParam) == EN_CHANGE)
        {
            HWND hOK = GetDlgItem(hDlg, IDOK);
            HWND hCancel = GetDlgItem(hDlg, IDCANCEL);

            if (hOK && hCancel)
            {
                //If edit control contains text, enable OK button
                char name[33];
                GetWindowText((HWND)lParam, name, sizeof(name));
                if (name[0])
                {
                    //Remove Cancel button default style
                    RemoveButtonDefaultStyle(hCancel);

                    //Enable OK button
                    EnableWindow(hOK, TRUE);

                    //Make OK button default
                    AddButtonDefaultStyle(hOK);
                }
                else
                {
                    //Disable OK button
                    EnableWindow(hOK, FALSE);

                    //Remove OK button default style
                    RemoveButtonDefaultStyle(hOK);

                    //Make Cancel button default
                    AddButtonDefaultStyle(hCancel);
                }
            }
        }
        if (LOWORD(wParam) == IDOK)
        {
            char name[33];
            int len = GetDlgItemText(hDlg, IDC_BOOKMARK_EDIT, name, sizeof(name));
            if (len > 0)
            {
                HWND hTree;
                if(hTree = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE))
                {
                    InsertBookmarkInFavorites(hTree, name, appCore);

                    appCore->writeFavoritesFile();

                    // Rebuild bookmarks menu.
                    BuildFavoritesMenu(menuBar, appCore, appInstance, &odAppMenu);
                }
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
        else if (LOWORD(wParam) == IDC_BOOKMARK_CREATEIN)
        {
            HWND button;
            RECT dlgRect, treeRect;
            HWND hTree;
            char text[16];
            if (GetWindowRect(hDlg, &dlgRect))
            {
                if (hTree = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE))
                {
                    if (GetWindowRect(hTree, &treeRect))
                    {
                        if (button = GetDlgItem(hDlg, IDC_BOOKMARK_CREATEIN))
                        {
                            if (GetWindowText(button, text, sizeof(text)))
                            {
                                int width = dlgRect.right - dlgRect.left;
                                if (strstr(text, ">>"))
                                {
                                    //Increase size of dialog
                                    int height = treeRect.bottom - dlgRect.top + 12;
                                    SetWindowPos(hDlg, HWND_TOP, 0, 0, width, height,
                                                 SWP_NOMOVE | SWP_NOZORDER);
                                    //Change text in button
                                    strcpy(text + strlen(text) - 2, "<<");
                                    SetWindowText(button, text);
                                }
                                else
                                {
                                    //Decrease size of dialog
                                    int height = treeRect.top - dlgRect.top;
                                    SetWindowPos(hDlg, HWND_TOP, 0, 0, width, height,
                                                 SWP_NOMOVE | SWP_NOZORDER);
                                    //Change text in button
                                    strcpy(text + strlen(text) - 2, ">>");
                                    SetWindowText(button, text);
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (LOWORD(wParam) == IDC_BOOKMARK_NEWFOLDER)
        {
            if(hBookmarkTree = GetDlgItem(hDlg, IDC_BOOKMARK_FOLDERTREE))
            {
                DialogBox(hRes, MAKEINTRESOURCE(IDD_ADDBOOKMARK_FOLDER),
                          hDlg, AddBookmarkFolderProc);
            }
        }
        break;
        }
    }

    return FALSE;
}

BOOL APIENTRY RenameBookmarkProc(HWND hDlg,
                                 UINT message,
                                 UINT wParam,
                                 LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
        //Center dialog directly over parent
        HWND hParent = GetParent(hDlg);
        CenterWindow(hParent, hDlg);

        //Limit text of folder name to 32 chars
        HWND hEdit = GetDlgItem(hDlg, IDC_NEWBOOKMARK);
        SendMessage(hEdit, EM_LIMITTEXT, 32, 0);

        //Set text in edit control to current bookmark name
        SetWindowText(hEdit, bookmarkName);

        return(TRUE);
        }

    case WM_COMMAND:
        {
        if (HIWORD(wParam) == EN_CHANGE)
        {
            HWND hOK = GetDlgItem(hDlg, IDOK);
            HWND hCancel = GetDlgItem(hDlg, IDCANCEL);

            if (hOK && hCancel)
            {
                //If edit control contains text, enable OK button
                char name[33];
                GetWindowText((HWND)lParam, name, sizeof(name));
                if (name[0])
                {
                    //Remove Cancel button default style
                    RemoveButtonDefaultStyle(hCancel);

                    //Enable OK button
                    EnableWindow(hOK, TRUE);

                    //Make OK button default
                    AddButtonDefaultStyle(hOK);
                }
                else
                {
                    //Disable OK button
                    EnableWindow(hOK, FALSE);

                    //Remove OK button default style
                    RemoveButtonDefaultStyle(hOK);

                    //Make Cancel button default
                    AddButtonDefaultStyle(hCancel);
                }
            }
        }
        if (LOWORD(wParam) == IDOK)
        {
            //Get text entered in Folder Name Edit box
            char name[33];
            HWND hEdit = GetDlgItem(hDlg, IDC_NEWBOOKMARK);
            if (hEdit)
            {
                if (GetWindowText(hEdit, name, sizeof(name)))
                    RenameBookmarkInFavorites(hBookmarkTree, name, appCore);
            }

            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
        }
    }

    return FALSE;
}

BOOL APIENTRY OrganizeBookmarksProc(HWND hDlg,
                                    UINT message,
                                    UINT wParam,
                                    LONG lParam)
{
    static UINT_PTR dragDropTimer;
    switch (message)
    {
    case WM_INITDIALOG:
        {
        HWND hCtrl;
        if (hCtrl = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
        {
            HTREEITEM hParent;
            if (hParent = PopulateBookmarksTree(hCtrl, appCore, hRes))
            {
                // Expand bookmarks item
                TreeView_Expand(hCtrl, hParent, TVE_EXPAND);
            }
        }
        if (hCtrl = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_DELETE))
            EnableWindow(hCtrl, FALSE);
        if (hCtrl = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_RENAME))
            EnableWindow(hCtrl, FALSE);

        return(TRUE);
        }

    case WM_COMMAND:
        {
        if (LOWORD(wParam) == IDOK)
        {
#if 0
            HWND hTree;
            if (hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
                SyncTreeFoldersWithFavoriteFolders(hTree, appCore);
#endif

            // Write any change to bookmarks
            appCore->writeFavoritesFile();

            // Rebuild bookmarks menu
            BuildFavoritesMenu(menuBar, appCore, hRes, &odAppMenu);

            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            //Refresh from file
            appCore->readFavoritesFile();

            EndDialog(hDlg, 0);
            return FALSE;
        }
        else if (LOWORD(wParam) == IDC_ORGANIZE_BOOKMARKS_NEWFOLDER)
        {
            if (hBookmarkTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
            {
                DialogBox(hRes, MAKEINTRESOURCE(IDD_ADDBOOKMARK_FOLDER), hDlg, AddBookmarkFolderProc);
            }
        }
        else if (LOWORD(wParam) == IDC_ORGANIZE_BOOKMARKS_RENAME)
        {
            if (hBookmarkTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
            {
                HTREEITEM hItem;
                TVITEM tvItem;
                if (hItem = TreeView_GetSelection(hBookmarkTree))
                {
                    tvItem.hItem = hItem;
                    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
                    tvItem.pszText = bookmarkName;
                    tvItem.cchTextMax = sizeof(bookmarkName);
                    if (TreeView_GetItem(hBookmarkTree, &tvItem))
                    {
                        DialogBox(hRes,
                                  MAKEINTRESOURCE(IDD_RENAME_BOOKMARK),
                                  hDlg, RenameBookmarkProc);
                    }
                }
            }
        }
        else if (LOWORD(wParam) == IDC_ORGANIZE_BOOKMARKS_DELETE)
        {
            HWND hTree;
            if (hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
                DeleteBookmarkFromFavorites(hTree, appCore);
        }
        break;
        }
    case WM_NOTIFY:
        {
            if (((LPNMHDR)lParam)->code == TVN_SELCHANGED)
            {
                HWND hTree;
                if (hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
                {
                    //Enable buttons as necessary
                    HTREEITEM hItem;
                    if (hItem = TreeView_GetSelection(hTree))
                    {
                        HWND hDelete, hRename;
                        hDelete = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_DELETE);
                        hRename = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARKS_RENAME);
                        if (hDelete && hRename)
                        {
                            if (TreeView_GetParent(hTree, hItem))
                            {
                                EnableWindow(hDelete, TRUE);
                                EnableWindow(hRename, TRUE);
                            }
                            else
                            {
                                EnableWindow(hDelete, FALSE);
                                EnableWindow(hRename, FALSE);
                            }
                        }
                    }
                }
            }
            else if(((LPNMHDR)lParam)->code == TVN_BEGINDRAG)
            {
                //Do not allow folders to be dragged
                HWND hTree;
                TVITEM tvItem;
                LPNMTREEVIEW nm = (LPNMTREEVIEW)lParam;
                HTREEITEM hItem = nm->itemNew.hItem;

                if (hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
                {
                    tvItem.hItem = hItem;
                    tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
                    if (TreeView_GetItem(hTree, &tvItem))
                    {
                        if(tvItem.lParam != 1)
                        {
                            //Start a timer to handle auto-scrolling
                            dragDropTimer = SetTimer(hDlg, 1, 100, NULL);
                            OrganizeBookmarksOnBeginDrag(hTree, (LPNMTREEVIEW)lParam);
                        }
                    }
                }
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if(isOrganizeBookmarksDragDropActive())
            {
                HWND hTree;
                if (hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
                {
                    OrganizeBookmarksOnMouseMove(hTree, GET_X_LPARAM(lParam),
                        GET_Y_LPARAM(lParam));
                }
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if(isOrganizeBookmarksDragDropActive())
            {
                HWND hTree;
                if (hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
                {
                    //Kill the auto-scroll timer
                    KillTimer(hDlg, dragDropTimer);

                    OrganizeBookmarksOnLButtonUp(hTree);
                    MoveBookmarkInFavorites(hTree, appCore);
                }
            }
        }
        break;
    case WM_TIMER:
        {
            if(isOrganizeBookmarksDragDropActive())
            {
                if(wParam == 1)
                {
                    //Handle
                    HWND hTree;
                    if (hTree = GetDlgItem(hDlg, IDC_ORGANIZE_BOOKMARK_TREE))
                    {
                        DragDropAutoScroll(hTree);
                    }
                }
            }
        }
        break;
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
            bind_textdomain_codeset("celestia", CurrentCP());
            SendMessage(hwnd, CB_INSERTSTRING, -1,
                        reinterpret_cast<LPARAM>(_("Windowed Mode")));
            bind_textdomain_codeset("celestia", "UTF8");

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
    return LoadMenu(hRes, MAKEINTRESOURCE(IDR_MAIN_MENU));
}

static void setMenuItemCheck(int menuItem, bool checked)
{
    CheckMenuItem(menuBar, menuItem, checked ? MF_CHECKED : MF_UNCHECKED);
}

struct IntStrPairComparePredicate
{
    IntStrPairComparePredicate() : dummy(0)
    {
    }

    bool operator()(const IntStrPair pair1, const IntStrPair pair2) const
    {
        return (pair1.second.compare(pair2.second) < 0);
    }

    int dummy;
};

static HMENU CreatePlanetarySystemMenu(string parentName, const PlanetarySystem* psys)
{
    // Use some vectors to categorize and sort the bodies within this PlanetarySystem.
    // In order to generate sorted menus, we must carry the name and menu index as a
    // single unit in the sort. One obvous way is to create a vector<pair<int,string>>
    // and then use a comparison predicate to sort.the vector based on the string in
    // each pair.

    // Declare vector<pair<int,string>> objects for each classification of body
    vector<IntStrPair> asteroids;
    vector<IntStrPair> comets;
    vector<IntStrPair> invisibles;
    vector<IntStrPair> moons;
    vector<IntStrPair> planets;
    vector<IntStrPair> spacecraft;

    // We will use these objects to iterate over all the above vectors
    vector<IntStrPairVec> objects;
    vector<string> menuNames;

    // Place each body in the correct vector based on classification
    HMENU menu = CreatePopupMenu();
    for (int i = 0; i < psys->getSystemSize(); i++)
    {
        Body* body = psys->getBody(i);
        if (!body->getName().empty())
        {
            switch(body->getClassification())
            {
            case Body::Asteroid:
                asteroids.push_back(make_pair(i, UTF8ToCurrentCP(body->getName(true))));
                break;
            case Body::Comet:
                comets.push_back(make_pair(i, UTF8ToCurrentCP(body->getName(true))));
                break;
            case Body::Invisible:
                invisibles.push_back(make_pair(i, UTF8ToCurrentCP(body->getName(true))));
                break;
            case Body::Moon:
                moons.push_back(make_pair(i, UTF8ToCurrentCP(body->getName(true))));
                break;
            case Body::Planet:
                planets.push_back(make_pair(i, UTF8ToCurrentCP(body->getName(true))));
                break;
            case Body::Spacecraft:
                spacecraft.push_back(make_pair(i, UTF8ToCurrentCP(body->getName(true))));
                break;
            }
        }
    }

    // Add each vector of PlanetarySystem bodies to a vector to iterate over
    objects.push_back(asteroids);
    menuNames.push_back(UTF8ToCurrentCP(_("Asteroids")));
    objects.push_back(comets);
    menuNames.push_back(UTF8ToCurrentCP(_("Comets")));
    objects.push_back(invisibles);
    menuNames.push_back(UTF8ToCurrentCP(_("Invisibles")));
    objects.push_back(moons);
    menuNames.push_back(UTF8ToCurrentCP(_("Moons")));
    objects.push_back(planets);
    menuNames.push_back(UTF8ToCurrentCP(_("Planets")));
    objects.push_back(spacecraft);
    menuNames.push_back(UTF8ToCurrentCP(_("Spacecraft")));

    // Now sort each vector and generate submenus
    IntStrPairComparePredicate pred;
    vector<IntStrPairVec>::iterator obj;
    vector<IntStrPair>::iterator it;
    vector<string>::iterator menuName;
    HMENU hSubMenu;
    int numSubMenus;

    // Count how many submenus we need to create
    numSubMenus = 0;
    for (obj=objects.begin(); obj != objects.end(); obj++)
    {
        if (obj->size() > 0)
            numSubMenus++;
    }

    menuName = menuNames.begin();
    for (obj=objects.begin(); obj != objects.end(); obj++)
    {
        // Only generate a submenu if the vector is not empty
        if (obj->size() > 0)
        {
            // Don't create a submenu for a single item
            if (obj->size() == 1)
            {
                it=obj->begin();
                AppendMenu(menu, MF_STRING, MENU_CHOOSE_PLANET + it->first, it->second.c_str());
            }
            else
            {
                // Skip sorting if we are dealing with the planets in our own Solar System.
                if (parentName != "Sol" || *menuName != UTF8ToCurrentCP(_("Planets")))
                    sort(obj->begin(), obj->end(), pred);

                if (numSubMenus > 1)
                {
                    // Add items to submenu
                    hSubMenu = CreatePopupMenu();
                    for(it=obj->begin(); it != obj->end(); it++)
                        AppendMenu(hSubMenu, MF_STRING, MENU_CHOOSE_PLANET + it->first, it->second.c_str());

                    AppendMenu(menu, MF_POPUP | MF_STRING, (DWORD)hSubMenu, menuName->c_str());
                }
                else
                {
                    // Just add items to the popup
                    for(it=obj->begin(); it != obj->end(); it++)
                        AppendMenu(menu, MF_STRING, MENU_CHOOSE_PLANET + it->first, it->second.c_str());
                }
            }
        }
        menuName++;
    }

    return menu;
}


static HMENU CreateAlternateSurfaceMenu(const vector<string>& surfaces)
{
    HMENU menu = CreatePopupMenu();

    AppendMenu(menu, MF_STRING, MENU_CHOOSE_SURFACE, "Normal");
    for (unsigned int i = 0; i < surfaces.size(); i++)
    {
        AppendMenu(menu, MF_STRING, MENU_CHOOSE_SURFACE + i + 1,
                   surfaces[i].c_str());
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
    switch (sel.getType())
    {
    case Selection::Type_Body:
        {
            name = sel.body()->getName(true);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, UTF8ToCurrentCP(name).c_str());
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, UTF8ToCurrentCP(_("&Goto")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, UTF8ToCurrentCP(_("&Follow")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_SYNCORBIT, UTF8ToCurrentCP(_("S&ync Orbit")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_INFO, UTF8ToCurrentCP(_("&Info")).c_str());
            HMENU refVectorMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_POPUP | MF_STRING, (DWORD) refVectorMenu, UTF8ToCurrentCP(_("&Reference Marks")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_BODY_AXES, UTF8ToCurrentCP(_("Show Body Axes")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_FRAME_AXES, UTF8ToCurrentCP(_("Show Frame Axes")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_SUN_DIRECTION, UTF8ToCurrentCP(_("Show Sun Direction")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_VELOCITY_VECTOR, UTF8ToCurrentCP(_("Show Velocity Vector")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_PLANETOGRAPHIC_GRID, UTF8ToCurrentCP(_("Show Planetographic Grid")).c_str());
            AppendMenu(refVectorMenu, MF_STRING, ID_RENDER_TERMINATOR, UTF8ToCurrentCP(_("Show Terminator")).c_str());

            CheckMenuItem(refVectorMenu, ID_RENDER_BODY_AXES,   sel.body()->findReferenceMark("body axes") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_FRAME_AXES,  sel.body()->findReferenceMark("frame axes") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_SUN_DIRECTION,  sel.body()->findReferenceMark("sun direction") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_VELOCITY_VECTOR,  sel.body()->findReferenceMark("velocity vector") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_PLANETOGRAPHIC_GRID, sel.body()->findReferenceMark("planetographic grid") ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(refVectorMenu, ID_RENDER_TERMINATOR, sel.body()->findReferenceMark("terminator") ? MF_CHECKED : MF_UNCHECKED);

            const PlanetarySystem* satellites = sel.body()->getSatellites();
            if (satellites != NULL && satellites->getSystemSize() != 0)
            {
                HMENU satMenu = CreatePlanetarySystemMenu(name, satellites);
                AppendMenu(hMenu, MF_POPUP | MF_STRING, (DWORD) satMenu,
                           UTF8ToCurrentCP(_("&Satellites")).c_str());
            }

            vector<string>* altSurfaces = sel.body()->getAlternateSurfaceNames();
            if (altSurfaces != NULL)
            {
                if (altSurfaces->size() != NULL)
                {
                    HMENU surfMenu = CreateAlternateSurfaceMenu(*altSurfaces);
                    AppendMenu(hMenu, MF_POPUP | MF_STRING, (DWORD) surfMenu,
                               UTF8ToCurrentCP(_("&Alternate Surfaces")).c_str());
                }
                delete altSurfaces;
            }
        }
        break;

    case Selection::Type_Star:
        {
            Simulation* sim = appCore->getSimulation();
            name = sim->getUniverse()->getStarCatalog()->getStarName(*(sel.star()));
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, UTF8ToCurrentCP(name).c_str());
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, UTF8ToCurrentCP(_("&Goto")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_INFO, UTF8ToCurrentCP(_("&Info")).c_str());

            SolarSystemCatalog* solarSystemCatalog = sim->getUniverse()->getSolarSystemCatalog();
            SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star()->getCatalogNumber());
            if (iter != solarSystemCatalog->end())
            {
                SolarSystem* solarSys = iter->second;
                HMENU planetsMenu = CreatePlanetarySystemMenu(name, solarSys->getPlanets());
                if (name == "Sol")
                    AppendMenu(hMenu, MF_POPUP | MF_STRING, (DWORD) planetsMenu, UTF8ToCurrentCP(_("Orbiting Bodies")).c_str());
                else
                    AppendMenu(hMenu, MF_POPUP | MF_STRING, (DWORD) planetsMenu, UTF8ToCurrentCP(_("Planets")).c_str());
            }
        }
        break;

    case Selection::Type_DeepSky:
        {
            Simulation* sim = appCore->getSimulation();
            name = sim->getUniverse()->getDSOCatalog()->getDSOName(sel.deepsky());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, UTF8ToCurrentCP(name).c_str());
            AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, UTF8ToCurrentCP(_("&Goto")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, UTF8ToCurrentCP(_("&Follow")).c_str());
            AppendMenu(hMenu, MF_STRING, ID_INFO, UTF8ToCurrentCP(_("&Info")).c_str());
        }
        break;

    case Selection::Type_Location:
        break;

    default:
        break;
    }

    if (appCore->getSimulation()->getUniverse()->isMarked(sel, 1))
        AppendMenu(hMenu, MF_STRING, ID_TOOLS_UNMARK, UTF8ToCurrentCP(_("&Unmark")).c_str());
    else
        AppendMenu(hMenu, MF_STRING, ID_TOOLS_MARK, UTF8ToCurrentCP(_("&Mark")).c_str());

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

#ifdef INFINITE_MOUSE
    ignoreNextMoveEvent = true;
#endif // INFINITE_MOUSE
}


// TODO: get rid of fixed urls
void ShowWWWInfo(const Selection& sel)
{
    string url;
    switch (sel.getType())
    {
    case Selection::Type_Body:
        {
            url = sel.body()->getInfoURL();
            if (url.empty())
            {
                string name = sel.body()->getName();
                for (unsigned int i = 0; i < name.size(); i++)
                    name[i] = tolower(name[i]);

                url = string("http://www.nineplanets.org/") + name + ".html";
            }
        }
        break;

    case Selection::Type_Star:
        {
            url = sel.star()->getInfoURL();
            if (url.empty())
            {
                char name[32];
                sprintf(name, "HIP%d", sel.star()->getCatalogNumber() & ~0xf0000000);
                url = string("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=") + name;
            }
        }
        break;

    case Selection::Type_DeepSky:
        url = sel.deepsky()->getInfoURL();
        break;

    case Selection::Type_Location:
        break;

    default:
        break;
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


unsigned int
ChooseBestMSAAPixelFormat(HDC hdc, int *formats, unsigned int numFormats,
                          int samplesRequested)
{
    int idealFormat = 0;
    int	bestFormat  = 0;
    int	bestSamples = 0;

    for (unsigned int i = 0; i < numFormats; i++)
    {
        int query = WGL_SAMPLES_ARB;
    	int result = 0;
        bool isFloatFormat = false;

        query = WGL_SAMPLES_ARB;
        wglGetPixelFormatAttribivARB(hdc, formats[i], 0, 1, &query, &result);

        if (result <= samplesRequested && result >= bestSamples)
        {
            bestSamples = result;
            bestFormat = formats[i];
        }

        if (result == samplesRequested)
            idealFormat = formats[i];
    }

    if (idealFormat != 0)
        return idealFormat;

    return bestFormat;
}


// Select the pixel format for a given device context
bool SetDCPixelFormat(HDC hDC)
{
    bool msaa = false;
    if (appCore->getConfig()->aaSamples > 1 &&
        WGLExtensionSupported("WGL_ARB_pixel_format") &&
        WGLExtensionSupported("WGL_ARB_multisample"))
    {
        msaa = true;
    }
        
    if (!msaa)
    {
        static PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),	// Size of this structure
            1,				// Version of this structure
            PFD_DRAW_TO_WINDOW |	// Draw to Window (not to bitmap)
            PFD_SUPPORT_OPENGL |	// Support OpenGL calls in window
            PFD_DOUBLEBUFFER,		// Double buffered mode
            PFD_TYPE_RGBA,		// RGBA Color mode
            GetDeviceCaps(hDC, BITSPIXEL),// Want the display bit depth
            0,0,0,0,0,0,		  // Not used to select mode
            0,0,			// Not used to select mode
            0,0,0,0,0,			// Not used to select mode
            24,				// Size of depth buffer
            0,				// Not used to select mode
            0,				// Not used to select mode
            PFD_MAIN_PLANE,             // Draw in main plane
            0,                          // Not used to select mode
            0,0,0                       // Not used to select mode
        };

        // Choose a pixel format that best matches that described in pfd
        int nPixelFormat = ChoosePixelFormat(hDC, &pfd);
        if (nPixelFormat == 0)
        {
            // Uh oh . . . looks like we can't handle OpenGL on this device.
            return false;
        }
        else
        {
            // Set the pixel format for the device context
            SetPixelFormat(hDC, nPixelFormat, &pfd);
            return true;
        }
    }
    else
    {
        PIXELFORMATDESCRIPTOR pfd;

        int ifmtList[] = {
            WGL_DRAW_TO_WINDOW_ARB,        TRUE,
            WGL_SUPPORT_OPENGL_ARB,        TRUE,
            WGL_DOUBLE_BUFFER_ARB,         TRUE,
            WGL_PIXEL_TYPE_ARB,            WGL_TYPE_RGBA_ARB,
            WGL_DEPTH_BITS_ARB,            24,
            WGL_COLOR_BITS_ARB,            24,
            WGL_RED_BITS_ARB,               8,
            WGL_GREEN_BITS_ARB,             8,
            WGL_BLUE_BITS_ARB,              8,
            WGL_ALPHA_BITS_ARB,             0,
            WGL_ACCUM_BITS_ARB,             0,
            WGL_STENCIL_BITS_ARB,           0,
            WGL_SAMPLE_BUFFERS_ARB,         appCore->getConfig()->aaSamples > 1,
            0
        };

        int             pixelFormatIndex;
        int             pixFormats[256];
        unsigned int    numFormats;

        wglChoosePixelFormatARB(hDC, ifmtList, NULL, 256, pixFormats, &numFormats);

        pixelFormatIndex = ChooseBestMSAAPixelFormat(hDC, pixFormats,
                                                     numFormats,
                                                     appCore->getConfig()->aaSamples);

        DescribePixelFormat(hDC, pixelFormatIndex,
                            sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        if (!SetPixelFormat(hDC, pixelFormatIndex, &pfd))
            return false;

        return true;
    }
}


HWND CreateOpenGLWindow(int x, int y, int width, int height,
                        int mode, int& newMode)
{
    assert(mode >= 0 && mode <= displayModes->size());
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
    wc.hIcon = LoadIcon(hRes, MAKEINTRESOURCE(IDI_CELESTIA_ICON));
    wc.hCursor = hDefaultCursor;
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
        dwStyle = WS_POPUPWINDOW | WS_MAXIMIZE;
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
    if (!SetDCPixelFormat(deviceContext))
    {
        MessageBox(NULL,
                   "Could not get appropriate pixel format for OpenGL rendering.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
		return NULL;
    }

    if (newMode == 0)
        SetMenu(hwnd, menuBar);
    else
        hideMenuBar = true;

	bool firstContext = false;
    if (glContext == NULL)
	{
        glContext = wglCreateContext(deviceContext);
		firstContext = true;
	}
    wglMakeCurrent(deviceContext, glContext);

	if (firstContext)
	{
		GLenum glewErr = glewInit();
		if (glewErr != GLEW_OK)
		{
			MessageBox(NULL, "Could not set up OpenGL extensions.", "Fatal Error",
					   MB_OK | MB_ICONERROR);
			return NULL;
		}
	}

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
    int modifiers = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000)
        modifiers |= CelestiaCore::ShiftKey;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        modifiers |= CelestiaCore::ControlKey;

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
    case VK_PRIOR:
        k = CelestiaCore::Key_PageUp;
        break;
    case VK_NEXT:
        k = CelestiaCore::Key_PageDown;
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
    case VK_DELETE:
        if (!down)
            appCore->charEntered('\177');
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        // Special handling required to send Ctrl+number keys to
        // Celestia keyboard handler.
        if (!down && (modifiers & CelestiaCore::ControlKey))
            appCore->charEntered((char) key, modifiers);
        break;

    case 'A':
    case 'Z':
        if ((GetKeyState(VK_CONTROL) & 0x8000) == 0)
            k = key;
        break;
    }

    if (k >= 0)
    {
        if (down)
            appCore->keyDown(k, modifiers);
        else
            appCore->keyUp(k, modifiers);
    }
}


static void BuildScriptsMenu(HMENU menuBar, const string& scriptsDir)
{
    HMENU fileMenu = GetSubMenu(menuBar, 0);

    if (ScriptMenuItems != NULL)
        delete ScriptMenuItems;

    ScriptMenuItems = ScanScriptsDirectory(scriptsDir, false);
    if (ScriptMenuItems == NULL || ScriptMenuItems->size() == 0)
    {
        EnableMenuItem(fileMenu, ID_FILE_SCRIPTS, MF_GRAYED);
        return;
    }
    
    MENUITEMINFO info;
    memset(&info, sizeof(info), 0);
    info.cbSize = sizeof(info);
    info.fMask = MIIM_SUBMENU;

    BOOL result = GetMenuItemInfo(fileMenu, 1, TRUE, &info);

    if (result)
    {
        HMENU scriptMenu = info.hSubMenu;

        // Remove the old menu items
        int count = GetMenuItemCount(scriptMenu);
        while (count-- > 0)
            DeleteMenu(scriptMenu, 0, MF_BYPOSITION);

        for (unsigned int i = 0; i < ScriptMenuItems->size(); i++)
        {
            AppendMenu(scriptMenu, MF_STRING, ID_FIRST_SCRIPT + i, (*ScriptMenuItems)[i].title.c_str());
        }
    }
}


static void syncMenusWithRendererState()
{
    int renderFlags = appCore->getRenderer()->getRenderFlags();
    int labelMode = appCore->getRenderer()->getLabelMode();
    float ambientLight = appCore->getRenderer()->getAmbientLightLevel();
    unsigned int textureRes = appCore->getRenderer()->getResolution();

    setMenuItemCheck(ID_VIEW_SHOW_FRAMES,
                     appCore->getFramesVisible());
    setMenuItemCheck(ID_VIEW_SYNC_TIME,
                     appCore->getSimulation()->getSyncTime());

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

    Renderer::StarStyle style = appCore->getRenderer()->getStarStyle();
    CheckMenuItem(menuBar, ID_RENDER_STARSTYLE_FUZZY,
                  style == Renderer::FuzzyPointStars ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_RENDER_STARSTYLE_POINTS,
                  style == Renderer::PointStars ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_RENDER_STARSTYLE_DISCS,
                  style == Renderer::ScaledDiscStars ? MF_CHECKED : MF_UNCHECKED);

    CheckMenuItem(menuBar, ID_RENDER_TEXTURERES_LOW,
                  textureRes == 0 ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_RENDER_TEXTURERES_MEDIUM,
                  textureRes == 1 ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_RENDER_TEXTURERES_HIGH,
                  textureRes == 2 ? MF_CHECKED : MF_UNCHECKED);

    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_STATE;
    if (GetMenuItemInfo(menuBar, ID_TIME_SHOWLOCAL, FALSE, &menuInfo))
    {
        if (appCore->getTimeZoneBias() == 0)
            CheckMenuItem(menuBar, ID_TIME_SHOWLOCAL, MF_UNCHECKED);
        else
            CheckMenuItem(menuBar, ID_TIME_SHOWLOCAL, MF_CHECKED);
    }

    CheckMenuItem(menuBar, ID_RENDER_ANTIALIASING,
        (renderFlags & Renderer::ShowSmoothLines)?MF_CHECKED:MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_RENDER_AUTOMAG,
        (renderFlags & Renderer::ShowAutoMag)?MF_CHECKED:MF_UNCHECKED);
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
		if (s_splash != NULL)
			s_splash->close();
			
        MessageBox(NULL,
                   msg.c_str(),
                   "Fatal Error",
                   MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
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
    cpValueName    - Name of Key Value to obtain value for.
    lpBuf      - Buffer to receive value of Key Value.
    iBufSize   - Size of buffer pointed to by lpBuf.

    RETURN     - true if value was successfully retrieved, false otherwise.

    Remarks: If requesting a string value, pass the character buffer to lpBuf.
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

static bool SetRegistry(HKEY key, LPCTSTR value, const string& strVal)
{
    LONG err = RegSetValueEx(key,
                             value,
                             0,
                             REG_SZ,
                             reinterpret_cast<CONST BYTE*> (strVal.c_str()),
                             strVal.length() + 1);

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

    bool bRC = false;

    if (RegSetValueEx(hKey, cpValueName, 0, REG_BINARY,
                      (LPBYTE) lpData, (DWORD) iDataSize) == ERROR_SUCCESS)
    {
        bRC = true;
    }

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

    GetRegistryValue(key, "Width", &prefs.winWidth, sizeof(prefs.winWidth));
    GetRegistryValue(key, "Height", &prefs.winHeight, sizeof(prefs.winHeight));
    GetRegistryValue(key, "XPos", &prefs.winX, sizeof(prefs.winX));
    GetRegistryValue(key, "YPos", &prefs.winY, sizeof(prefs.winY));
    GetRegistryValue(key, "RenderFlags", &prefs.renderFlags, sizeof(prefs.renderFlags));
    GetRegistryValue(key, "LabelMode", &prefs.labelMode, sizeof(prefs.labelMode));
    GetRegistryValue(key, "LocationFilter", &prefs.locationFilter, sizeof(prefs.locationFilter));
    GetRegistryValue(key, "OrbitMask", &prefs.orbitMask, sizeof(prefs.orbitMask));
    GetRegistryValue(key, "VisualMagnitude", &prefs.visualMagnitude, sizeof(prefs.visualMagnitude));
    GetRegistryValue(key, "AmbientLight", &prefs.ambientLight, sizeof(prefs.ambientLight));
    GetRegistryValue(key, "GalaxyLightGain", &prefs.galaxyLightGain, sizeof(prefs.galaxyLightGain));
    GetRegistryValue(key, "ShowLocalTime", &prefs.showLocalTime, sizeof(prefs.showLocalTime));
    GetRegistryValue(key, "DateFormat", &prefs.dateFormat, sizeof(prefs.dateFormat));
    GetRegistryValue(key, "HudDetail", &prefs.hudDetail, sizeof(prefs.hudDetail));
    GetRegistryValue(key, "FullScreenMode", &prefs.fullScreenMode, sizeof(prefs.fullScreenMode));

    prefs.starStyle = Renderer::FuzzyPointStars;
    GetRegistryValue(key, "StarStyle", &prefs.starStyle, sizeof(prefs.starStyle));
	prefs.renderPath = GLContext::GLPath_Basic;
	prefs.renderPathSet = GetRegistryValue(key, "RenderPath", &prefs.renderPath, sizeof(prefs.renderPath));

    GetRegistryValue(key, "LastVersion", &prefs.lastVersion, sizeof(prefs.lastVersion));
    GetRegistryValue(key, "TextureResolution", &prefs.textureResolution, sizeof(prefs.textureResolution));

    char surfaceName[512];
    surfaceName[0] = '\0';
    if (GetRegistryValue(key, "AltSurface", surfaceName, sizeof(surfaceName)))
        prefs.altSurfaceName = string(surfaceName);

    if (prefs.lastVersion < 0x01020500)
    {
        prefs.renderFlags |= Renderer::ShowCometTails;
        prefs.renderFlags |= Renderer::ShowRingShadows;
    }

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
    SetRegistryInt(key, "LocationFilter", prefs.locationFilter);
    SetRegistryInt(key, "OrbitMask", prefs.orbitMask);
    SetRegistryBin(key, "VisualMagnitude", &prefs.visualMagnitude, sizeof(prefs.visualMagnitude));
    SetRegistryBin(key, "AmbientLight", &prefs.ambientLight, sizeof(prefs.ambientLight));
    SetRegistryBin(key, "GalaxyLightGain", &prefs.galaxyLightGain, sizeof(prefs.galaxyLightGain));
    SetRegistryInt(key, "ShowLocalTime", prefs.showLocalTime);
    SetRegistryInt(key, "DateFormat", prefs.dateFormat);
    SetRegistryInt(key, "HudDetail", prefs.hudDetail);
    SetRegistryInt(key, "FullScreenMode", prefs.fullScreenMode);
    SetRegistryInt(key, "LastVersion", prefs.lastVersion);
    SetRegistryInt(key, "StarStyle", prefs.starStyle);
	  SetRegistryInt(key, "RenderPath", prefs.renderPath);
    SetRegistry(key, "AltSurface", prefs.altSurfaceName);
    SetRegistryInt(key, "TextureResolution", prefs.textureResolution);

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
    prefs.locationFilter = appCore->getSimulation()->getActiveObserver()->getLocationFilter();
    prefs.orbitMask = appCore->getRenderer()->getOrbitMask();
    prefs.visualMagnitude = appCore->getSimulation()->getFaintestVisible();
    prefs.ambientLight = appCore->getRenderer()->getAmbientLightLevel();
    prefs.galaxyLightGain = Galaxy::getLightGain();
    prefs.showLocalTime = (appCore->getTimeZoneBias() != 0);
    prefs.dateFormat = appCore->getDateFormat();
    prefs.hudDetail = appCore->getHudDetail();
    prefs.fullScreenMode = lastFullScreenMode;
    prefs.lastVersion = 0x01040100;
    prefs.altSurfaceName = appCore->getSimulation()->getActiveObserver()->getDisplayedSurface();
    prefs.starStyle = appCore->getRenderer()->getStarStyle();
	prefs.renderPath = appCore->getRenderer()->getGLContext()->getRenderPath();
    prefs.textureResolution = appCore->getRenderer()->getResolution();

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
    Ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    // Display the Save dialog box.
    if (GetSaveFileName(&Ofn))
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
        if (Ofn.nFileExtension == 0)
        {
            // If no extension was specified, use the selection of filter to
            // determine which type of file should be created, instead of
            // just defaulting to JPEG.
            nFileType = Ofn.nFilterIndex;
            strcat(Ofn.lpstrFile, ".");
            strcat(Ofn.lpstrFile, defaultExtensions[nFileType-1]);
        }
        else if (*(Ofn.lpstrFile + Ofn.nFileExtension) == '\0')
        {
            // If just a period was specified for the extension, use the
            // selection of filter to determine which type of file should be
            // created instead of just defaulting to JPEG.
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

        // Redraw to make sure that the back buffer is up to date
        appCore->draw();

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
            DPRINTF(0, "WTF? Unknown file extension specified for screen capture.\n");
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
    Ofn.lpstrTitle = "Save As - Specify Output File for Capture Movie";

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
            DPRINTF(0, "Unknown file extension specified for movie capture.\n");
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

            if (nFileType == 0)
                sprintf(errorMsg, "Specified file extension is not recognized.");
            else
                sprintf(errorMsg, "Could not capture movie.");

            MessageBox(hWnd, errorMsg, "Error", MB_OK | MB_ICONERROR);
        }
    }
}


static void HandleOpenScript(HWND hWnd, CelestiaCore* appCore)
{
    // Display File Open dialog to allow user to specify name and location of
    // of captured screen image.
    OPENFILENAME Ofn;
    char szFile[_MAX_PATH + 1];
    char szFileTitle[_MAX_PATH + 1];

    // Save the current directory
    char currentDir[_MAX_PATH + 1];
    currentDir[0] = '\0';
    GetCurrentDirectory(sizeof(currentDir), currentDir);

    szFile[0] = '\0';
    szFileTitle[0] = '\0';

    // Initialize OPENFILENAME
    ZeroMemory(&Ofn, sizeof(OPENFILENAME));
    Ofn.lStructSize = sizeof(OPENFILENAME);
    Ofn.hwndOwner = hWnd;
    Ofn.lpstrFilter = "Celestia Script\0*.celx;*.clx;*.cel\0";
    Ofn.lpstrFile= szFile;
    Ofn.nMaxFile = sizeof(szFile);
    Ofn.lpstrFileTitle = szFileTitle;
    Ofn.nMaxFileTitle = sizeof(szFileTitle);
    Ofn.lpstrInitialDir = (LPSTR)NULL;

    // Comment this out if you just want the standard "Save As" caption.
    // Ofn.lpstrTitle = "Save As - Specify File to Capture Image";

    // Display the Open dialog box.
    if (GetOpenFileName(&Ofn))
    {
        // If you got here, a path and file has been specified.
        // Ofn.lpstrFile contains full path to specified file
        // Ofn.lpstrFileTitle contains just the filename with extension
        ContentType type = DetermineFileType(Ofn.lpstrFile);

        if (type == Content_CelestiaScript)
        {
            appCore->runScript(Ofn.lpstrFile);
        }
        else if (type == Content_CelestiaLegacyScript)
        {
            ifstream scriptfile(Ofn.lpstrFile);
            if (!scriptfile.good())
            {
                MessageBox(hWnd, "Error opening script file.", "Error",
                           MB_OK | MB_ICONERROR);
            }
            else
            {
                CommandParser parser(scriptfile);
                CommandSequence* script = parser.parse();
                if (script == NULL)
                {
                    const vector<string>* errors = parser.getErrors();
                    const char* errorMsg = "";
                    if (errors->size() > 0)
                        errorMsg = (*errors)[0].c_str();
                    MessageBox(hWnd, errorMsg, "Error in script file.",
                               MB_OK | MB_ICONERROR);
                }
                else
                {
                    appCore->cancelScript(); // cancel any running script
                    appCore->runScript(script);
                }
            }
        }
    }

    if (strlen(currentDir) != 0)
        SetCurrentDirectory(currentDir);
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

vector<DEVMODE>* EnumerateDisplayModes(unsigned int minBPP)
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

    // Bail out early if EnumDisplaySettings fails for some messed up reason
    if (modes->empty())
        return modes;

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

    modes->resize(keepIter - modes->begin() + 1);

    // Select the fallback display mode--choose 640x480 at the current
    // pixel depth.  If for some bizarre reason that's not available,
    // fall back to the first mode in the list.
    fallbackFullScreenMode = 0;
    for (iter = modes->begin(); iter != modes->end(); iter++)
    {
        if (iter->dmPelsWidth == 640 && iter->dmPelsHeight == 480)
        {
            // Add one to the mode index, since mode 0 means windowed mode
            fallbackFullScreenMode = (iter - modes->begin()) + 1;
            break;
        }
    }
    if (fallbackFullScreenMode == 0 && modes->size() > 0)
        fallbackFullScreenMode = 1;
    lastFullScreenMode = fallbackFullScreenMode;

    return modes;
}


static char* skipSpace(char* s)
{
    while (*s == ' ')
        s++;
    return s;
}

static char* skipUntilQuote(char* s)
{
    while (*s != '"' && s != '\0')
        s++;
    return s;
}

static char* nextWord(char* s)
{
    while (*s != ' ' && *s != '\0')
        s++;
    return s;
}

static char** splitCommandLine(LPSTR cmdLine,
                               int& argc)
{
    int nArgs = 0;
    int maxArgs = 50;
    char** argv = new char*[maxArgs];

    cmdLine = skipSpace(cmdLine);
    while (*cmdLine != '\0')
    {
        char* startOfWord = cmdLine;
        char* endOfWord = cmdLine;
        int wordLength = 0;

        if (*cmdLine == '"')
        {
            // Handle quoted strings
            startOfWord = cmdLine + 1;
            endOfWord = skipUntilQuote(startOfWord);
            wordLength = endOfWord - startOfWord;
            if (*endOfWord != '\0')
                endOfWord++;
        }
        else
        {
            endOfWord = nextWord(cmdLine);
            wordLength = endOfWord - startOfWord;
            assert(wordLength != 0);
        }

        char* newWord = new char[wordLength + 1];
        strncpy(newWord, startOfWord, wordLength);
        newWord[wordLength] = '\0';

        if (nArgs == maxArgs)
        {
            char** newArgv = new char*[maxArgs * 2];
            copy(argv, argv + nArgs, newArgv);
            delete argv;
            argv = newArgv;
            maxArgs *= 2;
        }

        argv[nArgs] = newWord;
        nArgs++;

        cmdLine = endOfWord;
        cmdLine = skipSpace(cmdLine);
    }

    argc = nArgs;

    return argv;
}


static bool startFullscreen = false;
static bool runOnce = false;
static string startURL;
static string startDirectory;
static string startScript;
static vector<string> extrasDirectories;
static string configFileName;
static bool useAlternateConfigFile = false;
static bool skipSplashScreen = false;

static bool parseCommandLine(int argc, char* argv[])
{
    int i = 0;

    while (i < argc)
    {
        bool isLastArg = (i == argc - 1);
        if (strcmp(argv[i], "--verbose") == 0)
        {
            SetDebugVerbosity(1);
        }
        else if (strcmp(argv[i], "--fullscreen") == 0)
        {
            startFullscreen = true;
        }
        else if (strcmp(argv[i], "--once") == 0)
        {
            runOnce = true;
        }
        else if (strcmp(argv[i], "--dir") == 0)
        {
            if (isLastArg)
            {
                MessageBox(NULL,
                           "Directory expected after --dir", "Celestia Command Line Error",
                           MB_OK | MB_ICONERROR);
                return false;
            }
            i++;
            startDirectory = string(argv[i]);
        }
        else if (strcmp(argv[i], "--conf") == 0)
        {
            if (isLastArg)
            {
                MessageBox(NULL,
                           "Configuration file name expected after --conf",
                           "Celestia Command Line Error",
                           MB_OK | MB_ICONERROR);
                return false;
            }
            i++;
            configFileName = string(argv[i]);
            useAlternateConfigFile = true;
        }
        else if (strcmp(argv[i], "--extrasdir") == 0)
        {
            if (isLastArg)
            {
                MessageBox(NULL,
                           "Directory expected after --extrasdir", "Celestia Command Line Error",
                           MB_OK | MB_ICONERROR);
                return false;
            }
            i++;
            extrasDirectories.push_back(string(argv[i]));
        }
        else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--url") == 0)
        {
            if (isLastArg)
            {
                MessageBox(NULL,
                           "URL expected after --url", "Celestia Command Line Error",
                           MB_OK | MB_ICONERROR);
                return false;
            }
            i++;
            startURL = string(argv[i]);
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--nosplash") == 0)
        {
            skipSplashScreen = true;
        }
        else
        {
            char* buf = new char[strlen(argv[i]) + 256];
            sprintf(buf, "Invalid command line option '%s'", argv[i]);
            MessageBox(NULL,
                       buf, "Celestia Command Line Error",
                       MB_OK | MB_ICONERROR);
            delete[] buf;
            return false;
        }

        i++;
    }

    return true;
}


class WinSplashProgressNotifier : public ProgressNotifier
{
public:
    WinSplashProgressNotifier(SplashWindow* _splash) : splash(_splash) {};
    virtual ~WinSplashProgressNotifier() {};
    
    virtual void update(const string& filename)
    {
        splash->setMessage(UTF8ToCurrentCP(_("Loading: ")) + filename);
    }
    
private:
    SplashWindow* splash;
};

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
        
    // Say we're not ready to render yet.
    bReady = false;

    appInstance = hInstance;

    int argc;
    char** argv;
    argv = splitCommandLine(lpCmdLine, argc);
    bool cmdLineOK = parseCommandLine(argc, argv);
    if (!cmdLineOK)
        return 1;

    // If Celestia was invoked with the --once command line parameter,
    // check to see if there's already an instance of Celestia running.
    if (runOnce)
    {
        // The FindWindow method of checking for another running instance
        // is broken, and we should be using CreateMutex instead.  But we'll
        // sort that out later . . .
        HWND existingWnd = FindWindow(AppName, AppName);
        if (existingWnd)
        {
            // If there's an existing instance and we've been given a
            // URL on the command line, send the URL to the running instance
            // of Celestia before terminating.
            if (startURL != "")
            {
                COPYDATASTRUCT cd;
                cd.dwData = 0;
                cd.cbData = startURL.length();
                cd.lpData = reinterpret_cast<void*>(const_cast<char*>(startURL.c_str()));
                SendMessage(existingWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cd));
            }
            SetForegroundWindow(existingWnd);
            Sleep(1000);
            exit(0);
        }
    }

    // If a start directory was given on the command line, switch to it
    // now.
    if (startDirectory != "")
        SetCurrentDirectory(startDirectory.c_str());

	s_splash = new SplashWindow("splash.png");
    s_splash->setMessage("Loading data files...");
    if (!skipSplashScreen)
        s_splash->showSplash();
    
    OleInitialize(NULL);
    dropTarget = new CelestiaDropTarget();
    if (dropTarget)
    {
        if (CoLockObjectExternal(dropTarget, TRUE, TRUE) != S_OK)
        {
            cout << "Error locking drop target\n";
            delete dropTarget;
            dropTarget = NULL;
        }
    }

    // Specify some default values in case registry keys are not found. Ideally, these
    // defaults should never be used--they should be overridden by settings in
    // celestia.cfg.
    AppPreferences prefs;
    prefs.winWidth = 800;
    prefs.winHeight = 600;
    prefs.winX = CW_USEDEFAULT;
    prefs.winY = CW_USEDEFAULT;
    prefs.ambientLight = 0.1f;  // Low
    prefs.galaxyLightGain = 0.0f;
    prefs.labelMode = 0;
    prefs.locationFilter = 0;
    prefs.orbitMask = Body::Planet | Body::Moon;
    prefs.renderFlags = Renderer::DefaultRenderFlags;

    prefs.visualMagnitude = 6.0f;   //Default specified in Simulation::Simulation()
    prefs.showLocalTime = 0;
    prefs.dateFormat = 0;
    prefs.hudDetail = 1;
    prefs.fullScreenMode = -1;
    prefs.lastVersion = 0x00000000;
    prefs.textureResolution = 1;
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

    // Make sure windowRect contains default window size in case Celestia is
    // launched in fullscreen mode. Without this code, CreateOpenGLWindow()
    // will be called with windowRect = {0, 0, 0, 0} when the user switches to
    // windowed mode.
    windowRect.left = prefs.winX;
    windowRect.top = prefs.winY;
    windowRect.right = windowRect.left + prefs.winWidth;
    windowRect.bottom = windowRect.top + prefs.winHeight;

    joystickAvailable = InitJoystick(joystickCaps);

    displayModes = EnumerateDisplayModes(16);

    // If full screen mode was found in registry, override default with it.
    if (prefs.fullScreenMode != -1)
        lastFullScreenMode = prefs.fullScreenMode;

    // If the user changes the computer's graphics card or driver, the
    // number of display modes may change. Since we're storing a display
    // index this can cause troubles. Avoid out of range errors by
    // checking against the size of the mode list, falling back to a
    // safe mode if the last used mode index is now out of range.
    // TODO: A MUCH better fix for the problem of changing GPU/driver
    // is to store the mode parameters instead of just the mode index.
    if (lastFullScreenMode > displayModes->size())
    {
        lastFullScreenMode = fallbackFullScreenMode;
    }

    appCore = new CelestiaCore();
    if (appCore == NULL)
    {
        if (s_splash != NULL)
        {
            s_splash->close();
            delete s_splash;
            s_splash = NULL;
        }
		
        MessageBox(NULL,
                   "Out of memory.", "Fatal Error",
                   MB_OK | MB_ICONERROR | MB_TOPMOST);		   
        return false;
    }

    // Gettext integration
    setlocale(LC_ALL, ""); 
    setlocale(LC_NUMERIC, "C"); 
    bindtextdomain("celestia","locale");
    bind_textdomain_codeset("celestia", "UTF-8"); 
    bindtextdomain("celestia_constellations","locale");
    bind_textdomain_codeset("celestia_constellations", "UTF-8"); 
    textdomain("celestia");

    // Loading localized resources
    char res[255];
    sprintf(res, "locale\\res_%s.dll", _("LANGUAGE"));
    int langID = 0;
    if (sscanf(_("WinLangID"), "%x", &langID) == 1)
        SetThreadLocale(langID);
    if ((hRes = LoadLibrary(res)) == NULL) {
        cout << "Couldn't load localized resources: "<< res<< "\n";
        hRes = hInstance;
    }

    appCore->setAlerter(new WinAlerter());

    WinSplashProgressNotifier* progressNotifier = NULL;
    if (!skipSplashScreen)
        progressNotifier = new WinSplashProgressNotifier(s_splash);
        
    string* altConfig = useAlternateConfigFile ? &configFileName : NULL;
    bool initSucceeded = appCore->initSimulation(altConfig, &extrasDirectories, progressNotifier);
    
    delete progressNotifier;

    // Close the splash screen after all data has been loaded
	if (s_splash != NULL)
	{
		s_splash->close();
		delete s_splash;
        s_splash = NULL;
	}
	
	// Give up now if we failed initialization
	if (!initSucceeded)
		return 1;

    if (startURL != "")
        appCore->setStartURL(startURL);

    menuBar = CreateMenuBar();
    acceleratorTable = LoadAccelerators(hRes,
                                        MAKEINTRESOURCE(IDR_ACCELERATORS));

    if (appCore->getConfig() != NULL)
    {
        if (!compareIgnoringCase(appCore->getConfig()->cursor, "arrow"))
            hDefaultCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
        else if (!compareIgnoringCase(appCore->getConfig()->cursor, "inverting crosshair"))
            hDefaultCursor = LoadCursor(hRes, MAKEINTRESOURCE(IDC_CROSSHAIR));
        else
            hDefaultCursor = LoadCursor(hRes, MAKEINTRESOURCE(IDC_CROSSHAIR_OPAQUE));
    }

    cursorHandler = new WinCursorHandler(hDefaultCursor);
    appCore->setCursorHandler(cursorHandler);

    InitWGLExtensions(appInstance);

    HWND hWnd;
    if (startFullscreen)
    {
        hWnd = CreateOpenGLWindow(0, 0, 800, 600,
                                  lastFullScreenMode, currentScreenMode);

        // Prevent unnecessary destruction and recreation of OpenGLWindow in
        // while() loop below.
        newScreenMode = currentScreenMode;
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

    if (dropTarget != NULL)
    {
        HRESULT hr = RegisterDragDrop(hWnd, dropTarget);
        if (hr != S_OK)
        {
            cout << "Failed to register drop target as OLE object.\n";
        }
    }

    mainWindow = hWnd;

    UpdateWindow(mainWindow);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    extern void RegisterDatePicker();
    RegisterDatePicker();

    if (!appCore->initRenderer())
    {
        return 1;
    }

    // Set values saved in registry: renderFlags, visualMagnitude, labelMode and timezone bias.
    if (prefs.lastVersion != 0)
    {
        appCore->getSimulation()->setFaintestVisible(prefs.visualMagnitude);
        appCore->getRenderer()->setRenderFlags(prefs.renderFlags);
        appCore->getRenderer()->setLabelMode(prefs.labelMode);
        appCore->getSimulation()->getActiveObserver()->setLocationFilter(prefs.locationFilter);
        appCore->getRenderer()->setOrbitMask(prefs.orbitMask);
        appCore->getRenderer()->setAmbientLightLevel(prefs.ambientLight);
        Galaxy::setLightGain(prefs.galaxyLightGain);
        appCore->getRenderer()->setStarStyle(prefs.starStyle);
        appCore->setHudDetail(prefs.hudDetail);
        if (prefs.showLocalTime == 1)
            ShowLocalTime(appCore);
        else
            ShowUniversalTime(appCore);
        appCore->setDateFormat((astro::Date::Format) prefs.dateFormat);
        appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(prefs.altSurfaceName);
        appCore->getRenderer()->setResolution(prefs.textureResolution);
        if (prefs.renderPathSet)
        {
            GLContext* glContext = appCore->getRenderer()->getGLContext();
            if (glContext->renderPathSupported(prefs.renderPath))
                glContext->setRenderPath(prefs.renderPath);
        }
    }
    else
    {
        // Set default render flags for a new installation
        appCore->getRenderer()->setRenderFlags(Renderer::DefaultRenderFlags);
    }
    
    BuildFavoritesMenu(menuBar, appCore, appInstance, &odAppMenu);
    BuildScriptsMenu(menuBar, ScriptsDirectory);
    syncMenusWithRendererState();

    appCore->setContextMenuCallback(ContextMenu);

    bReady = true;
    
    // Get the current time
    time_t systime = time(NULL);
    struct tm *gmt = gmtime(&systime);
    double timeTDB = astro::J2000;
    if (gmt != NULL)
    {
        astro::Date d;
        d.year = gmt->tm_year + 1900;
        d.month = gmt->tm_mon + 1;
        d.day = gmt->tm_mday;
        d.hour = gmt->tm_hour;
        d.minute = gmt->tm_min;
        d.seconds = (int) gmt->tm_sec;
        timeTDB = astro::UTCtoTDB(d);
    }
    appCore->start(timeTDB);

    if (startURL != "")
    {
        COPYDATASTRUCT cd;
        cd.dwData = 0;
        cd.cbData = startURL.length();
        cd.lpData = reinterpret_cast<void*>(const_cast<char*>(startURL.c_str()));
        SendMessage(mainWindow, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cd));
    }

    // Initial tick required before first frame is rendered; this gives
    // start up scripts a chance to run.
    appCore->tick();

    MSG msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
        bool isVisible = !IsIconic(mainWindow);

        // If Celestia is in an inactive state, use GetMessage to avoid
        // sucking CPU cycles.  If time is paused and the camera isn't
        // moving in any view, we can probably also avoid constantly
        // rendering.
        BOOL haveMessage;
        if (isVisible)
            haveMessage = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);
        else
            haveMessage = GetMessage(&msg, NULL, 0U, 0U);

        if (!haveMessage)
        {
            // Tick the simulation
            appCore->tick();
        }

        if (haveMessage)
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
            if (eclipseFinder != NULL &&
                IsDialogMessage(eclipseFinder->hwnd, &msg))
                dialogMessage = true;
            if (locationsDlg != NULL &&
                IsDialogMessage(locationsDlg->hwnd, &msg))
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
    if (currentScreenMode != 0)
        RestoreDisplayMode();
    DestroyOpenGLWindow();

    if (appCore != NULL)
        delete appCore;

    OleUninitialize();

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
        //Instruct menu class to enumerate menu structure
        odAppMenu.Init(hWnd, menuBar);

        //Associate some menu items with bitmap resources
        odAppMenu.SetItemImage(appInstance, ID_FILE_OPENSCRIPT, IDB_SCRIPT);
        odAppMenu.SetItemImage(appInstance, ID_FILE_CAPTUREIMAGE, IDB_CAMERA);
        odAppMenu.SetItemImage(appInstance, ID_FILE_CAPTUREMOVIE, IDB_CAMCORDER);
        odAppMenu.SetItemImage(appInstance, ID_FILE_EXIT, IDB_EXIT);
        odAppMenu.SetItemImage(appInstance, ID_TIME_SETTIME, IDB_CLOCK);
        odAppMenu.SetItemImage(appInstance, ID_TIME_FREEZE, IDB_STOP);
        odAppMenu.SetItemImage(appInstance, ID_RENDER_VIEWOPTIONS, IDB_SUNGLASSES);
        odAppMenu.SetItemImage(appInstance, ID_RENDER_LOCATIONS, IDB_GLOBE);
        odAppMenu.SetItemImage(appInstance, ID_HELP_RUNDEMO, IDB_SCRIPT);
        odAppMenu.SetItemImage(appInstance, ID_HELP_CONTROLS, IDB_CONFIG);
        odAppMenu.SetItemImage(appInstance, ID_HELP_ABOUT, IDB_ABOUT);

        DragAcceptFiles(hWnd, TRUE);
        break;

    case WM_DROPFILES:
        break;

    case WM_MEASUREITEM:
        odAppMenu.MeasureItem(hWnd, lParam);
        return TRUE;

    case WM_DRAWITEM:
        odAppMenu.DrawItem(hWnd, lParam);
        return TRUE;

    case WM_MOUSEMOVE:
        {
	    int x, y;
	    x = LOWORD(lParam);
	    y = HIWORD(lParam);

            bool reallyMoved = x != lastMouseMove.x || y != lastMouseMove.y;
            lastMouseMove.x = x;
            lastMouseMove.y = y;

            if (reallyMoved)
            {
                appCore->mouseMove((float) x, (float) y);

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
                    if (ignoreNextMoveEvent)
                    {
                        // This hack is required because there's a move event
                        // right after canceling a context menu by clicking
                        // outside of it.  Because it was canceled by clicking,
                        // the mouse button down bits are set, and the infinite
                        // mouse code gets confused.
                        ignoreNextMoveEvent = false;
                    }
                    else if (cursorVisible)
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
                appCore->mouseMove((float) (x - lastX),
                                   (float) (y - lastY),
                                   buttons);

                if (currentScreenMode != 0)
                {
                    if (hideMenuBar && y < 10)
                    {
                        SetMenu(mainWindow, menuBar);
                        hideMenuBar = false;
                    }
                    else if (!hideMenuBar && y >= 10)
                    {
                        SetMenu(mainWindow, NULL);
                        hideMenuBar = true;
                    }
                }
            }
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

    case WM_MBUTTONUP:
        lastX = LOWORD(lParam);
        lastY = HIWORD(lParam);
        appCore->mouseButtonUp(LOWORD(lParam), HIWORD(lParam),
                               CelestiaCore::MiddleButton);
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
        case VK_INSERT:
        case 'C':
            if ((GetKeyState(VK_LCONTROL) | GetKeyState(VK_RCONTROL)) & 0x8000)
            {
                CopyStateURLToClipboard();
                appCore->flash(_("Copied URL"));
            }
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
            {
                break;
            }

            int charCode = (char) wParam;
            int modifiers = 0;
            if (GetKeyState(VK_SHIFT) & 0x8000)
                modifiers |= CelestiaCore::ShiftKey;

            // Catch backtab (shift+Tab)
            if (charCode == '\011' && (modifiers & CelestiaCore::ShiftKey))
                charCode = CelestiaCore::Key_BackTab;

            Renderer* r = appCore->getRenderer();
            int oldRenderFlags = r->getRenderFlags();
            int oldLabelMode = r->getLabelMode();

            //  Convert charCode from current locale to UTF-8
            char utf8CharCode[7];
            memset(utf8CharCode, 0, sizeof(utf8CharCode));
            WCHAR wCharCode;
            MultiByteToWideChar(CP_ACP, 0, (char*)&charCode, 1, &wCharCode, 1);
            WideCharToMultiByte(CP_UTF8, 0, &wCharCode, 1, utf8CharCode, 7, 0, 0);

            /*cerr << "Char input: (ANSI) " << (int)(unsigned char)charCode << " - UTF8 -> ";
            for(int i=0; utf8CharCode[i] != '\0'; i++) cerr << (int)(unsigned char)(utf8CharCode[i]) << " ";
            cerr << "[" << utf8CharCode << "]" << endl;*/

            Renderer::StarStyle oldStarStyle = r->getStarStyle();
            appCore->charEntered(utf8CharCode, modifiers);
            if (r->getRenderFlags() != oldRenderFlags ||
                r->getLabelMode() != oldLabelMode ||
                r->getStarStyle() != oldStarStyle)
            {
                syncMenusWithRendererState();
            }
        }
        break;

    case WM_IME_CHAR:
        {
            char ch[2];
            char utf8CharCode[7];
            memset(utf8CharCode, 0, sizeof(utf8CharCode));
            WCHAR wCharCode;
            ch[0] = (wParam >> 8);
            ch[1] = (wParam & 0xff);
            if (ch[0]) MultiByteToWideChar(CP_ACP, 0, ch, 2, &wCharCode, 1);
            else MultiByteToWideChar(CP_ACP, 0, &ch[1], 1, &wCharCode, 1);
            WideCharToMultiByte(CP_UTF8, 0, &wCharCode, 1, utf8CharCode, 7, 0, 0);
            appCore->charEntered(utf8CharCode);
            /*cerr << "IME input: (ANSI) " << (int)(unsigned char)ch[0] << " " << (int)(unsigned char)ch[1] << " - UTF8 -> ";
            for(int i=0; utf8CharCode[i] != '\0'; i++) cerr << (int)(unsigned char)(utf8CharCode[i]) << " ";
            cerr << "[" << utf8CharCode << "]" << endl;*/
        }
        break;

    case WM_COPYDATA:
        // The copy data message is used to send URL strings between
        // processes.
        {
            COPYDATASTRUCT* cd = reinterpret_cast<COPYDATASTRUCT*>(lParam);
            if (cd != NULL && cd->lpData != NULL)
            {
                char* urlChars = reinterpret_cast<char*>(cd->lpData);
                if (cd->cbData > 3) // minimum of "cel:" or ".cel"
                {
                    string urlString(urlChars, cd->cbData);

                    if (!urlString.substr(0,4).compare("cel:"))
                    {
                        appCore->flash(_("Loading URL"));
                        appCore->goToUrl(urlString);
                    }
                    else if (DetermineFileType(urlString) == Content_CelestiaScript)
                    {
                        appCore->runScript(urlString);
                    }
                    else
                    {
                        ifstream scriptfile(urlString.c_str());
                        if (!scriptfile.good())
                        {
                            appCore->flash(_("Error opening script"));
                        }
                        else
                        {
                            // TODO: Need to fix memory leak with scripts;
                            // a refcount is probably required.
                            CommandParser parser(scriptfile);
                            CommandSequence* script = parser.parse();
                            if (script == NULL)
                            {
                                const vector<string>* errors = parser.getErrors();
                                const char* errorMsg = "";
                                if (errors->size() > 0)
                                {
                                    errorMsg = (*errors)[0].c_str();
                                    appCore->flash(errorMsg);
                                }
                                else
                                {
                                    appCore->flash(_("Error loading script"));
                                }
                            }
                            else
                            {
                                appCore->flash(_("Running script"));
                                appCore->runScript(script);
                            }
                        }
                    }
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_NAVIGATION_CENTER:
            appCore->charEntered('c');
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
            DialogBox(hRes, MAKEINTRESOURCE(IDD_FINDOBJECT), hWnd, FindObjectProc);
            break;
        case ID_NAVIGATION_GOTO_OBJECT:
            if (gotoObjectDlg == NULL)
                gotoObjectDlg = new GotoObjectDialog(hRes, hWnd, appCore);
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
            else if (reinterpret_cast<LPARAM>(eclipseFinder) == lParam &&
                     eclipseFinder != NULL)
            {
                delete eclipseFinder;
                eclipseFinder = NULL;
            }
            else if (reinterpret_cast<LPARAM>(locationsDlg) == lParam &&
                     locationsDlg != NULL)
            {
                delete locationsDlg;
                locationsDlg = NULL;
            }
            break;

        case ID_NAVIGATION_TOURGUIDE:
            if (tourGuide == NULL)
                tourGuide = new TourGuide(hRes, hWnd, appCore);
            break;

        case ID_NAVIGATION_SSBROWSER:
            if (solarSystemBrowser == NULL)
                solarSystemBrowser = new SolarSystemBrowser(hRes, hWnd, appCore);
            break;

        case ID_NAVIGATION_STARBROWSER:
            if (starBrowser == NULL)
                starBrowser = new StarBrowser(hRes, hWnd, appCore);
            break;

        case ID_NAVIGATION_ECLIPSEFINDER:
            if (eclipseFinder == NULL)
                eclipseFinder = new EclipseFinderDialog(hRes, hWnd, appCore);
            break;

        case ID_RENDER_DISPLAYMODE:
            newScreenMode = currentScreenMode;
            CreateDialogParam(hRes,
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
                viewOptionsDlg = new ViewOptionsDialog(hRes, hWnd, appCore);
            break;

        case ID_RENDER_LOCATIONS:
            if (locationsDlg == NULL)
                locationsDlg = new LocationsDialog(hRes, hWnd, appCore);
            break;

        case ID_RENDER_MORESTARS:
            appCore->charEntered(']');
            break;

        case ID_RENDER_FEWERSTARS:
            appCore->charEntered('[');
            break;

        case ID_RENDER_AUTOMAG:
            appCore->charEntered('\031');
            syncMenusWithRendererState();
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

        case ID_RENDER_STARSTYLE_FUZZY:
            appCore->getRenderer()->setStarStyle(Renderer::FuzzyPointStars);
            syncMenusWithRendererState();
            break;

        case ID_RENDER_STARSTYLE_POINTS:
            appCore->getRenderer()->setStarStyle(Renderer::PointStars);
            syncMenusWithRendererState();
            break;

        case ID_RENDER_STARSTYLE_DISCS:
            appCore->getRenderer()->setStarStyle(Renderer::ScaledDiscStars);
            syncMenusWithRendererState();
            break;

        case ID_RENDER_TEXTURERES_LOW:
            appCore->getRenderer()->setResolution(0);
            syncMenusWithRendererState();
            break;
        case ID_RENDER_TEXTURERES_MEDIUM:
            appCore->getRenderer()->setResolution(1);
            syncMenusWithRendererState();
            break;
        case ID_RENDER_TEXTURERES_HIGH:
            appCore->getRenderer()->setResolution(2);
            syncMenusWithRendererState();
            break;

        case ID_RENDER_ANTIALIASING:
            appCore->charEntered('\030');
            syncMenusWithRendererState();
            break;

        case ID_RENDER_BODY_AXES:
            appCore->toggleReferenceMark("body axes");
            break;

        case ID_RENDER_FRAME_AXES:
            appCore->toggleReferenceMark("frame axes");
            break;

        case ID_RENDER_SUN_DIRECTION:
            appCore->toggleReferenceMark("sun direction");
            break;

        case ID_RENDER_VELOCITY_VECTOR:
            appCore->toggleReferenceMark("velocity vector");
            break;

        case ID_RENDER_PLANETOGRAPHIC_GRID:
            appCore->toggleReferenceMark("planetographic grid");
            break;

        case ID_RENDER_TERMINATOR:
            appCore->toggleReferenceMark("terminator");
            break;

        case ID_TIME_FASTER:
            appCore->charEntered('l');
            break;
        case ID_TIME_SLOWER:
            appCore->charEntered('k');
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
            ShowSetTimeDialog(hRes, hWnd, appCore);
            
            // Update the local time menu item--since the set time dialog handles setting the time zone,
            // should we just get rid of the menu item?
            if (appCore->getTimeZoneBias() == 0)
                CheckMenuItem(menuBar, ID_TIME_SHOWLOCAL, MF_UNCHECKED);
            else
                CheckMenuItem(menuBar, ID_TIME_SHOWLOCAL, MF_CHECKED);
            break;
        case ID_TIME_SHOWLOCAL:
            if (ToggleMenuItem(menuBar, ID_TIME_SHOWLOCAL))
                ShowLocalTime(appCore);
            else
                ShowUniversalTime(appCore);
            break;

        case ID_VIEW_HSPLIT:
            appCore->splitView(View::HorizontalSplit);
            break;

        case ID_VIEW_VSPLIT:
            appCore->splitView(View::VerticalSplit);
            break;

        case ID_VIEW_SINGLE:
            appCore->singleView();
            break;

        case ID_VIEW_DELETE_ACTIVE:
            appCore->deleteView();
            break;

        case ID_VIEW_SHOW_FRAMES:
            appCore->setFramesVisible(!appCore->getFramesVisible());
            syncMenusWithRendererState();
            break;

        case ID_VIEW_SYNC_TIME:
            {
                Simulation* sim = appCore->getSimulation();
                sim->setSyncTime(!sim->getSyncTime());
                if (sim->getSyncTime())
                    sim->synchronizeTime();
                syncMenusWithRendererState();
            }
            break;

        case ID_BOOKMARKS_ADDBOOKMARK:
            DialogBox(hRes, MAKEINTRESOURCE(IDD_ADDBOOKMARK), hWnd, AddBookmarkProc);
            break;

        case ID_BOOKMARKS_ORGANIZE:
            DialogBox(hRes, MAKEINTRESOURCE(IDD_ORGANIZE_BOOKMARKS), hWnd, OrganizeBookmarksProc);
            break;

        case ID_HELP_RUNDEMO:
            appCore->charEntered('D');
            break;

        case ID_HELP_CONTROLS:
            CreateDialogParam(hRes,
                              MAKEINTRESOURCE(IDD_CONTROLSHELP),
                              hWnd,
                              ControlsHelpProc,
                              NULL);
            break;

        case ID_HELP_ABOUT:
            DialogBox(hRes, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutProc);
            break;

        case ID_HELP_GLINFO:
            DialogBox(hRes, MAKEINTRESOURCE(IDD_GLINFO), hWnd, GLInfoProc);
            break;

        case ID_HELP_LICENSE:
            DialogBox(hRes, MAKEINTRESOURCE(IDD_LICENSE), hWnd, LicenseProc);
            break;

        case ID_INFO:
            ShowWWWInfo(appCore->getSimulation()->getSelection());
            break;

        case ID_FILE_OPENSCRIPT:
            HandleOpenScript(hWnd, appCore);
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

        case ID_GOTO_URL:
            {
                // Relies on a pointer in lparam, do this does not
                // work cross-process.
                char* urlString = reinterpret_cast<char*>(lParam);
                if (urlString != NULL)
                {
                    appCore->flash(string("URL: ") + string(urlString));
					appCore->goToUrl(urlString);
                }
            }
            break;

        case ID_TOOLS_MARK:
            {
                Simulation* sim = appCore->getSimulation();
                if (sim->getUniverse() != NULL)
                {
                    MarkerRepresentation markerRep(MarkerRepresentation::Diamond,
                                                   10.0f,
                                                   Color(0.0f, 1.0f, 0.0f, 0.9f));
						 
                    sim->getUniverse()->markObject(sim->getSelection(),
                                                   markerRep,
                                                   1);

                    appCore->getRenderer()->setRenderFlags(appCore->getRenderer()->getRenderFlags() | Renderer::ShowMarkers);
                }
            }
            break;

        case ID_TOOLS_UNMARK:
            {
                Simulation* sim = appCore->getSimulation();
                if (sim->getUniverse() != NULL)
                    sim->getUniverse()->unmarkObject(sim->getSelection(), 1);
            }
            break;

        default:
            {
                const FavoritesList* favorites = appCore->getFavorites();
                if (favorites != NULL &&
                    LOWORD(wParam) >= ID_BOOKMARKS_FIRSTBOOKMARK &&
                    LOWORD(wParam) - ID_BOOKMARKS_FIRSTBOOKMARK < (int) favorites->size())
                {
                    int whichFavorite = LOWORD(wParam) - ID_BOOKMARKS_FIRSTBOOKMARK;
                    appCore->activateFavorite(*(*favorites)[whichFavorite]);
                }
                else if (LOWORD(wParam) >= MENU_CHOOSE_PLANET &&
                         LOWORD(wParam) < MENU_CHOOSE_PLANET + 1000)
                {
                    // Handle the satellite/child object submenu
                    Selection sel = appCore->getSimulation()->getSelection();
                    switch (sel.getType())
                    {
                    case Selection::Type_Star:
                        appCore->getSimulation()->selectPlanet(LOWORD(wParam) - MENU_CHOOSE_PLANET);
                        break;

                    case Selection::Type_Body:
                        {
                            PlanetarySystem* satellites = (PlanetarySystem*) sel.body()->getSatellites();
                            appCore->getSimulation()->setSelection(Selection(satellites->getBody(LOWORD(wParam) - MENU_CHOOSE_PLANET)));
                            break;
                        }

                    case Selection::Type_DeepSky:
                        // Current deep sky object/galaxy implementation does
                        // not have children to select.
                        break;

                    case Selection::Type_Location:
                        break;

                    default:
                        break;
                    }
                }
                else if (LOWORD(wParam) >= MENU_CHOOSE_SURFACE &&
                         LOWORD(wParam) < MENU_CHOOSE_SURFACE + 1000)
                {
                    // Handle the alternate surface submenu
                    Selection sel = appCore->getSimulation()->getSelection();
                    if (sel.body() != NULL)
                    {
                        int index = (int) LOWORD(wParam) - MENU_CHOOSE_SURFACE - 1;
                        vector<string>* surfNames = sel.body()->getAlternateSurfaceNames();
                        if (surfNames != NULL)
                        {
                            string surfName;
                            if (index >= 0 && index < (int) surfNames->size())
                                surfName = surfNames->at(index);
                            appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(surfName);
                            delete surfNames;
                        }
                    }
                }
                else if (LOWORD(wParam) >= ID_FIRST_SCRIPT &&
                         LOWORD(wParam) <  ID_FIRST_SCRIPT + ScriptMenuItems->size())
                {
                    // Handle the script menu
                    unsigned int scriptIndex = LOWORD(wParam) - ID_FIRST_SCRIPT;
                    appCore->runScript((*ScriptMenuItems)[scriptIndex].filename);
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
