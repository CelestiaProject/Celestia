// winfinddlg.h
//
// Copyright (C) 2023, Celestia Development Team
//
// Extracted from winmain.cpp:
// Copyright (C) 2001-2007, Chris Laurel <claurel@shatters.net>
//
// Find object dialog.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winfinddlg.h"

#include <array>
#include <string_view>

#include <fmt/format.h>

#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>

#include "res/resource.h"
#include "tstring.h"

namespace celestia::win32
{

namespace
{

class FindObjectDialog
{
public:
    explicit FindObjectDialog(const CelestiaCore*);

    bool checkHWnd(HWND hWnd) const { return hWnd == hDlg; }
    BOOL init(HWND);
    BOOL command(WPARAM, LPARAM);

private:
    HWND hDlg;
    const CelestiaCore* appCore;
};

FindObjectDialog::FindObjectDialog(const CelestiaCore* _appCore) :
    appCore(_appCore)
{
}

BOOL
FindObjectDialog::init(HWND _hDlg)
{
    hDlg = _hDlg;
    SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LPARAM>(this));
    return TRUE;
}

BOOL
FindObjectDialog::command(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDOK:
        {
            std::array<TCHAR, 1024> buf;
            int len = GetDlgItemText(hDlg, IDC_FINDOBJECT_EDIT, buf.data(), buf.size());
            if (len <= 0)
                return TRUE;

            fmt::memory_buffer pathBuf;
            AppendTCharToUTF8(buf.data(), pathBuf);
            std::string_view path(pathBuf.data(), pathBuf.size());
            if (Selection sel = appCore->getSimulation()->findObjectFromPath(path, true);
                !sel.empty())
            {
                appCore->getSimulation()->setSelection(sel);
            }

            EndDialog(hDlg, 0);
            return TRUE;
        }
    case IDCANCEL:
        EndDialog(hDlg, 0);
        return FALSE;

    default:
        break;
    }

    return FALSE;
}

INT_PTR APIENTRY
FindObjectProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    FindObjectDialog* findObjectDlg;
    if (message == WM_INITDIALOG)
    {
        findObjectDlg = reinterpret_cast<FindObjectDialog*>(lParam);
        return findObjectDlg->init(hDlg);
    }

    findObjectDlg = reinterpret_cast<FindObjectDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (!findObjectDlg || !findObjectDlg->checkHWnd(hDlg))
        return FALSE;

    if (message == WM_COMMAND)
        return findObjectDlg->command(wParam, lParam);

    return FALSE;
}

} // end unnamed namespace

void ShowFindObjectDialog(HINSTANCE appInstance,
                          HWND appWindow,
                          const CelestiaCore* celestiaCore)
{
    FindObjectDialog findObjectDlg{celestiaCore};
    DialogBoxParam(appInstance,
                   MAKEINTRESOURCE(IDD_FINDOBJECT),
                   appWindow,
                   &FindObjectProc,
                   reinterpret_cast<LPARAM>(&findObjectDlg));
}

} // end namespace celestia::win32
