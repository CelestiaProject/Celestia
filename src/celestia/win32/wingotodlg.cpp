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
#include "celutil/winutil.h"

#include "res/resource.h"

using namespace Eigen;
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

static bool SetDialogFloat(HWND hDlg, int id, char* format, float f)
{
    char buf[128];

    sprintf(buf, format, f);

    return (SetDlgItemText(hDlg, id, buf) == TRUE);
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

            //Initialize name, distance, latitude and longitude edit boxes with current values.
            Simulation* sim = gotoDlg->appCore->getSimulation();
            double distance, longitude, latitude;
            sim->getSelectionLongLat(distance, longitude, latitude);

            //Display information in format appropriate for object
            if (sim->getSelection().body() != NULL)
            {
                distance = distance - (double) sim->getSelection().body()->getRadius();
                SetDialogFloat(hDlg, IDC_EDIT_DISTANCE, "%.1f", (float)distance);
                SetDialogFloat(hDlg, IDC_EDIT_LONGITUDE, "%.5f", (float)longitude);
                SetDialogFloat(hDlg, IDC_EDIT_LATITUDE, "%.5f", (float)latitude);
                SetDlgItemText(hDlg, IDC_EDIT_OBJECTNAME,
                 const_cast<char*>(UTF8ToCurrentCP(sim->getSelection().body()->getName(true)).c_str()));
            }
//            else if (sim->getSelection().star != NULL)
//            {
//                //Code to obtain searchable star name
//               SetDlgItemText(hDlg, IDC_EDIT_OBJECTNAME, (char*)sim->getSelection().star->);
//            }

            return(TRUE);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_GOTO)
        {
            char buf[1024], out[1024];
            wchar_t wbuff[1024];
            int len = GetDlgItemText(hDlg, IDC_EDIT_OBJECTNAME, buf, sizeof buf);

            Simulation* sim = gotoDlg->appCore->getSimulation();
            Selection sel;
            if (len > 0) 
            {
                int wlen = MultiByteToWideChar(CP_ACP, 0, buf, -1, wbuff, sizeof(wbuff));
                WideCharToMultiByte(CP_UTF8, 0, wbuff, wlen, out, sizeof(out), NULL, NULL);
                sel = sim->findObjectFromPath(string(out), true);
            }
            
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

                float longitude, latitude;
                if (GetDialogFloat(hDlg, IDC_EDIT_LONGITUDE, longitude) &&
                    GetDialogFloat(hDlg, IDC_EDIT_LATITUDE, latitude))
                {
                    sim->gotoSelectionLongLat(5.0,
                                              distance,
                                              degToRad(longitude),
                                              degToRad(latitude),
                                              Vector3f::UnitY());
                }
                else
                {
                    sim->gotoSelection(5.0,
                                       distance,
                                       Vector3f::UnitY(),
                                       ObserverFrame::ObserverLocal);
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
