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

#include <string>
#include <sstream>
#include <algorithm>
#include <set>
#include <windows.h>
#include <commctrl.h>
#include "winssbrowser.h"
#include "celutil/winutil.h"

#include "res/resource.h"

using namespace std;


HTREEITEM AddItemToTree(HWND hwndTV, LPSTR lpszItem, int nLevel, void* data,
                        HTREEITEM parent)
{ 
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM) TVI_FIRST; 

#if 0 
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 
#endif
    tvi.mask = TVIF_TEXT | TVIF_PARAM;
 
    // Set the text of the item. 
    tvi.pszText = lpszItem; 
    tvi.cchTextMax = lstrlen(lpszItem); 

    // Save the heading level in the item's application-defined 
    // data area. 
    tvi.lParam = (LPARAM) data; 
 
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 
    tvins.hParent = parent;
 
    // Add the item to the tree view control. 
    hPrev = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEM, 0, 
                                    (LPARAM) (LPTVINSERTSTRUCT) &tvins); 
 
#if 0 
    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (nLevel > 1)
    { 
        hti = TreeView_GetParent(hwndTV, hPrev); 
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
        tvi.hItem = hti; 
        // tvi.iImage = g_nClosed; 
        // tvi.iSelectedImage = g_nClosed; 
        TreeView_SetItem(hwndTV, &tvi); 
    }
#endif 
 
    return hPrev; 
}


void AddPlanetarySystemToTree(const PlanetarySystem* sys, HWND treeView, int level, HTREEITEM parent)
{
    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        Body* world = sys->getBody(i);
        if (world != NULL && world->getClassification() != Body::Invisible && !world->getName().empty())
        {
            HTREEITEM item;
            item = AddItemToTree(treeView, 
                                 const_cast<char*>(UTF8ToCurrentCP(world->getName(true)).c_str()),
                                 level,
                                 static_cast<void*>(world),
                                 parent);

            const PlanetarySystem* satellites = world->getSatellites();
            if (satellites != NULL)
                AddPlanetarySystemToTree(satellites, treeView, level + 1, item);
        }
    }
}


BOOL APIENTRY SolarSystemBrowserProc(HWND hDlg,
                                     UINT message,
                                     UINT wParam,
                                     LONG lParam)
{
    SolarSystemBrowser* browser = reinterpret_cast<SolarSystemBrowser*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            SolarSystemBrowser* browser = reinterpret_cast<SolarSystemBrowser*>(lParam);
            if (browser == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);

            HWND hwnd = GetDlgItem(hDlg, IDC_SSBROWSER_TREE);
            const SolarSystem* solarSys = browser->appCore->getSimulation()->getNearestSolarSystem();
            if (solarSys != NULL)
            {
                Universe* u = browser->appCore->getSimulation()->getUniverse();
                string starNameString = UTF8ToCurrentCP(u->getStarCatalog()->getStarName(*(solarSys->getStar())));
                HTREEITEM rootItem = AddItemToTree(hwnd, const_cast<char*>(starNameString.c_str()), 1, NULL,
                                                   (HTREEITEM) TVI_ROOT);
                const PlanetarySystem* planets = solarSys->getPlanets();
                if (planets != NULL)
                    AddPlanetarySystemToTree(planets, hwnd, 2, rootItem);

                SendMessage(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM) rootItem); 
            }
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
            LPNMHDR hdr = (LPNMHDR) lParam;
            
            if (hdr->code == TVN_SELCHANGED)
            {
                LPNMTREEVIEW nm = (LPNMTREEVIEW) lParam;
                Body* body = reinterpret_cast<Body*>(nm->itemNew.lParam);
                if (body != NULL)
                {
                    browser->appCore->getSimulation()->setSelection(Selection(body));
                }
                else
                {
                    // If the body is NULL, assume that this is the tree item for
                    // the sun.
                    const SolarSystem* solarSys = browser->appCore->getSimulation()->getNearestSolarSystem();
                    if (solarSys != NULL && solarSys->getStar() != NULL)
                        browser->appCore->getSimulation()->setSelection(Selection(const_cast<Star*>(solarSys->getStar())));
                }
            }
        }
    }

    return FALSE;
}


SolarSystemBrowser::SolarSystemBrowser(HINSTANCE appInstance,
                                       HWND _parent,
                                       CelestiaCore* _appCore) :
    appCore(_appCore),
    parent(_parent)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_SSBROWSER),
                             parent,
                             SolarSystemBrowserProc,
                             reinterpret_cast<LONG>(this));
}


SolarSystemBrowser::~SolarSystemBrowser()
{
    SetWindowLong(hwnd, DWL_USER, 0);
}
