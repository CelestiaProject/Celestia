// winstarbrowser.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Star browser tool for Windows.
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
#include "winstarbrowser.h"

#include "../res/resource.h"

using namespace std;


// TODO: More of the functions in this module should be converted to
// methods of the StarBrowser class.

enum {
    BrightestStars = 0,
    NearestStars = 1,
};


bool InitStarBrowserColumns(HWND listView)
{
    LVCOLUMN lvc;
    LVCOLUMN columns[5];

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 60;
    lvc.pszText = "";

    int nColumns = sizeof(columns) / sizeof(columns[0]);
    int i;

    for (i = 0; i < nColumns; i++)
        columns[i] = lvc;

    columns[0].pszText = "Name";
    columns[0].cx = 100;
    columns[1].pszText = "Distance";
    columns[1].fmt = LVCFMT_RIGHT;
    columns[1].cx = 75;
    columns[2].pszText = "App. mag";
    columns[2].fmt = LVCFMT_RIGHT;
    columns[3].pszText = "Abs. mag";
    columns[3].fmt = LVCFMT_RIGHT;
    columns[4].pszText = "Type";

    for (i = 0; i < nColumns; i++)
    {
        columns[i].iSubItem = i;
        if (ListView_InsertColumn(listView, i, &columns[i]) == -1)
            return false;
    }

    return true;
}


struct CloserStarPredicate
{
    Point3f pos;
    bool operator()(const Star* star0, const Star* star1) const
    {
        return ((pos - star0->getPosition()).lengthSquared() <
                (pos - star1->getPosition()).lengthSquared());
                               
    }
};

struct BrighterStarPredicate
{
    Point3f pos;
    UniversalCoord ucPos;
    bool operator()(const Star* star0, const Star* star1) const
    {
        float d0 = pos.distanceTo(star0->getPosition());
        float d1 = pos.distanceTo(star1->getPosition());

        // If the stars are closer than one light year, use
        // a more precise distance estimate.
        if (d0 < 1.0f)
            d0 = (star0->getPosition() - ucPos).length();
        if (d1 < 1.0f)
            d1 = (star1->getPosition() - ucPos).length();

        return (star0->getApparentMagnitude(d0) <
                star1->getApparentMagnitude(d1));
    }
};


// Find the nearest/brightest/X-est N stars in a database.  The
// supplied predicate determines which of two stars is a better match.
template<class Pred> vector<const Star*>*
FindStars(const StarDatabase& stardb, Pred pred, int nStars)
{
    vector<const Star*>* finalStars = new vector<const Star*>();
    if (nStars == 0)
        return finalStars;

    typedef multiset<const Star*, Pred> StarSet;
    StarSet firstStars(pred);

    int totalStars = stardb.size();
    if (totalStars < nStars)
        nStars = totalStars;

    // We'll need at least nStars in the set, so first fill
    // up the list indiscriminately.
    int i = 0;
    for (i = 0; i < nStars; i++)
        firstStars.insert(stardb.getStar(i));

    // From here on, only add a star to the set if it's
    // a better match than the worst matching star already
    // in the set.
    const Star* lastStar = *--firstStars.end();
    for (; i < totalStars; i++)
    {
        Star* star = stardb.getStar(i);
        if (pred(star, lastStar))
        {
            firstStars.insert(star);
            firstStars.erase(--firstStars.end());
            lastStar = *--firstStars.end();
        }
    }

    // Move the best matching stars into the vector
    finalStars->reserve(nStars);
    for (StarSet::const_iterator iter = firstStars.begin();
         iter != firstStars.end(); iter++)
    {
        finalStars->insert(finalStars->end(), *iter);
    }

    return finalStars;
}


bool InitStarBrowserLVItems(HWND listView, vector<const Star*>& stars)
{
    LVITEM lvi;

    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;

    for (int i = 0; i < stars.size(); i++)
    {
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.lParam = (LPARAM) stars[i];
        ListView_InsertItem(listView, &lvi);
    }

    return true;
}


