// winmain.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Original version:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Windows front end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <epoxy/wgl.h>

#include <fmt/format.h>
#include <fmt/xchar.h>

#include <celastro/date.h>
#include <celcompat/filesystem.h>
#include <celengine/render.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celestia/configfile.h>
#include <celestia/progressnotifier.h>
#include <celestia/scriptmenu.h>
#include <celutil/array_view.h>
#include <celutil/fsutils.h>
#include <celutil/gettext.h>
#include <celutil/localeutil.h>
#include <celutil/logger.h>
#include <celutil/winutil.h>

#include <windows.h>
#include <ole2.h>
#include <shellapi.h>

#include "res/resource.h"
#include "odmenu.h"
#include "tstring.h"
#include "winbookmarks.h"
#include "wincontextmenu.h"
#include "windatepicker.h"
#include "windroptarget.h"
#include "winmainwindow.h"
#include "winpreferences.h"
#include "winsplash.h"
#include "winuiutils.h"

using namespace std::string_view_literals;
using celestia::util::GetLogger;

namespace celestia::win32
{

namespace
{

constexpr wchar_t ScriptsDirectory[] = L"scripts";

void
RestoreDisplayMode()
{
    ChangeDisplaySettings(0, 0);
}

// Cursor handler callback class for Windows.  We pass an instance to
// the app core which the calls the setCursorShape method to change
// the cursor icon.
class WinCursorHandler : public CelestiaCore::CursorHandler
{
public:
    explicit WinCursorHandler(HCURSOR _defaultCursor);
    ~WinCursorHandler() override = default;
    void setCursorShape(CelestiaCore::CursorShape) override;
    CelestiaCore::CursorShape getCursorShape() const override;

private:
    CelestiaCore::CursorShape shape{ CelestiaCore::ArrowCursor };
    HCURSOR defaultCursor;
    HCURSOR sizeVertical{ LoadCursor(nullptr, IDC_SIZENS) };
    HCURSOR sizeHorizontal{ LoadCursor(nullptr, IDC_SIZEWE) };
};

WinCursorHandler::WinCursorHandler(HCURSOR _defaultCursor) :
    defaultCursor(_defaultCursor)
{
}

void
WinCursorHandler::setCursorShape(CelestiaCore::CursorShape _shape)
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

CelestiaCore::CursorShape
WinCursorHandler::getCursorShape() const
{
    return shape;
}

// end WinCursorHandler methods

HMENU
CreateMenuBar(HMODULE hRes)
{
    return LoadMenu(hRes, MAKEINTRESOURCE(IDR_MAIN_MENU));
}

bool
EnableFullScreen(const DEVMODE& dm)
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
        tstring message = UTF8ToTString(_("Unable to switch to full screen mode; running in window mode"));
        tstring error = UTF8ToTString(_("Error"));
        MessageBox(NULL, message.c_str(), error.c_str(), MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

void
DisableFullScreen()
{
    ChangeDisplaySettings(0, 0);
}

HWND
CreateOpenGLWindow(HINSTANCE appInstance, HMENU menuBar, HCURSOR hDefaultCursor,
                   int x, int y, int width, int height,
                   MainWindow* mainWindow,
                   util::array_view<DEVMODE> displayModes,
                   util::array_view<std::string> ignoreGLExtensions = {})
{
    int newMode = mainWindow->currentMode();
    if (newMode < 0 || newMode > displayModes.size())
    {
        newMode = 0;
    }
    if (newMode != 0)
    {
        x = 0;
        y = 0;
        width = displayModes[newMode - 1].dmPelsWidth;
        height = displayModes[newMode - 1].dmPelsHeight;
    }

    if (RegisterMainWindowClass(appInstance, hDefaultCursor) == 0)
        return nullptr;

    if (newMode != 0)
    {
        if (!EnableFullScreen(displayModes[newMode - 1]))
            newMode = 0;
    }
    else
    {
        DisableFullScreen();
        newMode = 0;
    }

    // Determine the proper window style to use
    DWORD dwStyle = newMode != 0
        ? (WS_POPUPWINDOW | WS_MAXIMIZE)
        : (WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

    // Create the window
    HWND hwnd = CreateWindow(AppName, AppName,
                             dwStyle,
                             x, y, width, height,
                             nullptr, nullptr,
                             appInstance,
                             mainWindow);

    if (hwnd == nullptr)
        return nullptr;

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    if (newMode == 0)
    {
        mainWindow->setFullScreen(false);
        SetMenu(hwnd, menuBar);
    }
    else
    {
        mainWindow->setFullScreen(true);
        mainWindow->setFullScreenMode(newMode);
        mainWindow->hideMenuBar();
    }

    mainWindow->setDeviceContext(ignoreGLExtensions);

    return hwnd;
}

void
DestroyOpenGLWindow(HINSTANCE appInstance, HWND hWnd, MainWindow* mainWindow)
{
    mainWindow->destroyDeviceContext();

    if (hWnd != nullptr)
    {
        SetMenu(hWnd, nullptr);
        DestroyWindow(hWnd);
    }

    UnregisterClass(AppName, appInstance);
}

void
DisableDemoMenu(HMENU menuBar)
{
    HMENU fileMenu = GetSubMenu(menuBar, 0);
    DeleteMenu(fileMenu, ID_FILE_RUNDEMO, MF_BYCOMMAND);
}

void
BuildScriptsMenu(HMENU menuBar, const fs::path& scriptsDir)
{
    HMENU fileMenu = GetSubMenu(menuBar, 0);

    std::vector<ScriptMenuItem> scriptMenuItems = ScanScriptsDirectory(scriptsDir, false);
    if (scriptMenuItems.empty())
    {
        EnableMenuItem(fileMenu, ID_FILE_SCRIPTS, MF_GRAYED);
        return;
    }

    MENUITEMINFO info;
    ZeroMemory(&info, sizeof(MENUITEMINFO));
    info.cbSize = sizeof(info);
    info.fMask = MIIM_SUBMENU;

    if (!GetMenuItemInfo(fileMenu, 1, TRUE, &info))
        return;

    HMENU scriptMenu = info.hSubMenu;

    // Remove the old menu items
    for (int count = GetMenuItemCount(scriptMenu); count > 0; count--)
        DeleteMenu(scriptMenu, 0, MF_BYPOSITION);

    fmt::basic_memory_buffer<TCHAR> buf;
    for (unsigned int i = 0; i < scriptMenuItems.size(); ++i)
    {
        buf.clear();
        AppendUTF8ToTChar(scriptMenuItems[i].title, buf);
        buf.push_back(TEXT('\0'));
        AppendMenu(scriptMenu, MF_STRING, ID_FIRST_SCRIPT + i, buf.data());
    }
}

class WinAlerter : public CelestiaCore::Alerter
{
public:
    explicit WinAlerter(const std::shared_ptr<SplashWindow>& _splash) : splash(_splash) {};
    ~WinAlerter() {};

    void fatalError(const std::string& msg) override
    {
        if (auto splashLock = splash.lock(); splashLock != nullptr)
            splashLock->close();

        tstring tmsg = UTF8ToTString(msg);
        tstring error = UTF8ToTString(_("Fatal Error"));
        MessageBox(NULL, tmsg.c_str(), error.c_str(), MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }

private:
    std::weak_ptr<SplashWindow> splash;
};

std::vector<DEVMODE>
EnumerateDisplayModes(unsigned int minBPP,
                      int& lastFullScreenMode,
                      int& fallbackFullScreenMode)
{
    lastFullScreenMode = 0;
    std::vector<DEVMODE> modes;

    DEVMODE dm;
    int i = 0;
    while (EnumDisplaySettings(NULL, i, &dm))
    {
        if (dm.dmBitsPerPel >= minBPP)
            modes.push_back(dm);
        ++i;
    }
    std::sort(modes.begin(), modes.end(),
              [](const auto& dm1, const auto& dm2)
              {
                  return std::tie(dm1.dmBitsPerPel, dm1.dmPelsWidth, dm1.dmPelsHeight, dm1.dmDisplayFrequency) <
                         std::tie(dm2.dmBitsPerPel, dm2.dmPelsWidth, dm2.dmPelsHeight, dm2.dmDisplayFrequency);
              });

    // Bail out early if EnumDisplaySettings fails for some messed up reason
    if (modes.empty())
        return modes;

    // Go through the sorted list and eliminate modes that differ only
    // by refresh rates.
    modes.erase(std::unique(modes.begin(), modes.end(),
                            [](const auto& dm1, const auto& dm2)
                            {
                                return std::tie(dm1.dmPelsWidth, dm1.dmPelsHeight, dm1.dmBitsPerPel) ==
                                       std::tie(dm2.dmPelsWidth, dm2.dmPelsHeight, dm2.dmBitsPerPel);
                            }),
                modes.end());

    // Select the fallback display mode--choose 640x480 at the current
    // pixel depth.  If for some bizarre reason that's not available,
    // fall back to the first mode in the list.
    fallbackFullScreenMode = 0;
    if (auto iter = std::find_if(modes.begin(), modes.end(),
                                 [](const auto& mode)
                                 {
                                     return mode.dmPelsWidth == 640 && mode.dmPelsHeight == 480;
                                 });
        iter != modes.end())
    {
        // Add one to the mode index, since mode 0 means windowed mode
        fallbackFullScreenMode = static_cast<int>(iter - modes.begin()) + 1;
    }

    if (fallbackFullScreenMode == 0 && !modes.empty())
        fallbackFullScreenMode = 1;
    lastFullScreenMode = fallbackFullScreenMode;

    return modes;
}

struct StartupOptions
{
    util::Level verbosity{ util::Level::Info };
    bool startFullscreen{ false };
    bool runOnce{ false };
    std::string startURL; // UTF-8
    fs::path startDirectory;
    fs::path startScript;
    std::vector<fs::path> extrasDirectories;
    fs::path configFileName;
    bool useAlternateConfigFile{ false };
    bool skipSplashScreen{ false };
};

bool
parseCommandLine(int argc, LPWSTR* argv, StartupOptions& options)
{
    int i = 1; // Skip the program name
    while (i < argc)
    {
        bool isLastArg = (i == argc - 1);
        std::wstring_view arg = argv[i];
        if (arg == L"--verbose"sv)
        {
            options.verbosity = util::Level::Verbose;
        }
        else if (arg == L"--debug"sv)
        {
            options.verbosity = util::Level::Debug;
        }
        else if (arg == L"--fullscreen"sv)
        {
            options.startFullscreen = true;
        }
        else if (arg == L"--once"sv)
        {
            options.runOnce = true;
        }
        else if (arg == L"--dir"sv)
        {
            if (isLastArg)
            {
                tstring message = UTF8ToTString(_("Directory expected after --dir"));
                tstring caption = UTF8ToTString(_("Celestia Command Line Error"));
                MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
                return false;
            }
            ++i;
            options.startDirectory = argv[i];
        }
        else if (arg == L"--conf"sv)
        {
            if (isLastArg)
            {
                tstring message = UTF8ToTString(_("Configuration file name expected after --conf"));
                tstring caption = UTF8ToTString(_("Celestia Command Line Error"));
                MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
                return false;
            }
            ++i;
            options.configFileName = argv[i];
            options.useAlternateConfigFile = true;
        }
        else if (arg == L"--extrasdir"sv)
        {
            if (isLastArg)
            {
                tstring message = UTF8ToTString(_("Directory expected after --extrasdir"));
                tstring caption = UTF8ToTString(_("Celestia Command Line Error"));
                MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
                return false;
            }
            ++i;
            options.extrasDirectories.emplace_back(argv[i]);
        }
        else if (arg == L"-u"sv || arg == L"--url"sv)
        {
            if (isLastArg)
            {
                tstring message = UTF8ToTString(_("URL expected after --url"));
                tstring caption = UTF8ToTString(_("Celestia Command Line Error"));
                MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
                return false;
            }
            ++i;
            options.startURL = util::WideToUTF8(argv[i]);
        }
        else if (arg == L"-s"sv || arg == L"--nosplash"sv)
        {
            options.skipSplashScreen = true;
        }
        else
        {
            tstring msgTemplate = UTF8ToTString(_("Invalid command line option '{}'"));
#ifdef _UNICODE
            std::wstring message = fmt::format(fmt::runtime(msgTemplate), arg);
#else
            auto tArg = WideToCurrentCP(arg);
            std::string message = fmt::format(fmt::runtime(msgTemplate), tArg);
#endif
            tstring caption = UTF8ToTString(_("Celestia Command Line Error"));
            MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
            return false;
        }

        ++i;
    }

    return true;
}

class WinSplashProgressNotifier : public ProgressNotifier
{
public:
    explicit WinSplashProgressNotifier(const std::shared_ptr<SplashWindow>& _splash) : splash(_splash) {};
    virtual ~WinSplashProgressNotifier() {};

    void update(const std::string& filename) override
    {
        if (auto splashLock = splash.lock(); splashLock != nullptr)
            splashLock->setMessage(fmt::format(_("Loading: {}"), filename));
    }

private:
    std::weak_ptr<SplashWindow> splash;
};

} // end unnamed namespace

} // end namespace celestia::win32

int APIENTRY
#ifdef _UNICODE
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
         LPWSTR /* lpCmdLine */, int /* nCmdShow */)
#else
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR /* lpCmdLine */, int /* nCmdShow */)
#endif
{
    using namespace celestia::win32;
    namespace astro = celestia::astro;
    namespace util = celestia::util;

    // Use the wchar_t-based command line to avoid needing to figure out what
    // code page file paths are in.
    LPWSTR cmdLine = GetCommandLineW();
    int argc;
    LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);

