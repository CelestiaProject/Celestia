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

static const int FeatureSizeSliderRange = 100;
static const float MinFeatureSize = 1.0f;
static const float MaxFeatureSize = 100.0f;

static const uint32 FilterOther = ~(Location::City |
                                    Location::Observatory |
                                    Location::LandingSite |
                                    Location::Crater |
                                    Location::Mons |
                                    Location::Terra |
                                    Location::EruptiveCenter |
                                    Location::Vallis |
                                    Location::Mare);

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
        Observer* obs = dlg->appCore->getSimulation()->getActiveObserver();
        uint32 locationFilter = obs->getLocationFilter();
        
        switch (LOWORD(wParam))
        {
        case IDC_SHOW_CITIES:
            obs->setLocationFilter(locationFilter ^ Location::City);
            break;
        case IDC_SHOW_OBSERVATORIES:
            obs->setLocationFilter(locationFilter ^ Location::Observatory);
            break;
        case IDC_SHOW_LANDING_SITES:
            obs->setLocationFilter(locationFilter ^ Location::LandingSite);
            break;
        case IDC_SHOW_MONTES:
            obs->setLocationFilter(locationFilter ^ Location::Mons);
            break;
        case IDC_SHOW_MARIA:
            obs->setLocationFilter(locationFilter ^ Location::Mare);
            break;
        case IDC_SHOW_CRATERS:
            obs->setLocationFilter(locationFilter ^ Location::Crater);
            break;
        case IDC_SHOW_VALLES:
            obs->setLocationFilter(locationFilter ^ Location::Vallis);
            break;
        case IDC_SHOW_TERRAE:
            obs->setLocationFilter(locationFilter ^ Location::Terra);
            break;
        case IDC_SHOW_VOLCANOES:
            obs->setLocationFilter(locationFilter ^ Location::EruptiveCenter);
            break;
        case IDC_SHOW_OTHERS:
            obs->setLocationFilter(locationFilter ^ FilterOther);
            break;
        case IDC_LABELFEATURES:
            {
                Renderer* renderer = dlg->appCore->getRenderer();
                uint32 labelMode = renderer->getLabelMode();
                renderer->setLabelMode(labelMode ^ Renderer::LocationLabels);
                break;
            }
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
            LRESULT sliderPos;
            
            if (sbValue == SB_THUMBTRACK)
                sliderPos = HIWORD(wParam);
            else                            
                sliderPos = SendMessage(GetDlgItem(hDlg, IDC_SLIDER_FEATURE_SIZE), TBM_GETPOS, 0, 0);
                
            char val[16];
            HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FEATURE_SIZE);
            float featureSize = (float) sliderPos / (float) FeatureSizeSliderRange;
            featureSize = MinFeatureSize + (MaxFeatureSize - MinFeatureSize) * featureSize;
            sprintf(val, "%d", (int) featureSize);
            SetWindowText(hwnd, val);
            
            dlg->appCore->getRenderer()->setMinimumFeatureSize(featureSize);					
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


static void dlgCheck(HWND hDlg, WORD item, uint32 flags, uint32 f)
{
    SendDlgItemMessage(hDlg, item, BM_SETCHECK,
                       ((flags & f) != 0) ? BST_CHECKED : BST_UNCHECKED, 0);
}


void LocationsDialog::SetControls(HWND hDlg)
{
    Observer* obs = appCore->getSimulation()->getActiveObserver();
    uint32 locFilter = obs->getLocationFilter();

    dlgCheck(hDlg, IDC_SHOW_CITIES,        locFilter, Location::City);
    dlgCheck(hDlg, IDC_SHOW_OBSERVATORIES, locFilter, Location::Observatory);
    dlgCheck(hDlg, IDC_SHOW_LANDING_SITES, locFilter, Location::LandingSite);
    dlgCheck(hDlg, IDC_SHOW_MONTES,        locFilter, Location::Mons);
    dlgCheck(hDlg, IDC_SHOW_MARIA,         locFilter, Location::Mare);
    dlgCheck(hDlg, IDC_SHOW_CRATERS,       locFilter, Location::Crater);
    dlgCheck(hDlg, IDC_SHOW_VALLES,        locFilter, Location::Vallis);
    dlgCheck(hDlg, IDC_SHOW_TERRAE,        locFilter, Location::Terra);
    dlgCheck(hDlg, IDC_SHOW_VOLCANOES,        locFilter, Location::EruptiveCenter);
    dlgCheck(hDlg, IDC_SHOW_OTHERS,        locFilter, Location::Other);

    uint32 labelMode = appCore->getRenderer()->getLabelMode();
    dlgCheck(hDlg, IDC_LABELFEATURES,     labelMode, Renderer::LocationLabels);

    // Set up feature size slider
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FEATURE_SIZE,
                       TBM_SETRANGE,
                       (WPARAM)TRUE,
                       (LPARAM) MAKELONG(0, FeatureSizeSliderRange));
    float featureSize = appCore->getRenderer()->getMinimumFeatureSize();
    int sliderPos = (int) (FeatureSizeSliderRange *
                           (featureSize - MinFeatureSize) /
                           (MaxFeatureSize - MinFeatureSize));
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
