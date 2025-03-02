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

#include "winviewoptsdlg.h"

#include <array>
#include <cmath>

#include <commctrl.h>

#include <celengine/body.h>
#include <celutil/flag.h>
#include "res/resource.h"

namespace celestia::win32
{

namespace
{

constexpr int DistanceSliderRange = 10000;
constexpr float MinDistanceLimit = 1.0f;
constexpr float MaxDistanceLimit = 1.0e6f;

BOOL APIENTRY
ViewOptionsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG)
    {
        ViewOptionsDialog* Dlg = reinterpret_cast<ViewOptionsDialog*>(lParam);
        if (Dlg == NULL)
            return EndDialog(hDlg, 0);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        //Read labelMode, renderFlags and hud detail
        Dlg->initialRenderFlags = Dlg->appCore->getRenderer()->getRenderFlags();
        Dlg->initialLabelMode = Dlg->appCore->getRenderer()->getLabelMode();
        Dlg->initialHudDetail = Dlg->appCore->getHudDetail();

        //Set dialog controls to reflect current label and render modes
        Dlg->SetControls(hDlg);

        return TRUE;
    }

    ViewOptionsDialog* Dlg = reinterpret_cast<ViewOptionsDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
    case WM_COMMAND:
    {
        Renderer* renderer = Dlg->appCore->getRenderer();
        RenderFlags renderFlags = renderer->getRenderFlags();
        RenderLabels labelMode = renderer->getLabelMode();
        BodyClassification orbitMask = renderer->getOrbitMask();

        switch (LOWORD(wParam))
        {
        case IDC_SHOWATMOSPHERES:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowAtmospheres);
            break;
        case IDC_SHOWCELESTIALGRID:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowCelestialSphere);
            break;
        case IDC_SHOWHORIZONGRID:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowHorizonGrid);
            break;
        case IDC_SHOWGALACTICGRID:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowGalacticGrid);
            break;
        case IDC_SHOWECLIPTICGRID:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowEclipticGrid);
            break;
        case IDC_SHOWECLIPTIC:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowEcliptic);
            break;
        case IDC_SHOWCLOUDS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowCloudMaps);
            break;
        case IDC_SHOWCLOUDSHADOWS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowCloudShadows);
            break;
        case IDC_SHOWCONSTELLATIONS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowDiagrams);
            break;
        case IDC_SHOWECLIPSESHADOWS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowEclipseShadows);
            break;
        case IDC_SHOWGALAXIES:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowGalaxies);
            break;
        case IDC_SHOWGLOBULARS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowGlobulars);
            break;
        case IDC_SHOWNEBULAE:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowNebulae);
            break;
        case IDC_SHOWOPENCLUSTERS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowOpenClusters);
            break;
        case IDC_SHOWNIGHTSIDELIGHTS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowNightMaps);
            break;
        case IDC_SHOWORBITS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowOrbits);
            break;
        case IDC_SHOWPARTIALTRAJECTORIES:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowPartialTrajectories);
            break;
        case IDC_SHOWFADINGORBITS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowFadingOrbits);
            break;
        case IDC_PLANETORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::Planet);
            break;
        case IDC_DWARFPLANETORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::DwarfPlanet);
            break;
        case IDC_STARORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::Stellar);
            break;
        case IDC_MOONORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::Moon);
            break;
        case IDC_MINORMOONORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::MinorMoon);
            break;
        case IDC_ASTEROIDORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::Asteroid);
            break;
        case IDC_COMETORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::Comet);
            break;
        case IDC_SPACECRAFTORBITS:
            renderer->setOrbitMask(orbitMask ^ BodyClassification::Spacecraft);
            break;
        case IDC_SHOWPLANETS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowPlanets);
            break;
        case IDC_SHOWDWARFPLANETS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowDwarfPlanets);
            break;
        case IDC_SHOWMOONS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowMoons);
            break;
        case IDC_SHOWMINORMOONS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowMinorMoons);
            break;
        case IDC_SHOWASTEROIDS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowAsteroids);
            break;
        case IDC_SHOWCOMETS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowComets);
            break;
        case IDC_SHOWSPACECRAFTS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowSpacecrafts);
            break;
        case IDC_SHOWSTARS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowStars);
            break;
        case IDC_SHOWCONSTELLATIONBORDERS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowBoundaries);
            break;
        case IDC_SHOWRINGSHADOWS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowRingShadows);
            break;
        case IDC_SHOWRINGS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowPlanetRings);
            break;
        case IDC_SHOWCOMETTAILS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowCometTails);
            break;
        case IDC_SHOWMARKERS:
            renderer->setRenderFlags(renderFlags ^ RenderFlags::ShowMarkers);
            break;
        case IDC_LABELCONSTELLATIONS:
            renderer->setLabelMode(labelMode ^ RenderLabels::ConstellationLabels);
            break;
        case IDC_LABELCONSTELLATIONSLATIN:
            renderer->setLabelMode(labelMode ^ RenderLabels::I18nConstellationLabels);
            break;
        case IDC_LABELGALAXIES:
            renderer->setLabelMode(labelMode ^ RenderLabels::GalaxyLabels);
            break;
        case IDC_LABELGLOBULARS:
            renderer->setLabelMode(labelMode ^ RenderLabels::GlobularLabels);
            break;
        case IDC_LABELNEBULAE:
            renderer->setLabelMode(labelMode ^ RenderLabels::NebulaLabels);
            break;
        case IDC_LABELOPENCLUSTERS:
            renderer->setLabelMode(labelMode ^ RenderLabels::OpenClusterLabels);
            break;
        case IDC_LABELPLANETS:
            renderer->setLabelMode(labelMode ^ RenderLabels::PlanetLabels);
            break;
        case IDC_LABELDWARFPLANETS:
            renderer->setLabelMode(labelMode ^ RenderLabels::DwarfPlanetLabels);
            break;
        case IDC_LABELMOONS:
            renderer->setLabelMode(labelMode ^ RenderLabels::MoonLabels);
            break;
        case IDC_LABELMINORMOONS:
            renderer->setLabelMode(labelMode ^ RenderLabels::MinorMoonLabels);
            break;
        case IDC_LABELSTARS:
            renderer->setLabelMode(labelMode ^ RenderLabels::StarLabels);
            break;
        case IDC_LABELASTEROIDS:
            renderer->setLabelMode(labelMode ^ RenderLabels::AsteroidLabels);
            break;
        case IDC_LABELCOMETS:
            renderer->setLabelMode(labelMode ^ RenderLabels::CometLabels);
            break;
        case IDC_LABELSPACECRAFT:
            renderer->setLabelMode(labelMode ^ RenderLabels::SpacecraftLabels);
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
                    HWND hwnd = GetDlgItem(hDlg, IDC_EDIT_FILTER_DISTANCE);
                    float logDistanceLimit = static_cast<float>(HIWORD(wParam)) /
                        static_cast<float>(DistanceSliderRange);
                    auto distanceLimit = static_cast<int>(std::pow(MaxDistanceLimit, logDistanceLimit));

                    SetDlgItemInt(hDlg, IDC_EDIT_FILTER_DISTANCE, distanceLimit, FALSE);

                    Dlg->appCore->getRenderer()->setDistanceLimit(distanceLimit);
                    break;
                }

            default:
                break;
            }
        }
    }

    return FALSE;
}