    StartupOptions options;
    if (!parseCommandLine(argc, argv, options))
        return 1;

    // If Celestia was invoked with the --once command line parameter,
    // check to see if there's already an instance of Celestia running.
    if (options.runOnce)
    {
        // The FindWindow method of checking for another running instance
        // is broken, and we should be using CreateMutex instead.  But we'll
        // sort that out later . . .
        if (HWND existingWnd = FindWindow(AppName, AppName); existingWnd)
        {
            // If there's an existing instance and we've been given a
            // URL on the command line, send the URL to the running instance
            // of Celestia before terminating.
            if (!options.startURL.empty())
            {
                COPYDATASTRUCT cd;
                cd.dwData = 0;
                cd.cbData = options.startURL.size();
                cd.lpData = options.startURL.data();
                SendMessage(existingWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cd));
            }
            SetForegroundWindow(existingWnd);
            Sleep(1000);
            exit(0);
        }
    }

    CreateLogger(options.verbosity);

    std::shared_ptr<SplashWindow> splash;
    if (!options.skipSplashScreen)
    {
        // by default look in the current (installation) directory
        fs::path splashFile(L"splash\\splash.png");
        if (!options.startDirectory.empty())
        {
            fs::path newSplashFile = fs::path(options.startDirectory) / splashFile;
            if (fs::exists(newSplashFile))
                splashFile = std::move(newSplashFile);
        }
        splash = std::make_shared<SplashWindow>(splashFile);
        splash->setMessage(_("Loading data files..."));
        splash->showSplash();
    }

    // If a start directory was given on the command line, switch to it
    // now.
    if (!options.startDirectory.empty())
        SetCurrentDirectoryW(options.startDirectory.c_str());

    OleInitialize(NULL);

    // Specify some default values in case registry keys are not found. Ideally, these
    // defaults should never be used--they should be overridden by settings in
    // celestia.cfg.
    AppPreferences prefs;
    LoadPreferencesFromRegistry(prefs);

    // Adjust window dimensions for screen dimensions
    int screenWidth = GetSystemMetricsForWindow(SM_CXSCREEN, nullptr);
    int screenHeight = GetSystemMetricsForWindow(SM_CYSCREEN, nullptr);
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
    RECT windowRect;
    windowRect.left = prefs.winX;
    windowRect.top = prefs.winY;
    windowRect.right = windowRect.left + prefs.winWidth;
    windowRect.bottom = windowRect.top + prefs.winHeight;

    int fullScreenMode = 0;
    int fallbackFullScreenMode = 0;
    std::vector<DEVMODE> displayModes = EnumerateDisplayModes(16, fullScreenMode, fallbackFullScreenMode);

    // If full screen mode was found in registry, override default with it.
    if (prefs.fullScreenMode != -1)
        fullScreenMode = prefs.fullScreenMode;

    // If the user changes the computer's graphics card or driver, the
    // number of display modes may change. Since we're storing a display
    // index this can cause troubles. Avoid out of range errors by
    // checking against the size of the mode list, falling back to a
    // safe mode if the last used mode index is now out of range.
    // TODO: A MUCH better fix for the problem of changing GPU/driver
    // is to store the mode parameters instead of just the mode index.
    if (fullScreenMode > displayModes.size())
    {
        fullScreenMode = fallbackFullScreenMode;
    }

    auto appCore = std::make_unique<CelestiaCore>();

    // Gettext integration
    CelestiaCore::initLocale();