bool InitStarBrowserItems(HWND listView, StarBrowser* browser)
{
    Simulation* sim = browser->appCore->getSimulation();
    StarDatabase* stardb = browser->appCore->getSimulation()->getStarDatabase();
    vector<const Star*>* stars = NULL;
    switch (browser->predicate)
    {
    case BrightestStars:
        {
            BrighterStarPredicate brighterPred;
            brighterPred.pos = browser->pos;
            brighterPred.ucPos = browser->ucPos;
            stars = FindStars(*stardb, brighterPred, browser->nStars);
        }
        break;

    case NearestStars:
        {
            CloserStarPredicate closerPred;
            closerPred.pos = browser->pos;
            stars = FindStars(*stardb, closerPred, browser->nStars);
        }
        break;

    default:
        return false;
    }
            
    bool succeeded = InitStarBrowserLVItems(listView, *stars);
    delete stars;

    return succeeded;
}


// Crud used for the list item display callbacks
static string starNameString("");
static char callbackScratch[256];
static ostringstream browserOss(ostringstream::out);

struct StarBrowserSortInfo
{
    int subItem;
    Point3f pos;
    UniversalCoord ucPos;
};

int CALLBACK StarBrowserCompareFunc(LPARAM lParam0, LPARAM lParam1,
                                    LPARAM lParamSort)
{
    StarBrowserSortInfo* sortInfo = reinterpret_cast<StarBrowserSortInfo*>(lParamSort);
    Star* star0 = reinterpret_cast<Star*>(lParam0);
    Star* star1 = reinterpret_cast<Star*>(lParam1);

    switch (sortInfo->subItem)
    {
    case 0:
        return 0;

    case 1:
        {
            float d0 = sortInfo->pos.distanceTo(star0->getPosition());
            float d1 = sortInfo->pos.distanceTo(star1->getPosition());
            return sign(d0 - d1);
        }

    case 2:
        {
            float d0 = sortInfo->pos.distanceTo(star0->getPosition());
            float d1 = sortInfo->pos.distanceTo(star1->getPosition());
            if (d0 < 1.0f)
                d0 = (star0->getPosition() - sortInfo->ucPos).length();
            if (d1 < 1.0f)
                d1 = (star1->getPosition() - sortInfo->ucPos).length();
            return sign(astro::absToAppMag(star0->getAbsoluteMagnitude(), d0) -
                        astro::absToAppMag(star1->getAbsoluteMagnitude(), d1));
        }

    case 3:
        return sign(star0->getAbsoluteMagnitude() - star1->getAbsoluteMagnitude());

    case 4:
        if (star0->getStellarClass() < star1->getStellarClass())
            return -1;
        else if (star1->getStellarClass() < star0->getStellarClass())
            return 1;
        else
            return 0;

    default:
        return 0;
    }
}


void StarBrowserDisplayItem(LPNMLVDISPINFOA nm, StarBrowser* browser)
{
    Star* star = reinterpret_cast<Star*>(nm->item.lParam);
    if (star == NULL)
    {
        nm->item.pszText = "";
        return;
    }

    switch (nm->item.iSubItem)
    {
    case 0:
        {
            Simulation* sim = browser->appCore->getSimulation();
            starNameString = sim->getStarDatabase()->getStarName(*star);
            nm->item.pszText = const_cast<char*>(starNameString.c_str());
        }
        break;
            
    case 1:
        {
            sprintf(callbackScratch, "%.3f",
                    browser->pos.distanceTo(star->getPosition()));
            nm->item.pszText = callbackScratch;
        }
        break;

    case 2:
        {
            Vec3f r = star->getPosition() - browser->ucPos;
            float appMag = astro::absToAppMag(star->getAbsoluteMagnitude(),
                                              r.length());
            sprintf(callbackScratch, "%.2f", appMag);
            nm->item.pszText = callbackScratch;
        }
        break;
            
    case 3:
        {
            sprintf(callbackScratch, "%.2f", star->getAbsoluteMagnitude());
            nm->item.pszText = callbackScratch;
        }
        break;

    case 4:
        {
            // Convoluted way of getting the stellar class string . . .
            // Seek to the beginning of an stringstream, output, then grab
            // the string and copy it to the scratch buffer.
            browserOss.seekp(0);
            browserOss << star->getStellarClass() << '\0';
            strcpy(callbackScratch, browserOss.str().c_str());
            nm->item.pszText = callbackScratch;
        }
    }
}


