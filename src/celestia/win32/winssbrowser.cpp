// winssbrowser.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Solar system browser tool for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winssbrowser.h"

#include <commctrl.h>

#include <celengine/body.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/solarsys.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include "res/resource.h"
#include "tstring.h"

namespace celestia::win32
{

namespace
{

HTREEITEM
AddItemToTree(HWND hwndTV, tstring_view itemText, int nLevel, const void* data, HTREEITEM parent)
{
    static HTREEITEM hPrev = TVI_FIRST;

    TVITEM tvi;
    tvi.mask = TVIF_TEXT | TVIF_PARAM;

    // Set the text of the item.
    tvi.pszText = const_cast<TCHAR*>(itemText.data());
    tvi.cchTextMax = static_cast<int>(itemText.size());

    // Save the heading level in the item's application-defined
    // data area.
    tvi.lParam = reinterpret_cast<LPARAM>(data);

    TVINSERTSTRUCT tvins;
    tvins.item = tvi;
    tvins.hInsertAfter = hPrev;
    tvins.hParent = parent;

    // Add the item to the tree view control.
    hPrev = reinterpret_cast<HTREEITEM>(SendMessage(hwndTV, TVM_INSERTITEM, 0,
                                                    reinterpret_cast<LPARAM>(&tvins)));

    return hPrev;
}

void
AddPlanetarySystemToTree(const PlanetarySystem* sys, HWND treeView, int level, HTREEITEM parent)
{
    for (int i = 0; i < sys->getSystemSize(); ++i)
    {
        const Body* world = sys->getBody(i);
        if (world != nullptr &&
            world->getClassification() != BodyClassification::Invisible &&
            !world->getName().empty())
        {
            HTREEITEM item;
            item = AddItemToTree(treeView,
                                 UTF8ToTString(world->getName(true)),
                                 level,
                                 world,
                                 parent);

            const PlanetarySystem* satellites = world->getSatellites();
            if (satellites != nullptr)
                AddPlanetarySystemToTree(satellites, treeView, level + 1, item);
        }
    }
}

INT_PTR CALLBACK
SolarSystemBrowserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG)
    {
        SolarSystemBrowser* browser = reinterpret_cast<SolarSystemBrowser*>(lParam);
        if (browser == nullptr)
            return EndDialog(hDlg, 0);

        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        HWND hwnd = GetDlgItem(hDlg, IDC_SSBROWSER_TREE);
        const SolarSystem* solarSys = browser->appCore->getSimulation()->getNearestSolarSystem();
        if (solarSys != NULL)
        {
            const Universe* u = browser->appCore->getSimulation()->getUniverse();
            tstring starNameString = UTF8ToTString(u->getStarCatalog()->getStarName(*(solarSys->getStar())));
            HTREEITEM rootItem = AddItemToTree(hwnd, starNameString, 1, nullptr, TVI_ROOT);
            const PlanetarySystem* planets = solarSys->getPlanets();
            if (planets != NULL)
                AddPlanetarySystemToTree(planets, hwnd, 2, rootItem);

            SendMessage(hwnd, TVM_EXPAND, TVE_EXPAND, reinterpret_cast<LPARAM>(rootItem));
        }
        return TRUE;
    }

    auto browser = reinterpret_cast<SolarSystemBrowser*>(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
        }
        return(TRUE);

    case WM_DESTROY:
        if (browser != NULL && browser->parent != NULL)
        {
            SendMessage(browser->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(browser));
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (browser != NULL && browser->parent != NULL)
            {
                SendMessage(browser->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(browser));
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_BUTTON_CENTER)
        {
            browser->appCore->charEntered('c');
        }
        else if (LOWORD(wParam) == IDC_BUTTON_GOTO)
        {
            browser->appCore->charEntered('G');
        }
        break;

    case WM_NOTIFY:
        {
            auto hdr = reinterpret_cast<LPNMHDR>(lParam);

            if (hdr->code == TVN_SELCHANGED)
            {
                auto nm = reinterpret_cast<LPNMTREEVIEW>(lParam);
                auto body = reinterpret_cast<Body*>(nm->itemNew.lParam);
                if (body != nullptr)
                {
                    browser->appCore->getSimulation()->setSelection(Selection(body));
                }
                else
                {
                    // If the body is NULL, assume that this is the tree item for
                    // the sun.
                    const SolarSystem* solarSys = browser->appCore->getSimulation()->getNearestSolarSystem();
                    if (solarSys != NULL && solarSys->getStar() != NULL)
                        browser->appCore->getSimulation()->setSelection(Selection(solarSys->getStar()));
                }
            }
        }
    }

    return FALSE;
}

} // end unnamed namespace

SolarSystemBrowser::SolarSystemBrowser(HINSTANCE appInstance,
                                       HWND _parent,
                                       CelestiaCore* _appCore) :
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_SSBROWSER),
                             parent,
                             &SolarSystemBrowserProc,
                             reinterpret_cast<LONG_PTR>(this));
}

SolarSystemBrowser::~SolarSystemBrowser()
{
    SetWindowLongPtr(hwnd, DWLP_USER, 0);
}

} // end namespace celestia::win32