#ifdef ENABLE_NLS
    std::error_code ec;
    fs::path localedir = (fs::current_path(ec) / LOCALEDIR);

    wbindtextdomain("celestia", localedir.c_str());
    bind_textdomain_codeset("celestia", "UTF-8");
    wbindtextdomain("celestia-data", localedir.c_str());
    bind_textdomain_codeset("celestia-data", "UTF-8");
    textdomain("celestia");

    const char *msgid = N_("LANGUAGE");
    const char *lang = _(msgid);

    // Loading localized resources
    HMODULE hRes = hInstance;
    if (msgid != lang) // gettext(s) returns either pointer to the translation or `s`
    {
        fmt::basic_memory_buffer<wchar_t, 16> langBuffer;
        AppendUTF8ToWide(lang, langBuffer);

        fs::path resPath = fmt::format(L"locale\\res_{}.dll",
                                       std::wstring_view(langBuffer.data(), langBuffer.size()));
        int langID = 0;

        hRes = LoadLibraryExW(resPath.c_str(), nullptr,
                              LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE
                                  | LOAD_LIBRARY_AS_IMAGE_RESOURCE
                                  | LOAD_LIBRARY_SEARCH_APPLICATION_DIR);
        if (hRes == nullptr)
        {
            GetLogger()->error(_("Could not load localized resources: {}\n"), resPath);
            hRes = hInstance;
        }
        else
        {
            GetLogger()->info(_("Loaded localized resource: {}\n"), resPath);
        }
    }
