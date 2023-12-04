// winfiledlgs.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Standard open/save dialogs.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#include <winfiledlgs.h>

#include <array>
#include <string>

#include <celcompat/filesystem.h>
#include <celestia/celestiacore.h>
#include <celutil/filetype.h>
#include <celutil/gettext.h>

#include <commdlg.h>

#include "tstring.h"

using namespace std::string_view_literals;

namespace celestia::win32
{

// Always use the wide character versions of OPENFILENAME
// as this matches the file system

void
HandleCaptureImage(HWND hWnd, CelestiaCore* appCore)
{
    // Display File SaveAs dialog to allow user to specify name and location of
    // of captured screen image.
    std::array<wchar_t, _MAX_PATH + 1> szFile;
    szFile.fill(L'\0');

    std::wstring filter;
    AppendUTF8ToWide(_("JPEG - JFIF Compliant (*.jpg)"), filter);
    filter.append(L"\0*.jpg;*.jif;*.jpeg\0"sv);
    AppendUTF8ToWide(_("Portable Network Graphics (*.png)"), filter);
    filter.append(L"\0*.png\0\0"sv);

    // Initialize OPENFILENAME
    OPENFILENAMEW Ofn;
    ZeroMemory(&Ofn, sizeof(OPENFILENAMEW));
    Ofn.lStructSize = static_cast<DWORD>(sizeof(OPENFILENAMEW));
    Ofn.hwndOwner = hWnd;
    Ofn.lpstrFilter = filter.data();
    Ofn.lpstrFile = szFile.data();
    Ofn.nMaxFile = static_cast<DWORD>(szFile.size());
    Ofn.lpstrInitialDir = nullptr;

    // Comment this out if you just want the standard "Save As" caption.
    std::wstring caption;
    AppendUTF8ToWide(_("Save As - Specify File to Capture Image"), caption);
    Ofn.lpstrTitle = caption.c_str();

    // OFN_HIDEREADONLY - Do not display read-only JPEG or PNG files
    // OFN_OVERWRITEPROMPT - If user selected a file, prompt for overwrite confirmation.
    Ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    // Display the Save dialog box.
    if (!GetSaveFileNameW(&Ofn))
        return;

    // If you got here, a path and file has been specified.
    // Ofn.lpstrFile contains full path to specified file
    fs::path filename(Ofn.lpstrFile);
    constexpr std::array<std::wstring_view, 2> defaultExtensions
    {
        L".jpg"sv,
        L".png"sv,
    };

    if (auto extension = filename.extension();
        (extension.empty() || extension == L"."sv) &&
        Ofn.nFilterIndex > 0 && Ofn.nFilterIndex <= defaultExtensions.size())
    {
        // If no extension was specified or extension was just a period,
        // use the selection of filter to determine which type of file
        // should be created.
        filename.replace_extension(defaultExtensions[Ofn.nFilterIndex - 1]);
    }

    ContentType type = DetermineFileType(filename);
    if (type != ContentType::JPEG && type != ContentType::PNG)
    {
        tstring errorMessage = UTF8ToTString(_("Please use a name ending in '.jpg' or '.png'."));
        tstring error = UTF8ToTString(_("Error"));
        MessageBox(hWnd, errorMessage.c_str(), error.c_str(), MB_OK | MB_ICONERROR);
        return;
    }

    // Redraw to make sure that the back buffer is up to date
    appCore->draw();
    if (!appCore->saveScreenShot(filename))
    {
        tstring errorMessage = UTF8ToTString(_("Could not save image file."));
        tstring error = UTF8ToTString(_("Error"));
        MessageBox(hWnd, errorMessage.c_str(), error.c_str(), MB_OK | MB_ICONERROR);
    }
}

void
HandleOpenScript(HWND hWnd, CelestiaCore* appCore)
{
    // Display File Open dialog to allow user to specify name and location of
    // of captured screen image.
    std::array<wchar_t, _MAX_PATH + 1> szFile;
    szFile.fill(L'\0');

    std::wstring filter;
    AppendUTF8ToWide(_("Celestia Script (*.celx, *.cel)"), filter);
#ifdef CELX
    filter.append(L"\0*.celx;*.cel\0\0"sv);
#else
    filter.append(L"\0*.cel\0\0"sv);
#endif

    // Initialize OPENFILENAME
    OPENFILENAMEW Ofn;
    ZeroMemory(&Ofn, sizeof(OPENFILENAMEW));
    Ofn.lStructSize = static_cast<DWORD>(sizeof(OPENFILENAMEW));
    Ofn.hwndOwner = hWnd;
    Ofn.lpstrFilter = filter.data();
    Ofn.lpstrFile = szFile.data();
    Ofn.nMaxFile = static_cast<DWORD>(szFile.size());
    Ofn.lpstrFileTitle = nullptr;
    Ofn.nMaxFileTitle = 0;
    Ofn.lpstrInitialDir = nullptr;

    // Comment this out if you just want the standard "Save As" caption.
    // Ofn.lpstrTitle = "Save As - Specify File to Capture Image";

    // Display the Open dialog box.
    if (!GetOpenFileNameW(&Ofn))
        return;

    // If you got here, a path and file has been specified.
    // Ofn.lpstrFile contains full path to specified file
    appCore->cancelScript();
    appCore->runScript(Ofn.lpstrFile);
}

} // end namespace celestia::win32
