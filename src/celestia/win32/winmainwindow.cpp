// winmainwindow.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// The main application window.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winmainwindow.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include <epoxy/wgl.h>

#include <fmt/format.h>

#include <celengine/body.h>
#include <celengine/glsupport.h>
#include <celengine/marker.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include <celestia/celestiastate.h>
#include <celestia/configfile.h>
#include <celestia/helper.h>
#include <celestia/scriptmenu.h>
#include <celestia/url.h>
#include <celutil/gettext.h>
#include <celutil/logger.h>
#include <celutil/tzutil.h>

#include <joystickapi.h>
#include <shellapi.h>

#include "res/resource.h"
#include "odmenu.h"
#include "tstring.h"
#include "winbookmarks.h"
#include "wincontextmenu.h"
#include "windisplaymodedlg.h"
#include "wineclipses.h"
#include "winfiledlgs.h"
#include "winfinddlg.h"
#include "wingotodlg.h"
#include "winhelpdlgs.h"
#include "winlocations.h"
#include "winpreferences.h"
#ifdef USE_FFMPEG
#include "winmoviecapture.h"
#endif
#include "winssbrowser.h"
#include "winstarbrowser.h"
#include "wintime.h"
#include "wintourguide.h"
#include "winviewoptsdlg.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace celestia::win32
{

namespace
{

bool
ToggleMenuItem(HMENU menu, int id)
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

void
setMenuItemCheck(HMENU menuBar, int menuItem, bool checked)
{
    CheckMenuItem(menuBar, menuItem, checked ? MF_CHECKED : MF_UNCHECKED);
}

// Version of the function from tstring.h with smaller intermediate buffer
// suitable for char event processing
template<typename T, std::enable_if_t<std::is_same_v<typename T::value_type, char>, int> = 0>
void
AppendTCharCodeToUTF8(TCHAR* tch, int nch, T& destination)
{
    if (nch <= 0)
        return;
    tstring_view source(tch, static_cast<std::size_t>(nch));
#ifdef _UNICODE
    AppendTCharToUTF8(source, destination);
#else
    fmt::basic_memory_buffer<wchar_t, 4> wbuffer;
    int wlength = MultiByteToWideChar(CP_ACP, 0, tch, nch, nullptr, 0);
    if (wlength <= 0)
        return;

    wbuffer.resize(static_cast<std::size_t>(wlength));
    wlength = MultiByteToWideChar(CP_ACP, 0, tch, nch, wbuffer.data(), wlength);
    if (wlength <= 0)
        return;

    int length = WideCharToMultiByte(CP_UTF8, 0, wbuffer.data(), wlength, nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return;

    const auto existingSize = destination.size();
    destination.resize(existingSize + static_cast<std::size_t>(length));
    length = WideCharToMultiByte(CP_UTF8, 0, wbuffer.data(), wlength,
                                 destination.data() + existingSize, length,
                                 nullptr, nullptr);
    destination.resize(length >= 0 ? (existingSize + static_cast<std::size_t>(length)) : existingSize);
#endif
}

unsigned int
ChooseBestMSAAPixelFormat(HDC hdc, int *formats, unsigned int numFormats,
                          int samplesRequested)
{
    int idealFormat = 0;
    int bestFormat  = 0;
    int bestSamples = 0;

    for (unsigned int i = 0; i < numFormats; i++)
    {
        int query = WGL_SAMPLES_ARB;
        int result = 0;

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
bool
SetDCPixelFormat(HDC hDC, const CelestiaCore* appCore)
{
    bool msaa = false;
    if (appCore->getConfig()->renderDetails.aaSamples > 1 &&
        epoxy_has_wgl_extension(hDC, "WGL_ARB_pixel_format") &&
        epoxy_has_wgl_extension(hDC, "WGL_ARB_multisample"))
    {
        msaa = true;
    }

    if (!msaa)
    {
        static PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),    // Size of this structure
            1,                // Version of this structure
            PFD_DRAW_TO_WINDOW |    // Draw to Window (not to bitmap)
            PFD_SUPPORT_OPENGL |    // Support OpenGL calls in window
            PFD_DOUBLEBUFFER,        // Double buffered mode
            PFD_TYPE_RGBA,        // RGBA Color mode
            (BYTE)GetDeviceCaps(hDC, BITSPIXEL),// Want the display bit depth
            0,0,0,0,0,0,          // Not used to select mode
            0,0,            // Not used to select mode
            0,0,0,0,0,            // Not used to select mode
            24,                // Size of depth buffer
            0,                // Not used to select mode
            0,                // Not used to select mode
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
            WGL_DRAW_TO_WINDOW_ARB, TRUE,
            WGL_SUPPORT_OPENGL_ARB, TRUE,
            WGL_DOUBLE_BUFFER_ARB,  TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_COLOR_BITS_ARB,     24,
            WGL_RED_BITS_ARB,       8,
            WGL_GREEN_BITS_ARB,     8,
            WGL_BLUE_BITS_ARB,      8,
            WGL_ALPHA_BITS_ARB,     0,
            WGL_ACCUM_BITS_ARB,     0,
            WGL_STENCIL_BITS_ARB,   0,
            WGL_SAMPLE_BUFFERS_ARB, appCore->getConfig()->renderDetails.aaSamples > 1,
            0
        };

        int             pixelFormatIndex;
        int             pixFormats[256];
        unsigned int    numFormats;

        wglChoosePixelFormatARB(hDC, ifmtList, NULL, 256, pixFormats, &numFormats);

        pixelFormatIndex = ChooseBestMSAAPixelFormat(hDC, pixFormats,
                                                     numFormats,
                                                     appCore->getConfig()->renderDetails.aaSamples);

        DescribePixelFormat(hDC, pixelFormatIndex,
                            sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        if (!SetPixelFormat(hDC, pixelFormatIndex, &pfd))
            return false;

        return true;
    }
}

bool
InitJoystick()
{
    int nJoysticks = joyGetNumDevs();
    if (nJoysticks <= 0)
        return false;

    JOYCAPS caps;
    if (joyGetDevCaps(JOYSTICKID1, &caps, sizeof(caps)) != JOYERR_NOERROR)
    {
        GetLogger()->error(_("Error getting joystick caps.\n"));
        return false;
    }

    GetLogger()->error(_("Using joystick: {}\n"), TCharToUTF8String(caps.szPname));

    return true;
}

} // end unnamed namespace

MainWindow::MainWindow(HINSTANCE _appInstance,
                       HMODULE _hRes,
                       HMENU _menuBar,
                       ODMenu* _odAppMenu,
                       CelestiaCore* _appCore,
                       int _fullScreenMode,
                       util::array_view<DEVMODE> _displayModes) :
    appInstance(_appInstance),
    hRes(_hRes),
    menuBar(_menuBar),
    odAppMenu(_odAppMenu),
    appCore(_appCore),
    lastFullScreenMode(_fullScreenMode),
    displayModes(_displayModes)
{
    joystickAvailable = InitJoystick();
}

MainWindow::~MainWindow() = default;

LRESULT
MainWindow::create(HWND _hWnd)
{
    hWnd = _hWnd;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(this));
    //Instruct menu class to enumerate menu structure
    odAppMenu->Init(hWnd, menuBar);

    //Associate some menu items with bitmap resources
    odAppMenu->SetItemImage(appInstance, ID_FILE_OPENSCRIPT, IDB_SCRIPT);
    odAppMenu->SetItemImage(appInstance, ID_FILE_RUNDEMO, IDB_SCRIPT);
    odAppMenu->SetItemImage(appInstance, ID_FILE_CAPTUREIMAGE, IDB_CAMERA);
    odAppMenu->SetItemImage(appInstance, ID_FILE_CAPTUREMOVIE, IDB_CAMCORDER);
    odAppMenu->SetItemImage(appInstance, ID_FILE_EXIT, IDB_EXIT);
    odAppMenu->SetItemImage(appInstance, ID_TIME_SETTIME, IDB_CLOCK);
    odAppMenu->SetItemImage(appInstance, ID_TIME_FREEZE, IDB_STOP);
    odAppMenu->SetItemImage(appInstance, ID_RENDER_VIEWOPTIONS, IDB_SUNGLASSES);
    odAppMenu->SetItemImage(appInstance, ID_RENDER_LOCATIONS, IDB_GLOBE);
    odAppMenu->SetItemImage(appInstance, ID_HELP_CONTROLS, IDB_CONFIG);
    odAppMenu->SetItemImage(appInstance, ID_HELP_ABOUT, IDB_ABOUT);
    odAppMenu->SetItemImage(appInstance, ID_BOOKMARKS_ADDBOOKMARK, IDB_BOOKMARK_ADD);
    odAppMenu->SetItemImage(appInstance, ID_BOOKMARKS_ORGANIZE, IDB_BOOKMARK_ORGANIZE);
    odAppMenu->SetItemImage(appInstance, ID_TIME_REALTIME, IDB_TIME_ADD);
    odAppMenu->SetItemImage(appInstance, ID_TIME_REVERSE, IDB_TIME_REMOVE);
    odAppMenu->SetItemImage(appInstance, ID_TIME_FASTER, IDB_TIME_FORWARD);
    odAppMenu->SetItemImage(appInstance, ID_TIME_SLOWER, IDB_TIME_BACKWARD);
    odAppMenu->SetItemImage(appInstance, ID_RENDER_DISPLAYMODE, IDB_DISPLAY_MODE);
    odAppMenu->SetItemImage(appInstance, ID_RENDER_FULLSCREEN, IDB_DISPLAY_FULL_MODE);
    odAppMenu->SetItemImage(appInstance, ID_VIEW_HSPLIT, IDB_DISPLAY_VIEW_HSPLIT);
    odAppMenu->SetItemImage(appInstance, ID_VIEW_VSPLIT, IDB_DISPLAY_VIEW_VSPLIT);
    odAppMenu->SetItemImage(appInstance, ID_VIEW_DELETE_ACTIVE, IDB_DISPLAY_VIEW_DELETE_ACTIVE);
    odAppMenu->SetItemImage(appInstance, ID_VIEW_SINGLE, IDB_DISPLAY_VIEW_SINGLE);
    odAppMenu->SetItemImage(appInstance, ID_HELP_GUIDE, IDB_DISPLAY_HELP_GUIDE);
    odAppMenu->SetItemImage(appInstance, ID_HELP_GLINFO, IDB_DISPLAY_HELP_GLINFO);
    odAppMenu->SetItemImage(appInstance, ID_HELP_LICENSE, IDB_DISPLAY_VIEW_HELP_LICENSE);
    odAppMenu->SetItemImage(appInstance, ID_NAVIGATION_HOME, IDB_NAV_HOME);
    odAppMenu->SetItemImage(appInstance, ID_NAVIGATION_TOURGUIDE, IDB_NAV_TOURGUIDE);
    odAppMenu->SetItemImage(appInstance, ID_NAVIGATION_SSBROWSER, IDB_NAV_SSBROWSER);
    odAppMenu->SetItemImage(appInstance, ID_NAVIGATION_STARBROWSER, IDB_NAV_STARBROWSER);
    odAppMenu->SetItemImage(appInstance, ID_NAVIGATION_ECLIPSEFINDER, IDB_NAV_ECLIPSEFINDER);

    DragAcceptFiles(hWnd, TRUE);

    return 0;
}

LRESULT
MainWindow::measureItem(LPARAM lParam) const
{
    odAppMenu->MeasureItem(hWnd, lParam);
    return TRUE;
}

LPARAM
MainWindow::drawItem(LPARAM lParam) const
{
    odAppMenu->DrawItem(hWnd, lParam);
    return TRUE;
}

void
MainWindow::mouseMove(WPARAM wParam, LPARAM lParam)
{
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);

    bool reallyMoved = x != lastMouseMove.x || y != lastMouseMove.y;
    lastMouseMove.x = x;
    lastMouseMove.y = y;

    if (!reallyMoved)
        return;

    appCore->mouseMove(static_cast<float>(x), static_cast<float>(y));

    if ((wParam & (MK_LBUTTON | MK_RBUTTON)) != 0)
    {
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
    appCore->mouseMove(static_cast<float>(x - lastX),
                       static_cast<float>(y - lastY),
                       buttons);

    if (isFullScreen)
    {
        if (menuBarHidden && y < 10)
        {
            SetMenu(hWnd, menuBar);
            menuBarHidden = false;
        }
        else if (!menuBarHidden && y >= 10)
        {
            SetMenu(hWnd, NULL);
            menuBarHidden = true;
        }
    }
}

void
MainWindow::buttonDown(LPARAM lParam, int button)
{
    lastX = LOWORD(lParam);
    lastY = HIWORD(lParam);
    appCore->mouseButtonDown(static_cast<float>(lastX),
                             static_cast<float>(lastY),
                             button);
}

void
MainWindow::buttonUp(LPARAM lParam, int button)
{
    if (!cursorVisible && (button == CelestiaCore::LeftButton || button == CelestiaCore::RightButton))
        restoreCursor();
    appCore->mouseButtonUp(static_cast<float>(LOWORD(lParam)),
                           static_cast<float>(HIWORD(lParam)),
                           button);
}

void
MainWindow::mouseWheel(WPARAM wParam) const
{
    int modifiers = 0;
    if ((wParam & MK_SHIFT) != 0)
        modifiers |= CelestiaCore::ShiftKey;

    float delta = static_cast<SHORT>(HIWORD(wParam)) > 0 ? -1.0f : 1.0f;
    appCore->mouseWheel(delta, modifiers);
}

void
MainWindow::keyDown(WPARAM wParam)
{
    switch (wParam)
    {
    case VK_ESCAPE:
        appCore->charEntered('\033');
        break;
    case VK_INSERT:
    case 'C':
        if ((GetKeyState(VK_LCONTROL) | GetKeyState(VK_RCONTROL)) & 0x8000)
        {
            copyStateURLToClipboard();
            appCore->flash(_("Copied URL"));
        }
        break;
    default:
        handleKey(wParam, true);
        break;
    }
}

void
MainWindow::handleKey(WPARAM wParam, bool down)
{
    int k = -1;
    int modifiers = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000)
        modifiers |= CelestiaCore::ShiftKey;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        modifiers |= CelestiaCore::ControlKey;

    switch (wParam)
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
            appCore->charEntered((char) wParam, modifiers);
        break;

    case 'A':
    case 'Z':
        if ((GetKeyState(VK_CONTROL) & 0x8000) == 0)
            k = wParam;
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

void
MainWindow::processChar(WPARAM wParam, LPARAM lParam) const
{
    // Bits 16-23 of lParam specify the scan code of the key pressed.

    // Ignore all keypad input, this will be handled by WM_KEYDOWN
    // messages.
    char cScanCode = static_cast<char>(HIWORD(lParam) & 0xFF);
    if ((cScanCode >= 71 && cScanCode <= 73) ||
        (cScanCode >= 75 && cScanCode <= 77) ||
        (cScanCode >= 79 && cScanCode <= 83))
    {
        return;
    }

    auto charCode = static_cast<TCHAR>(wParam);
    int modifiers = 0;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        modifiers |= CelestiaCore::ShiftKey;

    Renderer* r = appCore->getRenderer();
    std::uint64_t oldRenderFlags = r->getRenderFlags();
    int oldLabelMode = r->getLabelMode();
    Renderer::StarStyle oldStarStyle = r->getStarStyle();
    auto oldResolution = r->getResolution();
    auto oldColorTable = r->getStarColorTable();

    // Catch backtab (Shift+Tab)
    if (charCode == TEXT('\011') && (modifiers & CelestiaCore::ShiftKey))
    {
        appCore->charEntered(CelestiaCore::Key_BackTab, modifiers);
    }
    else
    {
        // Convert charCode to UTF-8
        fmt::basic_memory_buffer<char, 8> buffer;
        AppendTCharCodeToUTF8(&charCode, 1, buffer);
        buffer.push_back('\0');
        appCore->charEntered(buffer.data(), modifiers);
    }

    if (r->getRenderFlags() != oldRenderFlags ||
        r->getLabelMode() != oldLabelMode ||
        r->getStarStyle() != oldStarStyle ||
        r->getResolution() != oldResolution ||
        r->getStarColorTable() != oldColorTable)
    {
        SyncMenusWithRendererState(appCore, menuBar);
    }
}

void
MainWindow::imeChar(WPARAM wParam) const
{
    fmt::basic_memory_buffer<char, 16> buffer;
#ifdef _UNICODE
    auto charCode = static_cast<TCHAR>(wParam);
    AppendTCharCodeToUTF8(&charCode, 1, buffer);
#else
    std::array<char, 2> charSeq
    {
        static_cast<char>(wParam >> 8),
        static_cast<char>(wParam & 0x0ff),
    };
    if (charSeq[0])
        AppendTCharCodeToUTF8(charSeq.data(), 2, buffer);
    else
        AppendTCharCodeToUTF8(charSeq.data() + 1, 1, buffer);
#endif
    buffer.push_back('\0');
    appCore->charEntered(buffer.data());
}

void
MainWindow::copyData(LPARAM lParam) const
{
    // The copy data message is used to send URL strings between processes.
    auto cd = reinterpret_cast<COPYDATASTRUCT*>(lParam);
    if (cd == nullptr || cd->lpData == nullptr)
        return;

    std::string_view url(static_cast<const char*>(cd->lpData), cd->cbData);
    while (!url.empty() && url.back() == '\0')
        url = url.substr(0, url.size() - 1);
    if (url.size() >= 4 && url.substr(0, 4) == "cel:"sv)
    {
        appCore->flash(_("Loading URL"));
        appCore->goToUrl(url);
    }
    else if ((url.size() >= 4 && url.substr(url.size() - 4) == ".cel"sv) ||
             (url.size() >= 5 && url.substr(url.size() - 5) == ".celx"sv))
    {
        appCore->runScript(fs::u8path(url));
    }
}

void
MainWindow::command(WPARAM wParam, LPARAM lParam)
{
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
        ShowFindObjectDialog(hRes, hWnd, appCore);
        break;

    case ID_NAVIGATION_GOTO_OBJECT:
        if (gotoObjectDlg == nullptr)
            gotoObjectDlg = std::make_unique<GotoObjectDialog>(hRes, hWnd, appCore);
        break;

    case ID_SELECT_PRIMARY_BODY:
        handleSelectPrimary();
        break;

    case IDCLOSE:
        if (gotoObjectDlg != nullptr && reinterpret_cast<LPARAM>(gotoObjectDlg.get()) == lParam)
            gotoObjectDlg.reset();
        else if (tourGuide != nullptr && reinterpret_cast<LPARAM>(tourGuide.get()) == lParam)
            tourGuide.reset();
        else if (starBrowser != nullptr && reinterpret_cast<LPARAM>(starBrowser.get()) == lParam)
            starBrowser.reset();
        else if (solarSystemBrowser != nullptr && reinterpret_cast<LPARAM>(solarSystemBrowser.get()) == lParam)
            solarSystemBrowser.reset();
        else if (viewOptionsDlg != nullptr && reinterpret_cast<LPARAM>(viewOptionsDlg.get()) == lParam)
            viewOptionsDlg.reset();
        else if (eclipseFinder != nullptr && reinterpret_cast<LPARAM>(eclipseFinder.get()) == lParam)
            eclipseFinder.reset();
        else if (locationsDlg != nullptr && reinterpret_cast<LPARAM>(locationsDlg.get()) == lParam)
            locationsDlg.reset();
        else if (displayModeDlg != nullptr && reinterpret_cast<LPARAM>(displayModeDlg.get()) == lParam)
        {
            if (displayModeDlg->update)
            {
                if (displayModeDlg->screenMode == 0)
                {
                    isFullScreen = false;
                }
                else
                {
                    isFullScreen = true;
                    lastFullScreenMode = displayModeDlg->screenMode;
                }
            }
            displayModeDlg.reset();
        }
        break;

    case ID_NAVIGATION_TOURGUIDE:
        if (tourGuide == nullptr)
            tourGuide = std::make_unique<TourGuide>(hRes, hWnd, appCore);
        break;

    case ID_NAVIGATION_SSBROWSER:
        if (solarSystemBrowser == nullptr)
            solarSystemBrowser = std::make_unique<SolarSystemBrowser>(hRes, hWnd, appCore);
        break;

    case ID_NAVIGATION_STARBROWSER:
        if (starBrowser == nullptr)
            starBrowser = std::make_unique<StarBrowser>(hRes, hWnd, appCore);
        break;

    case ID_NAVIGATION_ECLIPSEFINDER:
        if (eclipseFinder == nullptr)
            eclipseFinder = std::make_unique<EclipseFinderDialog>(hRes, hWnd, appCore);
        break;

    case ID_RENDER_DISPLAYMODE:
        if (displayModeDlg == nullptr)
            displayModeDlg = std::make_unique<DisplayModeDialog>(hRes, hWnd, displayModes, currentMode());
        break;

    case ID_RENDER_FULLSCREEN:
        isFullScreen = !isFullScreen;
        break;

    case ID_RENDER_VIEWOPTIONS:
        if (viewOptionsDlg == nullptr)
            viewOptionsDlg = std::make_unique<ViewOptionsDialog>(hRes, hWnd, appCore);
        break;

    case ID_RENDER_LOCATIONS:
        if (locationsDlg == nullptr)
            locationsDlg = std::make_unique<LocationsDialog>(hRes, hWnd, appCore);
        break;

    case ID_RENDER_MORESTARS:
        appCore->charEntered(']');
        break;

    case ID_RENDER_FEWERSTARS:
        appCore->charEntered('[');
        break;

    case ID_RENDER_AUTOMAG:
        appCore->charEntered('\031');
        SyncMenusWithRendererState(appCore, menuBar);
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
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_RENDER_STARSTYLE_POINTS:
        appCore->getRenderer()->setStarStyle(Renderer::PointStars);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_RENDER_STARSTYLE_DISCS:
        appCore->getRenderer()->setStarStyle(Renderer::ScaledDiscStars);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_STARCOLOR_CLASSIC:
        appCore->getRenderer()->setStarColorTable(ColorTableType::Enhanced);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_STARCOLOR_D65:
        appCore->getRenderer()->setStarColorTable(ColorTableType::Blackbody_D65);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_STARCOLOR_SOLAR:
        appCore->getRenderer()->setStarColorTable(ColorTableType::SunWhite);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_STARCOLOR_VEGA:
        appCore->getRenderer()->setStarColorTable(ColorTableType::VegaWhite);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_RENDER_TEXTURERES_LOW:
        appCore->getRenderer()->setResolution(0);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_RENDER_TEXTURERES_MEDIUM:
        appCore->getRenderer()->setResolution(1);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_RENDER_TEXTURERES_HIGH:
        appCore->getRenderer()->setResolution(2);
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_RENDER_ANTIALIASING:
        appCore->charEntered('\030');
        SyncMenusWithRendererState(appCore, menuBar);
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
        appCore->splitView(celestia::View::HorizontalSplit);
        break;

    case ID_VIEW_VSPLIT:
        appCore->splitView(celestia::View::VerticalSplit);
        break;

    case ID_VIEW_SINGLE:
        appCore->singleView();
        break;

    case ID_VIEW_DELETE_ACTIVE:
        appCore->deleteView();
        break;

    case ID_VIEW_SHOW_FRAMES:
        appCore->setFramesVisible(!appCore->getFramesVisible());
        SyncMenusWithRendererState(appCore, menuBar);
        break;

    case ID_VIEW_SYNC_TIME:
        {
            Simulation* sim = appCore->getSimulation();
            sim->setSyncTime(!sim->getSyncTime());
            if (sim->getSyncTime())
                sim->synchronizeTime();
            SyncMenusWithRendererState(appCore, menuBar);
        }
        break;

    case ID_BOOKMARKS_ADDBOOKMARK:
        ShowAddBookmarkDialog(appInstance, hRes, hWnd, menuBar, odAppMenu, appCore);
        break;

    case ID_BOOKMARKS_ORGANIZE:
        ShowOrganizeBookmarksDialog(appInstance, hRes, hWnd, menuBar, odAppMenu, appCore);
        break;

    case ID_HELP_GUIDE:
        ShellExecute(hWnd, TEXT("open"), TEXT("help\\CelestiaGuide.html"), NULL, NULL, SW_NORMAL);
        break;

    case ID_HELP_CONTROLS:
        ShowControlsDialog(hRes, hWnd);
        break;

    case ID_HELP_ABOUT:
        ShowAboutDialog(hRes, hWnd);
        break;

    case ID_HELP_GLINFO:
        ShowGLInfoDialog(hRes, hWnd, appCore);
        break;

    case ID_HELP_LICENSE:
        ShowLicenseDialog(hRes, hWnd);
        break;

    case ID_INFO:
        showWWWInfo();
        break;

    case ID_FILE_OPENSCRIPT:
        HandleOpenScript(hWnd, appCore);
        break;

    case ID_FILE_RUNDEMO:
        handleRunDemo();
        break;

    case ID_FILE_CAPTUREIMAGE:
        HandleCaptureImage(hWnd, appCore);
        break;

#ifdef USE_FFMPEG
    case ID_FILE_CAPTUREMOVIE:
        HandleCaptureMovie(appInstance, hWnd, appCore);
        break;
#endif

    case ID_FILE_EXIT:
        SendMessage(hWnd, WM_CLOSE, 0, 0);
        break;

    case ID_TOOLS_MARK:
        if (Simulation* sim = appCore->getSimulation(); sim->getUniverse() != nullptr)
        {
            using celestia::MarkerRepresentation;
            MarkerRepresentation markerRep(MarkerRepresentation::Diamond,
                                            10.0f,
                                            Color(0.0f, 1.0f, 0.0f, 0.9f));

            sim->getUniverse()->markObject(sim->getSelection(),
                                            markerRep,
                                            1);

            appCore->getRenderer()->setRenderFlags(appCore->getRenderer()->getRenderFlags() | Renderer::ShowMarkers);
        }
        break;

    case ID_TOOLS_UNMARK:
        if (Simulation* sim = appCore->getSimulation(); sim->getUniverse() != nullptr)
            sim->getUniverse()->unmarkObject(sim->getSelection(), 1);
        break;

    default:
        commandDynamicMenus(wParam, lParam);
        break;
    }
}

void
MainWindow::resize(LPARAM lParam) const
{
    appCore->resize(LOWORD(lParam), HIWORD(lParam));
}

void
MainWindow::paint() const
{
    if (!bReady)
        return;

    appCore->draw();
    SwapBuffers(deviceContext);
    ValidateRect(hWnd, NULL);
}

bool
MainWindow::setDeviceContext(util::array_view<std::string> ignoreGLExtensions)
{
    deviceContext = GetDC(hWnd);
    if (!SetDCPixelFormat(deviceContext, appCore))
    {
        tstring message = UTF8ToTString(_("Could not get appropriate pixel format for OpenGL rendering."));
        tstring caption = UTF8ToTString(_("Fatal Error"));
        MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    bool firstContext = false;
    if (glContext == nullptr)
    {
        glContext = wglCreateContext(deviceContext);
        firstContext = true;
    }
    wglMakeCurrent(deviceContext, glContext);

    if (firstContext)
    {
        if (!gl::init(ignoreGLExtensions) || !gl::checkVersion(gl::GL_2_1))
        {
            tstring message = UTF8ToTString(_("Your system doesn't support OpenGL 2.1!"));
            tstring error = UTF8ToTString(_("Fatal Error"));
            MessageBox(NULL, message.c_str(), error.c_str(), MB_OK | MB_ICONERROR);
            return false;
        }
    }

    return true;
}

void
MainWindow::destroyDeviceContext()
{
    if (deviceContext != nullptr)
    {
        if (!ReleaseDC(hWnd, deviceContext))
        {
            tstring message = UTF8ToTString(_("Releasing device context failed."));
            tstring error = UTF8ToTString(_("Error"));
            MessageBox(NULL, message.c_str(), error.c_str(), MB_OK | MB_ICONERROR);
        }
        deviceContext = nullptr;
    }

    hWnd = nullptr;
}

bool
MainWindow::isDialogMessage(MSG* msg) const
{
    return (starBrowser != nullptr && IsDialogMessage(starBrowser->hwnd, msg)) ||
           (solarSystemBrowser != nullptr && IsDialogMessage(solarSystemBrowser->hwnd, msg)) ||
           (tourGuide != nullptr && IsDialogMessage(tourGuide->hwnd, msg)) ||
           (gotoObjectDlg != nullptr && IsDialogMessage(gotoObjectDlg->hwnd, msg)) ||
           (viewOptionsDlg != nullptr && IsDialogMessage(viewOptionsDlg->hwnd, msg)) ||
           (eclipseFinder != nullptr && IsDialogMessage(eclipseFinder->hwnd, msg)) ||
           (locationsDlg != nullptr && IsDialogMessage(locationsDlg->hwnd, msg)) ||
           (displayModeDlg != nullptr && IsDialogMessage(displayModeDlg->hwnd, msg));
}

void
MainWindow::handleJoystick()
{
    if (!useJoystick)
        return;

    JOYINFOEX info;
    info.dwSize = sizeof info;
    info.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS;
    MMRESULT err = joyGetPosEx(JOYSTICKID1, &info);

    if (err != JOYERR_NOERROR)
        return;

    float x = static_cast<float>(info.dwXpos) / 32768.0f - 1.0f;
    float y = static_cast<float>(info.dwYpos) / 32768.0f - 1.0f;

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

bool
MainWindow::applyCurrentPreferences(AppPreferences& prefs) const
{
    WINDOWPLACEMENT placement;

    placement.length = sizeof(placement);
    if (!GetWindowPlacement(hWnd, &placement))
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
    prefs.starsColor = static_cast<int>(appCore->getRenderer()->getStarColorTable());
    prefs.textureResolution = appCore->getRenderer()->getResolution();

    return true;
}

void
MainWindow::commandDynamicMenus(WPARAM wParam, LPARAM lParam)
{
    if (const FavoritesList* favorites = appCore->getFavorites();
        favorites != nullptr &&
        LOWORD(wParam) >= ID_BOOKMARKS_FIRSTBOOKMARK &&
        LOWORD(wParam) - ID_BOOKMARKS_FIRSTBOOKMARK < static_cast<int>(favorites->size()))
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
        case SelectionType::Star:
            appCore->getSimulation()->selectPlanet(LOWORD(wParam) - MENU_CHOOSE_PLANET);
            break;

        case SelectionType::Body:
            if (auto satellites = sel.body()->getSatellites(); satellites != nullptr)
                appCore->getSimulation()->setSelection(Selection(satellites->getBody(LOWORD(wParam) - MENU_CHOOSE_PLANET)));
            break;

        case SelectionType::DeepSky:
            // Current deep sky object/galaxy implementation does
            // not have children to select.
            break;

        case SelectionType::Location:
            break;

        default:
            break;
        }
    }
    else if (LOWORD(wParam) >= MENU_CHOOSE_SURFACE &&
             LOWORD(wParam) < MENU_CHOOSE_SURFACE + 1000)
    {
        // Handle the alternate surface submenu
        const Body* body = appCore->getSimulation()->getSelection().body();
        if (body == nullptr)
            return;

        int index = (int) LOWORD(wParam) - MENU_CHOOSE_SURFACE - 1;
        auto surfNames = GetBodyFeaturesManager()->getAlternateSurfaceNames(body);
        if (!surfNames.has_value())
            return;

        if (index >= 0 && index < static_cast<int>(surfNames->size()))
        {
            auto it = surfNames->begin();
            std::advance(it, index);
            appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(*it);
        }
        else
        {
            appCore->getSimulation()->getActiveObserver()->setDisplayedSurface({});
        }
    }
    else if (LOWORD(wParam) >= ID_FIRST_SCRIPT &&
             LOWORD(wParam) <  ID_FIRST_SCRIPT + scriptMenuItems.size())
    {
        // Handle the script menu
        unsigned int scriptIndex = LOWORD(wParam) - ID_FIRST_SCRIPT;
        appCore->runScript(scriptMenuItems[scriptIndex].filename);
    }
}

void
MainWindow::restoreCursor()
{
    ShowCursor(TRUE);
    cursorVisible = true;
    SetCursorPos(saveCursorPos.x, saveCursorPos.y);
}

bool
MainWindow::copyStateURLToClipboard()
{
    BOOL b;
    b = OpenClipboard(hWnd);
    if (!b)
        return false;

    CelestiaState appState(appCore);
    appState.captureState();

    Url url(appState);

    fmt::basic_memory_buffer<wchar_t> urlBuffer;
    AppendUTF8ToWide(url.getAsString(), urlBuffer);
    urlBuffer.push_back(L'\0');
    SIZE_T sizeBytes = urlBuffer.size() * sizeof(wchar_t);
    HGLOBAL clipboardDataHandle = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeBytes);

    auto clipboardData = GlobalLock(clipboardDataHandle);
    if (clipboardData == nullptr)
    {
        CloseClipboard();
        return false;
    }

    std::memcpy(clipboardData, urlBuffer.data(), sizeBytes);

    GlobalUnlock(clipboardDataHandle);
    EmptyClipboard();
    HANDLE h = SetClipboardData(CF_UNICODETEXT, clipboardDataHandle);
    CloseClipboard();
    return h != nullptr;
}