#endif

    auto alerter = std::make_unique<WinAlerter>(splash);
    appCore->setAlerter(alerter.get());

    std::unique_ptr<WinSplashProgressNotifier> progressNotifier = nullptr;
    if (!options.skipSplashScreen)
        progressNotifier = std::make_unique<WinSplashProgressNotifier>(splash);

    bool initSucceeded = appCore->initSimulation(options.configFileName, options.extrasDirectories, progressNotifier.get());

    progressNotifier.reset();

    // Close the splash screen after all data has been loaded
    if (splash != nullptr)
    {
        splash->close();
        splash.reset();
    }

    // Give up now if we failed initialization
    if (!initSucceeded)
        return 1;

    if (!options.startURL.empty())
        appCore->setStartURL(options.startURL);

    HMENU menuBar = CreateMenuBar(hRes);
#ifndef USE_FFMPEG
    EnableMenuItem(menuBar, ID_FILE_CAPTUREMOVIE, MF_GRAYED);
#endif
    HACCEL acceleratorTable = LoadAccelerators(hRes, MAKEINTRESOURCE(IDR_ACCELERATORS));

    if (appCore->getConfig() == NULL)
    {
        tstring message = UTF8ToTString(_("Confugration file missing!"));
        tstring caption = UTF8ToTString(_("Fatal Error"));
        MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
        return 1;
    }
    HCURSOR hDefaultCursor;
    if (!compareIgnoringCase(appCore->getConfig()->mouse.cursor, "arrow"))
        hDefaultCursor = LoadCursor(NULL, IDC_ARROW);
    else if (!compareIgnoringCase(appCore->getConfig()->mouse.cursor, "inverting crosshair"))
        hDefaultCursor = LoadCursor(hRes, MAKEINTRESOURCE(IDC_CROSSHAIR));
    else
        hDefaultCursor = LoadCursor(hRes, MAKEINTRESOURCE(IDC_CROSSHAIR_OPAQUE));

    appCore->getRenderer()->setSolarSystemMaxDistance(appCore->getConfig()->renderDetails.SolarSystemMaxDistance);
    appCore->getRenderer()->setShadowMapSize(appCore->getConfig()->renderDetails.ShadowMapSize);

    auto cursorHandler = std::make_unique<WinCursorHandler>(hDefaultCursor);
    appCore->setCursorHandler(cursorHandler.get());

