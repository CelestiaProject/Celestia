// wingotodlg.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Goto object dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <windows.h>
#include "wingotodlg.h"

#include "../res/resource.h"

using namespace std;


static bool GetDialogFloat(HWND hDlg, int id, float& f)
{
    char buf[128];
    
    if (GetDlgItemText(hDlg, id, buf, sizeof buf) > 0 &&
        sscanf(buf, " %f", &f) == 1)
        return true;
    else
        return false;
}


static BOOL APIENTRY GotoObjectProc(HWND hDlg,
                                    UINT message,
                                    UINT wParam,
                                    LONG lParam)
{
    GotoObjectDialog* gotoDlg = reinterpret_cast<GotoObjectDialog*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            GotoObjectDialog* gotoDlg = reinterpret_cast<GotoObjectDialog*>(lParam);
            if (gotoDlg == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);
            CheckRadioButton(hDlg,
                             IDC_RADIO_KM, IDC_RADIO_RADII,
                             IDC_RADIO_KM);
            return(TRUE);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_GOTO)
        {
            char buf[1024];
            int len = GetDlgItemText(hDlg, IDC_EDIT_OBJECTNAME, buf, sizeof buf);
            Simulation* sim = gotoDlg->appCore->getSimulation();
            Selection sel;
            if (len > 0)
                sel = sim->findObjectFromPath(buf);
            
            if (!sel.empty())
            {
                sim->setSelection(sel);
                sim->geosynchronousFollow();

                float distance = (float) (sel.radius() * 5);

                if (GetDialogFloat(hDlg, IDC_EDIT_DISTANCE, distance))
                {
                    if (IsDlgButtonChecked(hDlg, IDC_RADIO_AU) == BST_CHECKED)
                        distance = astro::AUtoKilometers(distance);
                    else if (IsDlgButtonChecked(hDlg, IDC_RADIO_RADII) == BST_CHECKED)
                        distance = distance * (float) sel.radius();
                    
                    distance += (float) sel.radius();
                }
                distance = astro::kilometersToLightYears(distance);

                float longitude, latitude;
                if (GetDialogFloat(hDlg, IDC_EDIT_LONGITUDE, longitude) &&
                    GetDialogFloat(hDlg, IDC_EDIT_LATITUDE, latitude))
                {
                    sim->gotoSelectionLongLat(5.0,
                                              distance,
                                              degToRad(longitude),
                                              degToRad(latitude),
                                              Vec3f(0, 1, 0));
                }
                else
                {
                    sim->gotoSelection(5.0,
                                       distance,
                                       Vec3f(0, 1, 0),
                                       astro::ObserverLocal);
                }
            }
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            if (gotoDlg != NULL && gotoDlg->parent != NULL)
            {
                SendMessage(gotoDlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(gotoDlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;

    case WM_DESTROY:
        if (gotoDlg != NULL && gotoDlg->parent != NULL)
        {
            SendMessage(gotoDlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(gotoDlg));
        }
        EndDialog(hDlg, 0);
        return TRUE;
    }

    return FALSE;
}


GotoObjectDialog::GotoObjectDialog(HINSTANCE appInstance,
                                   HWND _parent,
                                   CelestiaCore* _appCore) :
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_GOTO_OBJECT),
                             parent,
                             GotoObjectProc,
                             reinterpret_cast<LONG>(this));
}