void
MainWindow::handleSelectPrimary() const
{
    Selection sel = appCore->getSimulation()->getSelection();
    if (sel.body() != nullptr)
        appCore->getSimulation()->setSelection(Helper::getPrimary(sel.body()));
}

void
MainWindow::showWWWInfo() const
{
    Selection sel = appCore->getSimulation()->getSelection();
    std::string url;
    switch (sel.getType())
    {
    case SelectionType::Body:
        url = sel.body()->getInfoURL();
        break;

    case SelectionType::Star:
        url = sel.star()->getInfoURL();
        if (url.empty())
        {
            // TODO: get rid of fixed URLs
            constexpr std::string_view simbadUrl = "http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident="sv;

            AstroCatalog::IndexNumber number = sel.star()->getIndex();
            if (number <= StarDatabase::MAX_HIPPARCOS_NUMBER)
            {
                url = fmt::format("{}HIP+{}", simbadUrl, number);
            }
            else if (number <= Star::MaxTychoCatalogNumber)
            {
                AstroCatalog::IndexNumber tyc3 = number / UINT32_C(1000000000);
                number -= tyc3 * UINT32_C(1000000000);
                AstroCatalog::IndexNumber tyc2 = number / UINT32_C(10000);
                number -= tyc2 * UINT32_C(10000);
                AstroCatalog::IndexNumber tyc1 = number;
                url = fmt::format("{}TYC+{}-{}-{}", simbadUrl, tyc1, tyc2, tyc3);
            }
        }
        break;

    case SelectionType::DeepSky:
        url = sel.deepsky()->getInfoURL();
        break;

    case SelectionType::Location:
        break;

    default:
        break;
    }

    if (url.empty())
        return;

#ifdef _UNICODE
    fmt::basic_memory_buffer<wchar_t> wbuffer;
    AppendUTF8ToWide(url, wbuffer);
    wbuffer.push_back(L'\0');
    ShellExecute(hWnd, TEXT("open"), wbuffer.data(), nullptr, nullptr, 0);
#else
    ShellExecute(hWnd, TEXT("open"), url.c_str(), nullptr, nullptr, 0);
#endif
}

