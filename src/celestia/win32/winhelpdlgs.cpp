// winhelpdlgs.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Various dialog boxes in the Help menu.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winhelpdlgs.h"

#include <cstddef>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <celcompat/filesystem.h>
#include <celestia/celestiacore.h>
#include <celestia/helper.h>
#include <celutil/fsutils.h>
#include <celutil/gettext.h>

#include <shellapi.h>

#include "res/resource.h"
#include "tstring.h"
#include "winhyperlinks.h"

using namespace std::string_view_literals;

namespace celestia::win32
{

namespace
{

constexpr std::string_view licenseUrl = "https://www.gnu.org/licenses/old-licenses/gpl-2.0.html"sv;

void
AppendConvertEOL(std::string_view src, tstring& dest)
{
    // Replace LF with CRLF and convert to wchar_t/CP_ACP
    for (;;)
    {
        auto pos = src.find('\n');
        if (pos == std::string_view::npos)
        {
            AppendUTF8ToTChar(src, dest);
            return;
        }

        AppendUTF8ToTChar(src.substr(0, pos), dest);
        dest += TEXT("\r\n");
        src = src.substr(pos + 1);
    }
}

bool
LoadItemTextFromFile(const fs::path& filename, tstring& message)
{
    constexpr std::size_t bufferSize = 4096;
    std::ifstream f(filename, std::ios::in | std::ios::binary);
    if (!f.good())
        return false;

    std::vector<char> buffer(bufferSize, '\0');
    while (!f.eof())
    {
        f.read(buffer.data(), buffer.size()); /* Flawfinder: ignore */
        if (!f.good() && !f.eof())
            return false;

        auto charsRead = static_cast<std::size_t>(f.gcount());
        AppendConvertEOL(std::string_view(buffer.data(), charsRead), message);
    }

    return true;
}

INT_PTR APIENTRY
ControlsHelpProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        if (tstring message; LoadItemTextFromFile(util::LocaleFilename("controls.txt"), message))
        {
            SetDlgItemText(hDlg, IDC_TEXT_CONTROLSHELP, message.c_str());
            return TRUE;
        }
        break;

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

INT_PTR APIENTRY
AboutProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        MakeHyperlinkFromStaticCtrl(hDlg, IDC_CELESTIALINK);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;

        case IDC_CELESTIALINK:
            ShellExecute(hDlg, TEXT("open"), CELESTIA_URL, nullptr, nullptr, SW_SHOWNORMAL);
            return TRUE;

        default:
            break;
        }

    default:
        break;
    }

    return FALSE;
}

INT_PTR APIENTRY
LicenseProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            tstring message;
            if (!LoadItemTextFromFile(util::LocaleFilename("COPYING"), message))
            {
                message.clear();
                // Gettext complains about using \r in translated messages, so add it in afterwards
                AppendConvertEOL(fmt::format(_("License file missing!\nSee {}"), licenseUrl), message);
            }
            SetDlgItemText(hDlg, IDC_LICENSE_TEXT, message.c_str());
        }
        return TRUE;

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

INT_PTR APIENTRY
GLInfoProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            auto appCore = reinterpret_cast<const CelestiaCore*>(lParam);
            std::string s = Helper::getRenderInfo(appCore->getRenderer());
            tstring message;
            AppendConvertEOL(s, message);
            SetDlgItemText(hDlg, IDC_GLINFO_TEXT, message.c_str());
        }
        return TRUE;

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

} // end unnamed namespace

void
ShowControlsDialog(HINSTANCE appInstance, HWND appWindow)
{
    CreateDialog(appInstance,
                 MAKEINTRESOURCE(IDD_CONTROLSHELP),
                 appWindow,
                 &ControlsHelpProc);
}

void
ShowAboutDialog(HINSTANCE appInstance, HWND appWindow)
{
    DialogBox(appInstance,
              MAKEINTRESOURCE(IDD_ABOUT),
              appWindow,
              &AboutProc);
}

void
ShowLicenseDialog(HINSTANCE appInstance, HWND appWindow)
{
    DialogBox(appInstance,
              MAKEINTRESOURCE(IDD_LICENSE),
              appWindow,
              &LicenseProc);
}

void
ShowGLInfoDialog(HINSTANCE appInstance, HWND appWindow, const CelestiaCore* appCore)
{
    DialogBoxParam(appInstance,
                   MAKEINTRESOURCE(IDD_GLINFO),
                   appWindow,
                   &GLInfoProc,
                   reinterpret_cast<LPARAM>(appCore));
}

} // end namespace celestia::win32
