// wintourguide.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Space 'tour guide' dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <sstream>
#include <algorithm>
#include <set>
#include <windows.h>
#include <commctrl.h>
#include "wintourguide.h"

#include "../res/resource.h"

using namespace std;


BOOL APIENTRY TourGuideProc(HWND hDlg,
                            UINT message,
                            UINT wParam,
                            LONG lParam)
{
    TourGuide* tourGuide = reinterpret_cast<TourGuide*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            TourGuide* guide = reinterpret_cast<TourGuide*>(lParam);
            if (guide == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);

            HWND hwnd = GetDlgItem(hDlg, IDC_COMBO_TOURGUIDE);
            const DestinationList* destinations = guide->appCore->getDestinations();
            if (hwnd != NULL && destinations != NULL)
            {
                int count = 0;
                for (DestinationList::const_iterator iter = destinations->begin();
                     iter != destinations->end(); iter++)
                {
                    Destination* dest = *iter;
                    if (dest != NULL)
                    {
                        cout << "Adding item: " << dest->name << '\n';
                        COMBOBOXEXITEM item;

                        item.iItem = -1;
                        item.mask = CBEIF_TEXT;
                        item.pszText = const_cast<char*>(dest->name.c_str());
                        item.cchTextMax = dest->name.length();
#if 0
                        SendMessage(hwnd, CBEM_INSERTITEM, 0,
                                    reinterpret_cast<LPARAM>(&item));
#endif
                        SendMessage(hwnd, CB_INSERTSTRING, -1,
                                    reinterpret_cast<LPARAM>(dest->name.c_str()));
                    }
                }
            }
        }
        return(TRUE);

    case WM_DESTROY:
        if (tourGuide != NULL && tourGuide->parent != NULL)
        {
            SendMessage(tourGuide->parent, WM_COMMAND, ID_CLOSE_TOURGUIDE,
                        reinterpret_cast<LPARAM>(tourGuide));
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (tourGuide != NULL && tourGuide->parent != NULL)
            {
                SendMessage(tourGuide->parent, WM_COMMAND, ID_CLOSE_TOURGUIDE,
                            reinterpret_cast<LPARAM>(tourGuide));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_BUTTON_GOTO)
        {
            tourGuide->appCore->charEntered('G');
        }
        else if (LOWORD(wParam) == IDC_COMBO_TOURGUIDE)
        {
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hwnd = reinterpret_cast<HWND>(lParam);
                int item = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
                const DestinationList* destinations = tourGuide->appCore->getDestinations();
                if (item != CB_ERR && item < destinations->size())
                {
                    Destination* dest = (*destinations)[item];
                    Selection sel = tourGuide->appCore->getSimulation()->findObjectFromPath(dest->target);

                    SetDlgItemText(hDlg,
                                   IDC_TEXT_DESCRIPTION,
                                   dest->description.c_str());

                    if (!sel.empty())
                    {
                        tourGuide->appCore->getSimulation()->setSelection(sel);
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}


TourGuide::TourGuide(HINSTANCE appInstance,
                     HWND _parent,
                     CelestiaCore* _appCore) :
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_TOURGUIDE),
                             parent,
                             TourGuideProc,
                             reinterpret_cast<LONG>(this));
}