void
MainWindow::handleRunDemo() const
{
    const auto& demoScriptFile = appCore->getConfig()->paths.demoScriptFile;
    if (demoScriptFile.empty())
        return;

    appCore->cancelScript();
    appCore->runScript(demoScriptFile);
}

LRESULT CALLBACK
MainWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* mainWindow;
    if (uMsg == WM_CREATE)
    {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        mainWindow = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        return mainWindow->create(hWnd);
    }

    mainWindow = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (!mainWindow || !mainWindow->checkHWnd(hWnd))
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    switch(uMsg)
    {
    case WM_DROPFILES:
        break;

    case WM_MEASUREITEM:
        return mainWindow->measureItem(lParam);

    case WM_DRAWITEM:
        return mainWindow->drawItem(lParam);

    case WM_MOUSEMOVE:
        mainWindow->mouseMove(wParam, lParam);
        break;

    case WM_LBUTTONDOWN:
        mainWindow->buttonDown(lParam, CelestiaCore::LeftButton);
        break;

    case WM_RBUTTONDOWN:
        mainWindow->buttonDown(lParam, CelestiaCore::RightButton);
        break;

    case WM_MBUTTONDOWN:
        mainWindow->buttonDown(lParam, CelestiaCore::MiddleButton);
        break;

    case WM_LBUTTONUP:
        mainWindow->buttonUp(lParam, CelestiaCore::LeftButton);
        break;

    case WM_RBUTTONUP:
        mainWindow->buttonUp(lParam, CelestiaCore::RightButton);
        break;

    case WM_MBUTTONUP:
        mainWindow->buttonUp(lParam, CelestiaCore::MiddleButton);
        break;

    case WM_MOUSEWHEEL:
        mainWindow->mouseWheel(wParam);
        break;

    case WM_KEYDOWN:
        mainWindow->keyDown(wParam);
        break;

    case WM_KEYUP:
        mainWindow->handleKey(wParam, false);
        break;

    case WM_CHAR:
        mainWindow->processChar(wParam, lParam);
        break;

    case WM_IME_CHAR:
        mainWindow->imeChar(wParam);
        break;

    case WM_COPYDATA:
        mainWindow->copyData(lParam);
        break;

    case WM_COMMAND:
        mainWindow->command(wParam, lParam);
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        mainWindow->resize(lParam);
        break;

    case WM_PAINT:
        mainWindow->paint();
        break;

    default:
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}