BOOL APIENTRY StarBrowserProc(HWND hDlg,
                              UINT message,
                              UINT wParam,
                              LONG lParam)
{
    StarBrowser* browser = reinterpret_cast<StarBrowser*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            cout << "StarBrowserProc: WM_INITDIALOG\n";
            StarBrowser* browser = reinterpret_cast<StarBrowser*>(lParam);
            if (browser == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);

            HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
            InitStarBrowserColumns(hwnd);
            InitStarBrowserItems(hwnd, browser);
            CheckRadioButton(hDlg, IDC_RADIO_NEAREST, IDC_RADIO_BRIGHTEST, IDC_RADIO_NEAREST);
            return(TRUE);
        }

    case WM_DESTROY:
        if (browser != NULL && browser->parent != NULL)
        {
            SendMessage(browser->parent, WM_COMMAND, ID_CLOSE_STARBROWSER,
                        reinterpret_cast<LPARAM>(browser));
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            if (browser != NULL && browser->parent != NULL)
            {
                SendMessage(browser->parent, WM_COMMAND, ID_CLOSE_STARBROWSER,
                            reinterpret_cast<LPARAM>(browser));
            }
            EndDialog(hDlg, 0);
            return TRUE;

        case IDC_BUTTON_CENTER:
            browser->appCore->charEntered('C');
            break;

        case IDC_BUTTON_GOTO:
            browser->appCore->charEntered('G');
            break;

        case IDC_RADIO_BRIGHTEST:
            browser->predicate = BrightestStars;
            break;

        case IDC_RADIO_NEAREST:
            browser->predicate = NearestStars;
            break;

        case IDC_BUTTON_REFRESH:
            {
                Simulation* sim = browser->appCore->getSimulation();
                browser->ucPos = sim->getObserver().getPosition();
                browser->pos = (Point3f) browser->ucPos;
                HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
                if (hwnd != 0)
                {
                    ListView_DeleteAllItems(hwnd);
                    InitStarBrowserItems(hwnd, browser);
                }
            }
            break;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR) lParam;
            
            if (hdr->code == LVN_GETDISPINFO)
            {
                StarBrowserDisplayItem((LPNMLVDISPINFOA) lParam, browser);
            }
            else if (hdr->code == LVN_ITEMCHANGED)
            {
                LPNMLISTVIEW nm = (LPNMLISTVIEW) lParam;
                if ((nm->uNewState & LVIS_SELECTED) != 0)
                {
                    Simulation* sim = browser->appCore->getSimulation();
                    Star* star = reinterpret_cast<Star*>(nm->lParam);
                    if (star != NULL)
                        sim->setSelection(Selection(star));
                }
            }
            else if (hdr->code == LVN_COLUMNCLICK)
            {
                HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
                if (hwnd != 0)
                {
                    LPNMLISTVIEW nm = (LPNMLISTVIEW) lParam;
                    StarBrowserSortInfo sortInfo;
                    sortInfo.subItem = nm->iSubItem;
                    sortInfo.ucPos = browser->ucPos;
                    sortInfo.pos = browser->pos;
                    ListView_SortItems(hwnd, StarBrowserCompareFunc,
                                       reinterpret_cast<LPARAM>(&sortInfo));
                }
            }
        }
        break;
    }

    return FALSE;
}


StarBrowser::StarBrowser(HINSTANCE appInstance,
                         HWND _parent,
                         CelestiaCore* _appCore) :
    appCore(_appCore),
    parent(_parent)
{
    ucPos = appCore->getSimulation()->getObserver().getPosition();
    pos = (Point3f) ucPos;

    predicate = NearestStars;
    nStars = 100;

    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_STARBROWSER),
                             parent,
                             StarBrowserProc,
                             reinterpret_cast<LONG>(this));
}
