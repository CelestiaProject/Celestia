// winlocations.cpp
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous utilities for Locations UI implementation.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winlocations.h"
#include "res/resource.h"
#include <celutil/winutil.h>

using namespace std;

static const int FeatureSizeSliderRange = 1000;
static const float MinFeatureSize = 1.0f;
static const float MaxFeatureSize = 100.0f;

static BOOL APIENTRY LocationsProc(HWND hDlg,
                                   UINT message,
                                   UINT wParam,
                                   LONG lParam)
{
    LocationsDialog* dlg = reinterpret_cast<LocationsDialog*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            dlg = reinterpret_cast<LocationsDialog*>(lParam);
            if (dlg == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);

            // Store original settings in case user cancels the dialog
            // dlg->initialLocationFlags = dlg->appCore->getSimulation()->getActiveObserver();
            dlg->initialLocationFlags = 0;
            dlg->initialFeatureSize = dlg->appCore->getRenderer()->getMinimumFeatureSize();

            // Set dialog controls to reflect current label and render modes
            dlg->SetControls(hDlg);

            return(TRUE);
        }
        break;

    case WM_COMMAND:
    {
        Renderer* renderer = dlg->appCore->getRenderer();
        //uint32 locationFlags = renderer->getRenderFlags();
        uint32 locationsFlags = 0;
        
        switch (LOWORD(wParam))
        {
        case IDC_SHOW_CITIES:
            break;
        case IDC_SHOW_OBSERVATORIES:
            break;
        case IDC_SHOW_LANDING_SITES:
            break;
        case IDC_SHOW_MONTES:
            break;
        case IDC_SHOW_MARIA:
            break;
        case IDC_SHOW_CRATERS:
            break;
        case IDOK:
            if (dlg != NULL && dlg->parent != NULL)
            {
                SendMessage(dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        case IDCANCEL:
            if (dlg != NULL && dlg->parent != NULL)
            {
                // Reset render flags, label mode, and hud detail to
                // initial values
                dlg->RestoreSettings(hDlg);
                SendMessage(dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    case WM_DESTROY:
        if (dlg != NULL && dlg->parent != NULL)
        {
            SendMessage(dlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(dlg));
        }
        return TRUE;

    case WM_HSCROLL:
        {
            WORD sbValue = LOWORD(wParam);
            switch (sbValue)
            {
            case SB_THUMBTRACK:
                {
                    char val[16];
                    HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FEATURE_SIZE);
                    float featureSize = (float) HIWORD(wParam) /
                        (float) FeatureSizeSliderRange;
                    featureSize = MinFeatureSize + (MaxFeatureSize - MinFeatureSize) * featureSize;
                    sprintf(val, "%d", (int) featureSize);
                    SetWindowText(hwnd, val);
                    dlg->appCore->getRenderer()->setMinimumFeatureSize(featureSize);
                    break;
                }

            default:
                break;
            }
        }
    }

    return FALSE;
}


LocationsDialog::LocationsDialog(HINSTANCE appInstance,
                                 HWND _parent,
                                 CelestiaCore* _appCore) :
    CelestiaWatcher(*_appCore),
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_LOCATIONS),
                             parent,
                             LocationsProc,
                             reinterpret_cast<LONG>(this));
}


void LocationsDialog::SetControls(HWND hDlg)
{
#if 0
    // Set checkboxes and radiobuttons
    SendDlgItemMessage(hDlg, IDC_SHOWATMOSPHERES, BM_SETCHECK,
        ((renderFlags & appCore->getRenderer()->ShowAtmospheres) != 0)? BST_CHECKED:BST_UNCHECKED, 0);
#endif

    // Set up feature size slider
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FEATURE_SIZE,
                       TBM_SETRANGE,
                       (WPARAM)TRUE,
                       (LPARAM) MAKELONG(0, FeatureSizeSliderRange));
    float featureSize = appCore->getRenderer()->getMinimumFeatureSize();
    int sliderPos = (int) (FeatureSizeSliderRange * (featureSize - MinFeatureSize) / (MaxFeatureSize - MinFeatureSize));
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FEATURE_SIZE,
                       TBM_SETPOS,
                       (WPARAM) TRUE,
                       (LPARAM) sliderPos);
    
    char val[16];
    HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FEATURE_SIZE);
    sprintf(val, "%d", (int) featureSize);
    SetWindowText(hwnd, val);
}


void LocationsDialog::RestoreSettings(HWND hDlg)
{
}

void LocationsDialog::notifyChange(CelestiaCore*, int)
{
    if (parent != NULL)
        SetControls(hwnd);
}
