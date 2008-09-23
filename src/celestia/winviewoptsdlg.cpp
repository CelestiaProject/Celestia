// winviewoptsdlg.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// View Options dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <windows.h>
#include <commctrl.h>
#include "winviewoptsdlg.h"
#include "..\celengine\render.h"

#include "res/resource.h"

using namespace std;

static const int DistanceSliderRange = 10000;
static const float MinDistanceLimit = 1.0f;
static const float MaxDistanceLimit = 1.0e6f;

static BOOL APIENTRY ViewOptionsProc(HWND hDlg,
                                     UINT message,
                                     UINT wParam,
                                     LONG lParam)
{
    ViewOptionsDialog* Dlg = reinterpret_cast<ViewOptionsDialog*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            ViewOptionsDialog* Dlg = reinterpret_cast<ViewOptionsDialog*>(lParam);
            if (Dlg == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);

            //Read labelMode, renderFlags and hud detail
            Dlg->initialRenderFlags = Dlg->appCore->getRenderer()->getRenderFlags();
            Dlg->initialLabelMode = Dlg->appCore->getRenderer()->getLabelMode();
            Dlg->initialHudDetail = Dlg->appCore->getHudDetail();

            //Set dialog controls to reflect current label and render modes
            Dlg->SetControls(hDlg);

            return(TRUE);
        }
        break;

    case WM_COMMAND:
    {
        Renderer* renderer = Dlg->appCore->getRenderer();
        uint32 renderFlags = renderer->getRenderFlags();
        uint32 labelMode = renderer->getLabelMode();
        uint32 orbitMask = renderer->getOrbitMask();

        switch (LOWORD(wParam))
        {
        case IDC_SHOWATMOSPHERES:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowAtmospheres);
            break;
        case IDC_SHOWCELESTIALGRID:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowCelestialSphere);
            break;
        case IDC_SHOWHORIZONGRID:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowHorizonGrid);
            break;
        case IDC_SHOWGALACTICGRID:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowGalacticGrid);
            break;
        case IDC_SHOWECLIPTICGRID:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowEclipticGrid);
            break;
        case IDC_SHOWCLOUDS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowCloudMaps);
            break;
        case IDC_SHOWCLOUDSHADOWS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowCloudShadows);
            break;
        case IDC_SHOWCONSTELLATIONS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowDiagrams);
            break;
        case IDC_SHOWECLIPSESHADOWS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowEclipseShadows);
            break;
        case IDC_SHOWGALAXIES:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowGalaxies);
            break;
        case IDC_SHOWGLOBULARS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowGlobulars);
            break;
        case IDC_SHOWNEBULAE:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowNebulae);
            break;
        case IDC_SHOWOPENCLUSTERS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowOpenClusters);
            break;
        case IDC_SHOWNIGHTSIDELIGHTS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowNightMaps);
            break;
        case IDC_SHOWORBITS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowOrbits);
            break;
        case IDC_PLANETORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::Planet);
            break;
        case IDC_DWARFPLANETORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::DwarfPlanet);
            break;
        case IDC_STARORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::Stellar);
            break;
        case IDC_MOONORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::Moon);
            break;
        case IDC_MINORMOONORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::MinorMoon);
            break;
        case IDC_ASTEROIDORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::Asteroid);
            break;
        case IDC_COMETORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::Comet);
            break;
        case IDC_SPACECRAFTORBITS:
            renderer->setOrbitMask(orbitMask ^ Body::Spacecraft);
            break;
        case IDC_SHOWPLANETS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowPlanets);
            break;
        case IDC_SHOWSTARS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowStars);
            break;
        case IDC_SHOWCONSTELLATIONBORDERS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowBoundaries);
            break;
        case IDC_SHOWRINGSHADOWS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowRingShadows);
            break;
        case IDC_SHOWCOMETTAILS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowCometTails);
            break;
        case IDC_SHOWMARKERS:
            renderer->setRenderFlags(renderFlags ^ Renderer::ShowMarkers);
            break;
        case IDC_LABELCONSTELLATIONS:
            renderer->setLabelMode(labelMode ^ Renderer::ConstellationLabels);
            break;
        case IDC_LABELCONSTELLATIONSLATIN:
            renderer->setLabelMode(labelMode ^ Renderer::I18nConstellationLabels);
            break;
        case IDC_LABELGALAXIES:
            renderer->setLabelMode(labelMode ^ Renderer::GalaxyLabels);
            break;
		    case IDC_LABELGLOBULARS:
			      renderer->setLabelMode(labelMode ^ Renderer::GlobularLabels);
			      break;
		    case IDC_LABELNEBULAE:
			      renderer->setLabelMode(labelMode ^ Renderer::NebulaLabels);
			      break;
		    case IDC_LABELOPENCLUSTERS:
			      renderer->setLabelMode(labelMode ^ Renderer::OpenClusterLabels);
			      break;
        case IDC_LABELPLANETS:
            renderer->setLabelMode(labelMode ^ Renderer::PlanetLabels);
            break;
        case IDC_LABELDWARFPLANETS:
            renderer->setLabelMode(labelMode ^ Renderer::DwarfPlanetLabels);
            break;
        case IDC_LABELMOONS:
            renderer->setLabelMode(labelMode ^ Renderer::MoonLabels);
            break;
        case IDC_LABELMINORMOONS:
            renderer->setLabelMode(labelMode ^ Renderer::MinorMoonLabels);
            break;
        case IDC_LABELSTARS:
            renderer->setLabelMode(labelMode ^ Renderer::StarLabels);
            break;
        case IDC_LABELASTEROIDS:
            renderer->setLabelMode(labelMode ^ Renderer::AsteroidLabels);
            break;
        case IDC_LABELCOMETS:
            renderer->setLabelMode(labelMode ^ Renderer::CometLabels);
            break;
        case IDC_LABELSPACECRAFT:
            renderer->setLabelMode(labelMode ^ Renderer::SpacecraftLabels);
            break;
        case IDC_INFOTEXT0:
            Dlg->appCore->setHudDetail(0);
            break;
        case IDC_INFOTEXT1:
            Dlg->appCore->setHudDetail(1);
            break;
        case IDC_INFOTEXT2:
            Dlg->appCore->setHudDetail(2);
            break;
        case IDOK:
            if (Dlg != NULL && Dlg->parent != NULL)
            {
                SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(Dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        case IDCANCEL:
            if (Dlg != NULL && Dlg->parent != NULL)
            {
                // Reset render flags, label mode, and hud detail to
                // initial values
                Dlg->RestoreSettings(hDlg);
                SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(Dlg));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    case WM_DESTROY:
        if (Dlg != NULL && Dlg->parent != NULL)
        {
            SendMessage(Dlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(Dlg));
        }
        return TRUE;

    case WM_HSCROLL:
        {
            WORD sbValue = LOWORD(wParam);
            switch (sbValue)
            {
            case SB_THUMBTRACK:
                // case SB_ENDSCROLL:
                {
                    char val[16];
                    HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FILTER_DISTANCE);
                    float logDistanceLimit = (float) HIWORD(wParam) /
                        (float) DistanceSliderRange;
                    float distanceLimit = (float) pow(MaxDistanceLimit, 
                                                 logDistanceLimit);
                    sprintf(val, "%d", (int) distanceLimit);
                    SetWindowText(hwnd, val);
                    Dlg->appCore->getRenderer()->setDistanceLimit(distanceLimit);
                    break;
                }

            default:
                cout << "Slider msg: " << sbValue << '\n';
                break;
#if 0
            case SB_THUMBPOSITION:
                {
                    browser->nStars = (int)HIWORD(wParam);
                    RefreshItems(hDlg, browser);
                    break;
                }
#endif
            }
        }
    }

    return FALSE;
}