ATOM
RegisterMainWindowClass(HINSTANCE appInstance, HCURSOR hDefaultCursor)
{
    // Set up and register the window class
    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = &MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = appInstance;
    wc.hIcon = LoadIcon(appInstance, MAKEINTRESOURCE(IDI_CELESTIA_ICON));
    wc.hCursor = hDefaultCursor;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = AppName;
    ATOM result = RegisterClass(&wc);
    if (result == 0)
    {
        tstring message = UTF8ToTString(_("Failed to register the window class."));
        tstring fatalError = UTF8ToTString(_("Fatal Error"));
        MessageBox(NULL, message.c_str(), fatalError.c_str(), MB_OK | MB_ICONERROR);
    }

    return result;
}

void
SyncMenusWithRendererState(CelestiaCore* appCore, HMENU menuBar)
{
    std::uint64_t renderFlags = appCore->getRenderer()->getRenderFlags();
    float ambientLight = appCore->getRenderer()->getAmbientLightLevel();
    unsigned int textureRes = appCore->getRenderer()->getResolution();

    setMenuItemCheck(menuBar, ID_VIEW_SHOW_FRAMES,
                     appCore->getFramesVisible());
    setMenuItemCheck(menuBar, ID_VIEW_SYNC_TIME,
                     appCore->getSimulation()->getSyncTime());

    if (std::abs(ambientLight) < 1.0e-3f)
    {
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_CHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
    }
    else if (std::abs(0.1f - ambientLight) < 1.0e-3f)
    {
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_CHECKED);
        CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
    }
    else if (std::abs(0.25f - ambientLight) < 1.0e-3f)
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

    auto colorType = appCore->getRenderer()->getStarColorTable();
    CheckMenuItem(menuBar, ID_STARCOLOR_CLASSIC, colorType == ColorTableType::Enhanced ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_STARCOLOR_D65,  colorType == ColorTableType::Blackbody_D65 ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_STARCOLOR_SOLAR,  colorType == ColorTableType::SunWhite ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_STARCOLOR_VEGA,  colorType == ColorTableType::VegaWhite ? MF_CHECKED : MF_UNCHECKED);

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
        ((renderFlags & Renderer::ShowSmoothLines) != 0)? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menuBar, ID_RENDER_AUTOMAG,
        ((renderFlags & Renderer::ShowAutoMag) != 0) ? MF_CHECKED : MF_UNCHECKED);
}

void
ShowUniversalTime(CelestiaCore* appCore)
{
    appCore->setTimeZoneBias(0);
    appCore->setTimeZoneName("UTC");
}

void
ShowLocalTime(CelestiaCore* appCore)
{
    std::string tzName;
    int dstBias;
    if (GetTZInfo(tzName, dstBias))
    {
        appCore->setTimeZoneName(tzName);
        appCore->setTimeZoneBias(dstBias);
    }
}

} // end namespace celestia::win32
