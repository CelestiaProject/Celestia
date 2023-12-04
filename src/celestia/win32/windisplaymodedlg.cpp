// windisplaymodedlg.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Windows display mode selection.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "windisplaymodedlg.h"

#include <iterator>
#include <optional>

#include <fmt/format.h>
#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <celutil/gettext.h>

#include <commctrl.h>

#include "res/resource.h"
#include "tstring.h"

namespace celestia::win32
{

namespace
{

BOOL
DisplayModeDialogInit(HWND hDlg, DisplayModeDialog* displayModeDlg)
{
    SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LPARAM>(displayModeDlg));

    HWND hwnd = GetDlgItem(hDlg, IDC_COMBO_RESOLUTION);

    // Add windowed mode as the first item on the menu
    tstring str = UTF8ToTString(_("Windowed Mode"));
    SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(str.c_str()));

    fmt::basic_memory_buffer<TCHAR> buf;
    for (const DEVMODE& displayMode : displayModeDlg->displayModes)
    {
        buf.clear();
        fmt::format_to(std::back_inserter(buf), TEXT("{} x {} x {}"),
                       displayMode.dmPelsWidth, displayMode.dmPelsHeight,
                       displayMode.dmBitsPerPel);
        buf.push_back(TEXT('\0'));
        SendMessage(hwnd, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(buf.data()));
    }

    SendMessage(hwnd, CB_SETCURSEL, displayModeDlg->screenMode, 0);
    return TRUE;
}

BOOL
DisplayModeDialogCommand(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         DisplayModeDialog* displayModeDlg)
{
    switch (LOWORD(wParam))
    {
    case IDOK:
        displayModeDlg->update = true;
        [[fallthrough]];

    case IDCANCEL:
        if (displayModeDlg->parent != nullptr)
        {
            SendMessage(displayModeDlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(displayModeDlg));
        }
        EndDialog(hDlg, 0);
        return TRUE;

    case IDC_COMBO_RESOLUTION:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            HWND hwnd = reinterpret_cast<HWND>(lParam);
            int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
            if (item != CB_ERR)
                displayModeDlg->screenMode = item;
        }
        return TRUE;

    default:
        return FALSE;
    }
}

INT_PTR APIENTRY
SelectDisplayModeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DisplayModeDialog* displayModeDlg;
    if (message == WM_INITDIALOG)
    {
        displayModeDlg = reinterpret_cast<DisplayModeDialog*>(lParam);
        return DisplayModeDialogInit(hDlg, displayModeDlg);
    }

    displayModeDlg = reinterpret_cast<DisplayModeDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!displayModeDlg || displayModeDlg->hwnd != hDlg)
        return FALSE;

    switch (message)
    {
    case WM_DESTROY:
        if (displayModeDlg->parent != nullptr)
        {
            SendMessage(displayModeDlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(displayModeDlg));
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    case WM_COMMAND:
        return DisplayModeDialogCommand(hDlg, wParam, lParam, displayModeDlg);
    }

    return FALSE;
}

} // end unnamed namespace

DisplayModeDialog::DisplayModeDialog(HINSTANCE _appInstance,
                                     HWND _parent,
                                     util::array_view<DEVMODE> _displayModes,
                                     int _screenMode) :
    parent(_parent),
    displayModes(_displayModes),
    screenMode(_screenMode)
{
    hwnd = CreateDialogParam(_appInstance,
                             MAKEINTRESOURCE(IDD_DISPLAYMODE),
                             parent,
                             &SelectDisplayModeProc,
                             reinterpret_cast<LPARAM>(this));
}



} // end namespace celestia::win32
