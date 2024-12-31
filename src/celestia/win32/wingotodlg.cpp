// wingotodlg.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Original version:
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Goto object dialog for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "wingotodlg.h"

#include <array>
#include <cstddef>
#include <string_view>
#include <system_error>
#ifdef _UNICODE
#include <cwctype>
#else
#include <cctype>
#endif

#include <Eigen/Core>

#include <fmt/format.h>
#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <celastro/astro.h>
#include <celengine/body.h>
#include <celengine/observer.h>
#include <celengine/simulation.h>
#include <celestia/celestiacore.h>
#include <celmath/mathlib.h>
#include "res/resource.h"
#include "tcharconv.h"
#include "tstring.h"

namespace celestia::win32
{

namespace
{

bool
GetDialogFloat(HWND hDlg, int id, float& f)
{
    std::array<TCHAR, 128> buf;
    UINT size = GetDlgItemText(hDlg, id, buf.data(), buf.size());
    if (size == 0)
        return false;

    const TCHAR* start = buf.data();
    const TCHAR* end = buf.data() + size;
#ifdef _UNICODE
    while (start != end && std::iswspace(static_cast<std::wint_t>(*start)))
#else
    while (start != end && std::isspace(static_cast<unsigned char>(*start)))
#endif
        ++start;

    if (start == end)
        return false;

    return from_tchars(start, end, f).ec == std::errc{};
}

bool
SetDialogFloat(HWND hDlg, int id, unsigned int precision, float f)
{
    std::array<TCHAR, 128> buf;
    auto it = fmt::format_to_n(buf.begin(), buf.size() - 1, TEXT("{:.{}f}"), f, precision).out;
    *it = TEXT('\0');
    return (SetDlgItemText(hDlg, id, buf.data()) == TRUE);
}

INT_PTR CALLBACK
GotoObjectProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG)
    {
        auto gotoDlg = reinterpret_cast<GotoObjectDialog*>(lParam);
        if (gotoDlg == NULL)
            return EndDialog(hDlg, 0);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
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
            SetDialogFloat(hDlg, IDC_EDIT_DISTANCE, 1, (float)distance);
            SetDialogFloat(hDlg, IDC_EDIT_LONGITUDE, 5, (float)longitude);
            SetDialogFloat(hDlg, IDC_EDIT_LATITUDE, 5, (float)latitude);
            SetDlgItemText(hDlg, IDC_EDIT_OBJECTNAME,
                const_cast<TCHAR*>(UTF8ToTString(sim->getSelection().body()->getName(true)).c_str()));
        }

        return TRUE;
    }

    auto gotoDlg = reinterpret_cast<GotoObjectDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_GOTO)
        {
            std::array<TCHAR, 1024> buf;
            int len = GetDlgItemText(hDlg, IDC_EDIT_OBJECTNAME, buf.data(), buf.size());

            Simulation* sim = gotoDlg->appCore->getSimulation();
            Selection sel;
            if (len > 0)
            {
                fmt::memory_buffer path;
                AppendTCharToUTF8(tstring_view(buf.data(), static_cast<std::size_t>(len)), path);
                sel = sim->findObjectFromPath(std::string_view(path.data(), path.size()), true);
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
                        distance = distance * static_cast<float>(sel.radius());

                    distance += static_cast<float>(sel.radius());
                }

                float longitude, latitude;
                if (GetDialogFloat(hDlg, IDC_EDIT_LONGITUDE, longitude) &&
                    GetDialogFloat(hDlg, IDC_EDIT_LATITUDE, latitude))
                {
                    sim->gotoSelectionLongLat(5.0,
                                              distance,
                                              math::degToRad(longitude),
                                              math::degToRad(latitude),
                                              Eigen::Vector3f::UnitY());
                }
                else
                {
                    sim->gotoSelection(5.0,
                                       distance,
                                       Eigen::Vector3f::UnitY(),
                                       ObserverFrame::CoordinateSystem::ObserverLocal);
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

} // end unnamed namespace

GotoObjectDialog::GotoObjectDialog(HINSTANCE appInstance,
                                   HWND _parent,
                                   CelestiaCore* _appCore) :
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_GOTO_OBJECT),
                             parent,
                             &GotoObjectProc,
                             reinterpret_cast<LPARAM>(this));
}

} // end namespace celestia::win32
