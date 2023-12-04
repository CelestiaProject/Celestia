// winmainwindow.h
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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <celutil/array_view.h>

#include <windows.h>

class CelestiaCore;
struct ScriptMenuItem;

namespace celestia::win32
{

struct AppPreferences;
class DisplayModeDialog;
class EclipseFinderDialog;
class GotoObjectDialog;
class LocationsDialog;
class ODMenu;
class SolarSystemBrowser;
class StarBrowser;
class TourGuide;
class ViewOptionsDialog;

constexpr inline TCHAR AppName[] = TEXT("Celestia");

class MainWindow
{
public:
    MainWindow(HINSTANCE, HMODULE, HMENU, ODMenu*,
               CelestiaCore*, int, util::array_view<DEVMODE>);
    ~MainWindow();
    bool checkHWnd(HWND other) const { return other == hWnd; }
    LRESULT create(HWND);
    LRESULT measureItem(LPARAM) const;
    LRESULT drawItem(LPARAM) const;
    void mouseMove(WPARAM, LPARAM);
    void buttonDown(LPARAM, int);
    void buttonUp(LPARAM, int);
    void mouseWheel(WPARAM) const;
    void keyDown(WPARAM);
    void handleKey(WPARAM, bool);
    void processChar(WPARAM, LPARAM) const;
    void imeChar(WPARAM) const;
    void copyData(LPARAM) const;
    void command(WPARAM, LPARAM);
    void resize(LPARAM) const;
    void paint() const;

    bool setDeviceContext(util::array_view<std::string> ignoreGLExtensions = {});
    void destroyDeviceContext();
    void setRenderState(bool ready) { bReady = ready; }
    void ignoreNextMove() { ignoreNextMoveEvent = true; }
    bool fullScreen() const { return isFullScreen; }
    void setFullScreen(bool value) { isFullScreen = value; }
    int fullScreenMode() const { return lastFullScreenMode; }
    void setFullScreenMode(int mode) { lastFullScreenMode = mode; }
    int currentMode() const { return isFullScreen ? lastFullScreenMode : 0; }
    void hideMenuBar() { menuBarHidden = true; }
    bool isDialogMessage(MSG*) const;
    void handleJoystick();
    bool applyCurrentPreferences(AppPreferences&) const;

private:
    void commandDynamicMenus(WPARAM, LPARAM);
    void restoreCursor();
    bool copyStateURLToClipboard();
    void handleSelectPrimary() const;
    void showWWWInfo() const;
    void handleRunDemo() const;

    HWND hWnd{ nullptr };
    HDC deviceContext{ nullptr };
    HGLRC glContext{ nullptr };

    HINSTANCE appInstance;
    HMODULE hRes;
    HMENU menuBar;
    ODMenu* odAppMenu;
    CelestiaCore* appCore;
    // The mode used when isFullScreen is true; saved and restored from the registry
    int lastFullScreenMode{ 0 };
    util::array_view<DEVMODE> displayModes;

    POINT saveCursorPos{ 0, 0 };
    POINT lastMouseMove{ 0, 0 };
    int lastX{ 0 };
    int lastY{ 0 };
    bool ignoreNextMoveEvent{ false };
    bool cursorVisible{ true };

    bool menuBarHidden{ false };

    // Display mode indices
    bool bReady{ false };
    bool isFullScreen{ false };

    bool joystickAvailable{ false };
    bool useJoystick{ false };

    std::unique_ptr<GotoObjectDialog> gotoObjectDlg;
    std::unique_ptr<TourGuide> tourGuide;
    std::unique_ptr<StarBrowser> starBrowser;
    std::unique_ptr<SolarSystemBrowser> solarSystemBrowser;
    std::unique_ptr<ViewOptionsDialog> viewOptionsDlg;
    std::unique_ptr<EclipseFinderDialog> eclipseFinder;
    std::unique_ptr<LocationsDialog> locationsDlg;
    std::unique_ptr<DisplayModeDialog> displayModeDlg;

    std::vector<ScriptMenuItem> scriptMenuItems;
};

ATOM RegisterMainWindowClass(HINSTANCE appInstance, HCURSOR hDefaultCursor);
void SyncMenusWithRendererState(CelestiaCore* appCore, HMENU menuBar);

void ShowUniversalTime(CelestiaCore* appCore);
void ShowLocalTime(CelestiaCore* appCore);

}