template<typename T>
void
dlgCheckEnum(HWND hDlg, WORD item, T flags, T f)
{
    SendDlgItemMessage(hDlg, item, BM_SETCHECK,
                       util::is_set(flags, f) ? BST_CHECKED : BST_UNCHECKED, 0);
}

} // end unnamed namespace

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
                             (DLGPROC)ViewOptionsProc,
                             reinterpret_cast<LONG_PTR>(this));
}

void ViewOptionsDialog::SetControls(HWND hDlg)
{
    RenderFlags renderFlags = appCore->getRenderer()->getRenderFlags();
    RenderLabels labelMode = appCore->getRenderer()->getLabelMode();
    int hudDetail = appCore->getHudDetail();
    BodyClassification orbitMask = appCore->getRenderer()->getOrbitMask();

    //Set checkboxes and radiobuttons
    dlgCheckEnum(hDlg, IDC_SHOWATMOSPHERES, renderFlags, RenderFlags::ShowAtmospheres);
    dlgCheckEnum(hDlg, IDC_SHOWCELESTIALGRID, renderFlags, RenderFlags::ShowCelestialSphere);
    dlgCheckEnum(hDlg, IDC_SHOWHORIZONGRID, renderFlags, RenderFlags::ShowHorizonGrid);
    dlgCheckEnum(hDlg, IDC_SHOWGALACTICGRID, renderFlags, RenderFlags::ShowGalacticGrid);
    dlgCheckEnum(hDlg, IDC_SHOWECLIPTICGRID, renderFlags, RenderFlags::ShowEclipticGrid);
    dlgCheckEnum(hDlg, IDC_SHOWECLIPTIC, renderFlags, RenderFlags::ShowEcliptic);
    dlgCheckEnum(hDlg, IDC_SHOWCLOUDS, renderFlags, RenderFlags::ShowCloudMaps);
    dlgCheckEnum(hDlg, IDC_SHOWCLOUDSHADOWS, renderFlags, RenderFlags::ShowCloudShadows);
    dlgCheckEnum(hDlg, IDC_SHOWCONSTELLATIONS, renderFlags, RenderFlags::ShowDiagrams);
    dlgCheckEnum(hDlg, IDC_SHOWECLIPSESHADOWS, renderFlags, RenderFlags::ShowEclipseShadows);
    dlgCheckEnum(hDlg, IDC_SHOWGALAXIES, renderFlags, RenderFlags::ShowGalaxies);
    dlgCheckEnum(hDlg, IDC_SHOWGLOBULARS, renderFlags, RenderFlags::ShowGlobulars);
    dlgCheckEnum(hDlg, IDC_SHOWNEBULAE, renderFlags, RenderFlags::ShowNebulae);
    dlgCheckEnum(hDlg, IDC_SHOWOPENCLUSTERS, renderFlags, RenderFlags::ShowOpenClusters);
    dlgCheckEnum(hDlg, IDC_SHOWNIGHTSIDELIGHTS, renderFlags, RenderFlags::ShowNightMaps);
    dlgCheckEnum(hDlg, IDC_SHOWORBITS, renderFlags, RenderFlags::ShowOrbits);
    dlgCheckEnum(hDlg, IDC_SHOWFADINGORBITS, renderFlags, RenderFlags::ShowFadingOrbits);
    dlgCheckEnum(hDlg, IDC_SHOWPARTIALTRAJECTORIES, renderFlags, RenderFlags::ShowPartialTrajectories);
    dlgCheckEnum(hDlg, IDC_PLANETORBITS, orbitMask, BodyClassification::Planet);
    dlgCheckEnum(hDlg, IDC_DWARFPLANETORBITS,orbitMask, BodyClassification::DwarfPlanet);
    dlgCheckEnum(hDlg, IDC_MOONORBITS, orbitMask, BodyClassification::Moon);
    dlgCheckEnum(hDlg, IDC_MINORMOONORBITS, orbitMask, BodyClassification::MinorMoon);
    dlgCheckEnum(hDlg, IDC_ASTEROIDORBITS, orbitMask, BodyClassification::Asteroid);
    dlgCheckEnum(hDlg, IDC_COMETORBITS, orbitMask, BodyClassification::Comet);
    dlgCheckEnum(hDlg, IDC_SPACECRAFTORBITS, orbitMask, BodyClassification::Spacecraft);
    dlgCheckEnum(hDlg, IDC_STARORBITS, orbitMask, BodyClassification::Stellar);
    dlgCheckEnum(hDlg, IDC_SHOWPLANETS, renderFlags, RenderFlags::ShowPlanets);
    dlgCheckEnum(hDlg, IDC_SHOWDWARFPLANETS, renderFlags, RenderFlags::ShowDwarfPlanets);
    dlgCheckEnum(hDlg, IDC_SHOWMOONS, renderFlags, RenderFlags::ShowMoons);
    dlgCheckEnum(hDlg, IDC_SHOWMINORMOONS, renderFlags, RenderFlags::ShowMinorMoons);
    dlgCheckEnum(hDlg, IDC_SHOWASTEROIDS, renderFlags, RenderFlags::ShowAsteroids);
    dlgCheckEnum(hDlg, IDC_SHOWCOMETS, renderFlags, RenderFlags::ShowComets);
    dlgCheckEnum(hDlg, IDC_SHOWSPACECRAFTS, renderFlags, RenderFlags::ShowSpacecrafts);
    dlgCheckEnum(hDlg, IDC_SHOWSTARS, renderFlags, RenderFlags::ShowStars);
    dlgCheckEnum(hDlg, IDC_SHOWCONSTELLATIONBORDERS, renderFlags, RenderFlags::ShowBoundaries);
    dlgCheckEnum(hDlg, IDC_SHOWRINGSHADOWS, renderFlags, RenderFlags::ShowRingShadows);
    dlgCheckEnum(hDlg, IDC_SHOWRINGS, renderFlags, RenderFlags::ShowPlanetRings);
    dlgCheckEnum(hDlg, IDC_SHOWCOMETTAILS, renderFlags, RenderFlags::ShowCometTails);
    dlgCheckEnum(hDlg, IDC_SHOWMARKERS, renderFlags, RenderFlags::ShowMarkers);

    dlgCheckEnum(hDlg, IDC_LABELCONSTELLATIONS, labelMode, RenderLabels::ConstellationLabels);
    dlgCheckEnum(hDlg, IDC_LABELCONSTELLATIONSLATIN, ~labelMode, RenderLabels::I18nConstellationLabels); // check box if flag unset
    dlgCheckEnum(hDlg, IDC_LABELGALAXIES, labelMode, RenderLabels::GalaxyLabels);
    dlgCheckEnum(hDlg, IDC_LABELGLOBULARS, labelMode, RenderLabels::GlobularLabels);
    dlgCheckEnum(hDlg, IDC_LABELNEBULAE, labelMode, RenderLabels::NebulaLabels);
    dlgCheckEnum(hDlg, IDC_LABELOPENCLUSTERS, labelMode, RenderLabels::OpenClusterLabels);
    dlgCheckEnum(hDlg, IDC_LABELSTARS, labelMode, RenderLabels::StarLabels);
    dlgCheckEnum(hDlg, IDC_LABELPLANETS, labelMode, RenderLabels::PlanetLabels);
    dlgCheckEnum(hDlg, IDC_LABELDWARFPLANETS, labelMode, RenderLabels::DwarfPlanetLabels);
    dlgCheckEnum(hDlg, IDC_LABELMOONS, labelMode, RenderLabels::MoonLabels);
    dlgCheckEnum(hDlg, IDC_LABELMINORMOONS, labelMode, RenderLabels::MinorMoonLabels);
    dlgCheckEnum(hDlg, IDC_LABELASTEROIDS, labelMode, RenderLabels::AsteroidLabels);
    dlgCheckEnum(hDlg, IDC_LABELCOMETS, labelMode, RenderLabels::CometLabels);
    dlgCheckEnum(hDlg, IDC_LABELSPACECRAFT, labelMode, RenderLabels::SpacecraftLabels);

    CheckRadioButton(hDlg, IDC_INFOTEXT0, IDC_INFOTEXT2, IDC_INFOTEXT0 + hudDetail);

    // Set up distance slider
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FILTER_DISTANCE,
                       TBM_SETRANGE,
                       (WPARAM)TRUE,
                       (LPARAM) MAKELONG(0, DistanceSliderRange));
    float distanceLimit = appCore->getRenderer()->getDistanceLimit();
    float logDistanceLimit = std::log(distanceLimit) / std::log(MaxDistanceLimit);
    SendDlgItemMessage(hDlg,
                       IDC_SLIDER_FILTER_DISTANCE,
                       TBM_SETPOS,
                       (WPARAM) TRUE,
                       (LPARAM) (logDistanceLimit * DistanceSliderRange));

    SetDlgItemInt(hDlg, IDC_EDIT_FILTER_DISTANCE, static_cast<UINT>(distanceLimit), FALSE);
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

} // end namespace celestia::win32