ViewOptionsDialog::ViewOptionsDialog(HINSTANCE appInstance,
                                     HWND _parent,
                                     CelestiaCore* _appCore) :
    CelestiaWatcher(*_appCore),
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_VIEWOPTIONS),
                             parent,
                             ViewOptionsProc,
                             reinterpret_cast<LONG>(this));
}


static void dlgCheck(HWND hDlg, WORD item, uint32 flags, uint32 f)
{
    SendDlgItemMessage(hDlg, item, BM_SETCHECK,
                       ((flags & f) != 0) ? BST_CHECKED : BST_UNCHECKED, 0);
}

void ViewOptionsDialog::SetControls(HWND hDlg)
{
    int renderFlags = appCore->getRenderer()->getRenderFlags();
    int labelMode = appCore->getRenderer()->getLabelMode();
    int hudDetail = appCore->getHudDetail();
    int orbitMask = appCore->getRenderer()->getOrbitMask();

    //Set checkboxes and radiobuttons
    SendDlgItemMessage(hDlg, IDC_SHOWATMOSPHERES, BM_SETCHECK,
        (renderFlags & Renderer::ShowAtmospheres)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCELESTIALGRID, BM_SETCHECK,
        (renderFlags & Renderer::ShowCelestialSphere)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWHORIZONGRID, BM_SETCHECK,
        (renderFlags & Renderer::ShowHorizonGrid)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWGALACTICGRID, BM_SETCHECK,
        (renderFlags & Renderer::ShowGalacticGrid)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWECLIPTICGRID, BM_SETCHECK,
        (renderFlags & Renderer::ShowEclipticGrid)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCLOUDS, BM_SETCHECK,
        (renderFlags & Renderer::ShowCloudMaps)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCLOUDSHADOWS, BM_SETCHECK,
        (renderFlags & Renderer::ShowCloudShadows)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCONSTELLATIONS, BM_SETCHECK,
        (renderFlags & Renderer::ShowDiagrams)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWECLIPSESHADOWS, BM_SETCHECK,
        (renderFlags & Renderer::ShowEclipseShadows)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWGALAXIES, BM_SETCHECK,
        (renderFlags & Renderer::ShowGalaxies)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWGLOBULARS, BM_SETCHECK,
        (renderFlags & Renderer::ShowGlobulars)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWNEBULAE, BM_SETCHECK,
        (renderFlags & Renderer::ShowNebulae)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWOPENCLUSTERS, BM_SETCHECK,
        (renderFlags & Renderer::ShowOpenClusters)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWNIGHTSIDELIGHTS, BM_SETCHECK,
        (renderFlags & Renderer::ShowNightMaps)? BST_CHECKED:BST_UNCHECKED, 0);
    dlgCheck(hDlg, IDC_SHOWORBITS,       renderFlags, Renderer::ShowOrbits);
    dlgCheck(hDlg, IDC_PLANETORBITS,     orbitMask,   Body::Planet);
    dlgCheck(hDlg, IDC_DWARFPLANETORBITS,     orbitMask,   Body::DwarfPlanet);
    dlgCheck(hDlg, IDC_MOONORBITS,       orbitMask,   Body::Moon);
    dlgCheck(hDlg, IDC_MINORMOONORBITS,       orbitMask,   Body::MinorMoon);
    dlgCheck(hDlg, IDC_ASTEROIDORBITS,   orbitMask,   Body::Asteroid);
    dlgCheck(hDlg, IDC_COMETORBITS,      orbitMask,   Body::Comet);
    dlgCheck(hDlg, IDC_SPACECRAFTORBITS, orbitMask,   Body::Spacecraft);
    dlgCheck(hDlg, IDC_STARORBITS,       orbitMask,   Body::Stellar);
    SendDlgItemMessage(hDlg, IDC_SHOWPLANETS, BM_SETCHECK,
        (renderFlags & Renderer::ShowPlanets)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWSTARS, BM_SETCHECK,
        (renderFlags & Renderer::ShowStars)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCONSTELLATIONBORDERS, BM_SETCHECK,
        (renderFlags & Renderer::ShowBoundaries)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWRINGSHADOWS, BM_SETCHECK,
        (renderFlags & Renderer::ShowRingShadows)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWCOMETTAILS, BM_SETCHECK,
        (renderFlags & Renderer::ShowCometTails)? BST_CHECKED:BST_UNCHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_SHOWMARKERS, BM_SETCHECK,
        (renderFlags & Renderer::ShowMarkers)? BST_CHECKED:BST_UNCHECKED, 0);

    dlgCheck(hDlg, IDC_LABELCONSTELLATIONS, labelMode, Renderer::ConstellationLabels);
    SendDlgItemMessage(hDlg, IDC_LABELCONSTELLATIONSLATIN, BM_SETCHECK,
                       ((labelMode & Renderer::I18nConstellationLabels) == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
    dlgCheck(hDlg, IDC_LABELGALAXIES,   labelMode, Renderer::GalaxyLabels);
	  dlgCheck(hDlg, IDC_LABELGLOBULARS, labelMode, Renderer::GlobularLabels);
	  dlgCheck(hDlg, IDC_LABELNEBULAE,    labelMode, Renderer::NebulaLabels);
	  dlgCheck(hDlg, IDC_LABELOPENCLUSTERS, labelMode, Renderer::OpenClusterLabels);
    dlgCheck(hDlg, IDC_LABELSTARS,      labelMode, Renderer::StarLabels);
    dlgCheck(hDlg, IDC_LABELPLANETS,    labelMode, Renderer::PlanetLabels);
    dlgCheck(hDlg, IDC_LABELDWARFPLANETS,    labelMode, Renderer::DwarfPlanetLabels);
    dlgCheck(hDlg, IDC_LABELMOONS,      labelMode, Renderer::MoonLabels);
    dlgCheck(hDlg, IDC_LABELMINORMOONS,      labelMode, Renderer::MinorMoonLabels);
    dlgCheck(hDlg, IDC_LABELASTEROIDS,  labelMode, Renderer::AsteroidLabels);
    dlgCheck(hDlg, IDC_LABELCOMETS,     labelMode, Renderer::CometLabels);
    dlgCheck(hDlg, IDC_LABELSPACECRAFT, labelMode, Renderer::SpacecraftLabels);

    CheckRadioButton(hDlg, IDC_INFOTEXT0, IDC_INFOTEXT2, IDC_INFOTEXT0 + hudDetail);

    // Set up distance slider
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FILTER_DISTANCE,
                       TBM_SETRANGE,
                       (WPARAM)TRUE,
                       (LPARAM) MAKELONG(0, DistanceSliderRange));
    float distanceLimit = appCore->getRenderer()->getDistanceLimit();
    float logDistanceLimit = (float) (log(distanceLimit) / 
                                      log(MaxDistanceLimit));
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FILTER_DISTANCE,
                       TBM_SETPOS,
                       (WPARAM) TRUE,
                       (LPARAM) (logDistanceLimit * DistanceSliderRange));

    char val[16];
    HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FILTER_DISTANCE);
    sprintf(val, "%d", (int) distanceLimit);
    SetWindowText(hwnd, val);
}


void ViewOptionsDialog::RestoreSettings(HWND hDlg)
{
    appCore->getRenderer()->setRenderFlags(initialRenderFlags);
    appCore->getRenderer()->setLabelMode(initialLabelMode);
    appCore->setHudDetail(initialHudDetail);
}

void ViewOptionsDialog::notifyChange(CelestiaCore*, int)
{
    if (parent != NULL)
        SetControls(hwnd);
}