#ifndef PORTABLE_BUILD
    if (!prefs.ignoreOldFavorites)
    { // move favorites to the new location
        fs::path path;
        if (appCore->getConfig() != nullptr && !appCore->getConfig()->paths.favoritesFile.empty())
            path = appCore->getConfig()->paths.favoritesFile;
        else
            path = L"favorites.cel";

        if (path.is_relative())
            path = util::WriteableDataPath() / path;

        std::error_code ec;
        if (fs::exists(L"favorites.cel", ec)) // old exists
        {
            if (!fs::exists(path)) // new does not
            {
                tstring message = UTF8ToTString(_("Old favorites file detected.\nCopy to the new location?"));
                tstring caption = UTF8ToTString(_("Copy favorites?"));
                int resp = MessageBox(NULL, message.c_str(), caption.c_str(), MB_YESNO);
                if (resp == IDYES)
                {
                    CopyFileW(L"favorites.cel", path.c_str(), true);
                    prefs.ignoreOldFavorites = true;
                }
            }
        }
    }
#endif

    auto odAppMenu = std::make_unique<ODMenu>();

    HWND hWnd;
    auto mainWindow = std::make_unique<MainWindow>(hInstance, hRes, menuBar, odAppMenu.get(),
                                                   appCore.get(), fullScreenMode, displayModes);
    mainWindow->setFullScreenMode(fullScreenMode);
    if (options.startFullscreen)
    {
        mainWindow->setFullScreen(true);
        hWnd = CreateOpenGLWindow(hInstance, menuBar, hDefaultCursor,
                                  0, 0, 800, 600,
                                  mainWindow.get(), displayModes,
                                  appCore->getConfig()->renderDetails.ignoreGLExtensions);

    }
    else
    {
        hWnd = CreateOpenGLWindow(hInstance, menuBar, hDefaultCursor,
                                  prefs.winX, prefs.winY, prefs.winWidth, prefs.winHeight,
                                  mainWindow.get(), displayModes,
                                  appCore->getConfig()->renderDetails.ignoreGLExtensions);
    }

    // Prevent unnecessary destruction and recreation of OpenGLWindow in
    // while() loop below.
    int savedMode = mainWindow->currentMode();

    if (hWnd == NULL)
    {
        tstring message = UTF8ToTString(_("Failed to create the application window."));
        tstring caption = UTF8ToTString(_("Fatal Error"));
        MessageBox(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    auto dropTarget = std::make_unique<CelestiaDropTarget>(appCore.get());
    if (CoLockObjectExternal(dropTarget.get(), TRUE, TRUE) != S_OK)
    {
        GetLogger()->error(_("Error locking drop target\n"));
        dropTarget.reset();
    }

    if (dropTarget)
    {
        HRESULT hr = RegisterDragDrop(hWnd, dropTarget.get());
        if (hr != S_OK)
        {
            GetLogger()->error(_("Failed to register drop target as OLE object.\n"));
        }
    }

    UpdateWindow(hWnd);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    RegisterDatePicker();

    appCore->setScreenDpi(GetDPIForWindow(hWnd));

    if (!appCore->initRenderer())
        return 1;

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

        appCore->getRenderer()->setStarColorTable(static_cast<ColorTableType>(prefs.starsColor));

        if (prefs.showLocalTime == 1)
            ShowLocalTime(appCore.get());
        else
            ShowUniversalTime(appCore.get());
        appCore->setDateFormat(static_cast<astro::Date::Format>(prefs.dateFormat));
        appCore->getSimulation()->getActiveObserver()->setDisplayedSurface(prefs.altSurfaceName);
        appCore->getRenderer()->setResolution(prefs.textureResolution);
    }
    else
    {
        // Set default render flags for a new installation
        appCore->getRenderer()->setRenderFlags(Renderer::DefaultRenderFlags);
    }

    if (appCore->getConfig()->paths.demoScriptFile.empty())
        DisableDemoMenu(menuBar);
    BuildFavoritesMenu(menuBar, appCore.get(), hInstance, odAppMenu.get());
    BuildScriptsMenu(menuBar, ScriptsDirectory);
    SyncMenusWithRendererState(appCore.get(), menuBar);

    WinContextMenuHandler contextMenuHandler(appCore.get(), hWnd, mainWindow.get());
    appCore->setContextMenuHandler(&contextMenuHandler);

    mainWindow->setRenderState(true);

    appCore->start();

    if (!options.startURL.empty())
    {
        COPYDATASTRUCT cd;
        cd.dwData = 0;
        cd.cbData = options.startURL.size();
        cd.lpData = options.startURL.data();
        SendMessage(hWnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cd));
    }

    // Initial tick required before first frame is rendered; this gives
    // start up scripts a chance to run.
    appCore->tick();

    MSG msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
        bool isVisible = !IsIconic(hWnd);

        // If Celestia is in an inactive state, use GetMessage to avoid
        // sucking CPU cycles.  If time is paused and the camera isn't
        // moving in any view, we can probably also avoid constantly
        // rendering.
        BOOL haveMessage = isVisible
            ? PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)
            : GetMessage(&msg, NULL, 0U, 0U);

        if (!haveMessage)
        {
            // Tick the simulation
            appCore->tick();
        }

        if (haveMessage)
        {
            // Translate and dispatch the message
            if (!mainWindow->isDialogMessage(&msg))
            {
                if (!TranslateAccelerator(hWnd, acceleratorTable, &msg))
                    TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            // And force a redraw
            InvalidateRect(hWnd, NULL, FALSE);
        }

        mainWindow->handleJoystick();

        if (int newMode = mainWindow->currentMode(); newMode != savedMode)
        {
            if (savedMode == 0)
                GetWindowRect(hWnd, &windowRect);

            DestroyOpenGLWindow(hInstance, hWnd, mainWindow.get());
            hWnd = CreateOpenGLWindow(hInstance, menuBar, hDefaultCursor,
                                      windowRect.left, windowRect.top,
                                      windowRect.right-windowRect.left,
                                      windowRect.bottom-windowRect.top,
                                      mainWindow.get(), displayModes);
            contextMenuHandler.setHwnd(hWnd);
            UpdateWindow(hWnd);
            savedMode = newMode;
        }
    }

    // Save application preferences
    if (mainWindow->applyCurrentPreferences(prefs))
        SavePreferencesToRegistry(prefs);

    // Not ready to render anymore.
    mainWindow->setRenderState(false);

    // Clean up the window
    if (mainWindow->fullScreen())
        RestoreDisplayMode();
    DestroyOpenGLWindow(hInstance, hWnd, mainWindow.get());

    appCore.reset();

    OleUninitialize();

    return msg.wParam;
}
