// winmain.cpp
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Windows front end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <process.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#include "gl.h"
#include "celestia.h"
#include "vecmath.h"
#include "quaternion.h"
#include "util.h"
#include "timer.h"
#include "mathlib.h"
#include "astro.h"
#include "celestiacore.h"

#include "../res/resource.h"


//----------------------------------
// Skeleton functions and variables.
//-----------------
char AppName[] = "Celestia";

static CelestiaCore* appCore = NULL;

// Timer info.
static double currentTime = 0.0;
static Timer* timer = NULL;

static bool fullscreen = false;
static bool bReady = false;

HINSTANCE appInstance;
HWND mainWindow = 0;
HWND ssBrowserWindow = 0;

static HMENU menuBar = 0;
static HACCEL acceleratorTable = 0;

bool cursorVisible = true;

astro::Date newTime(0.0);

#define INFINITE_MOUSE
static int lastX = 0;
static int lastY = 0;

#define ROTATION_SPEED  6
#define ACCELERATION    20.0f

static LRESULT CALLBACK MainWindowProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);


#define MENU_CHOOSE_PLANET   32000



void ChangeDisplayMode()
{
    DEVMODE device_mode;
  
    memset(&device_mode, 0, sizeof(DEVMODE));

    device_mode.dmSize = sizeof(DEVMODE);

    device_mode.dmPelsWidth  = 800;
    device_mode.dmPelsHeight = 600;
    device_mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

    ChangeDisplaySettings(&device_mode, CDS_FULLSCREEN);
}

  
void RestoreDisplayMode()
{
    ChangeDisplaySettings(0, 0);
}


void AppendLocationToMenu(string name, int index)
{
    MENUITEMINFO menuInfo;
    menuInfo.cbSize = sizeof(MENUITEMINFO);
    menuInfo.fMask = MIIM_SUBMENU;
    if (GetMenuItemInfo(menuBar, 4, TRUE, &menuInfo))
    {
        HMENU locationsMenu = menuInfo.hSubMenu;

        menuInfo.cbSize = sizeof MENUITEMINFO;
        menuInfo.fMask = MIIM_TYPE | MIIM_ID;
        menuInfo.fType = MFT_STRING;
        menuInfo.wID = ID_LOCATIONS_FIRSTLOCATION + index;
        menuInfo.dwTypeData = const_cast<char*>(name.c_str());
        InsertMenuItem(locationsMenu, index + 2, TRUE, &menuInfo);
    }
}


bool LoadItemTextFromFile(HWND hWnd,
                          int item,
                          char* filename)
{
    ifstream textFile(filename, ios::in | ios::binary);
    string s;

    if (!textFile.good())
    {
        SetDlgItemText(hWnd, item, "License file missing!\r\r\nSee http://www.gnu.org/copyleft/gpl.html");
        return true;
    }

    char c;
    while (textFile.get(c))
    {
        if (c == '\012' || c == '\014')
            s += "\r\r\n";
        else
            s += c;
    }

    SetDlgItemText(hWnd, item, s.c_str());

    return true;
}


BOOL APIENTRY AboutProc(HWND hDlg,
                        UINT message,
                        UINT wParam,
                        LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY LicenseProc(HWND hDlg,
                          UINT message,
                          UINT wParam,
                          LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        LoadItemTextFromFile(hDlg, IDC_LICENSE_TEXT, "COPYING");
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY GLInfoProc(HWND hDlg,
                         UINT message,
                         UINT wParam,
                         LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            char* vendor = (char*) glGetString(GL_VENDOR);
            char* render = (char*) glGetString(GL_RENDERER);
            char* version = (char*) glGetString(GL_VERSION);
            char* ext = (char*) glGetString(GL_EXTENSIONS);
            string s;
            s += "Vendor: ";
            if (vendor != NULL)
                s += vendor;
            s += "\r\r\n";
            
            s += "Renderer: ";
            if (render != NULL)
                s += render;
            s += "\r\r\n";

            s += "Version: ";
            if (version != NULL)
                s += version;
            s += "\r\r\n";

            s += "\r\r\nSupported Extensions:\r\r\n";
            if (ext != NULL)
            {
                string extString(ext);
                int pos = extString.find(' ', 0);
                while (pos != string::npos)
                {
                    extString.replace(pos, 1, "\r\r\n");
                    pos = extString.find(' ', pos);
                }
                s += extString;
            }

            SetDlgItemText(hDlg, IDC_GLINFO_TEXT, s.c_str());
        }
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY FindObjectProc(HWND hDlg,
                             UINT message,
                             UINT wParam,
                             LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char buf[1024];
            int len = GetDlgItemText(hDlg, IDC_FINDOBJECT_EDIT, buf, 1024);
            if (len > 0)
            {
                Selection sel = appCore->getSimulation()->findObject(string(buf));
                if (!sel.empty())
                    appCore->getSimulation()->setSelection(sel);
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY AddLocationProc(HWND hDlg,
                              UINT message,
                              UINT wParam,
                              LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            char buf[1024];
            int len = GetDlgItemText(hDlg, IDC_LOCATION_EDIT, buf, 1024);
            if (len > 0)
            {
                string name(buf);

                appCore->addFavorite(name);
                AppendLocationToMenu(name, appCore->getFavorites()->size() - 1);
            }
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, 0);
            return FALSE;
        }
        break;
    }

    return FALSE;
}


BOOL APIENTRY SetTimeProc(HWND hDlg,
                          UINT message,
                          UINT wParam,
                          LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            SYSTEMTIME sysTime;
            newTime = astro::Date(appCore->getSimulation()->getTime());
            sysTime.wYear = newTime.year;
            sysTime.wMonth = newTime.month;
            sysTime.wDay = newTime.day;
            sysTime.wDayOfWeek = ((int) ((double) newTime + 0.5) + 1) % 7;
            sysTime.wHour = newTime.hour;
            sysTime.wMinute = newTime.minute;
            sysTime.wSecond = (int) newTime.seconds;
            sysTime.wMilliseconds = 0;

            HWND hwnd = GetDlgItem(hDlg, IDC_DATEPICKER);
            if (hwnd != NULL)
            {
                DateTime_SetFormat(hwnd, "dd' 'MMM' 'yyy");
                DateTime_SetSystemtime(hwnd, GDT_VALID, &sysTime);
            }
            hwnd = GetDlgItem(hDlg, IDC_TIMEPICKER);
            if (hwnd != NULL)
            {
                DateTime_SetFormat(hwnd, "HH':'mm':'ss' UT'");
                DateTime_SetSystemtime(hwnd, GDT_VALID, &sysTime);
            }
        }
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            if (LOWORD(wParam) == IDOK)
                appCore->getSimulation()->setTime((double) newTime);
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR) lParam;

            if (hdr->code == DTN_DATETIMECHANGE)
            {
                LPNMDATETIMECHANGE change = (LPNMDATETIMECHANGE) lParam;
                if (change->dwFlags == GDT_VALID)
                {
                    astro::Date date(change->st.wYear, change->st.wMonth, change->st.wDay);
                    newTime.year = change->st.wYear;
                    newTime.month = change->st.wMonth;
                    newTime.day = change->st.wDay;
                    newTime.hour = change->st.wHour;
                    newTime.minute = change->st.wMinute;
                    newTime.seconds = change->st.wSecond + (double) change->st.wMilliseconds / 1000.0;
                }
            }
        }
    }

    return FALSE;
}


HTREEITEM AddItemToTree(HWND hwndTV, LPSTR lpszItem, int nLevel, void* data)
{ 
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM) TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 
    // HTREEITEM hti; 

#if 0 
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 
#endif
    tvi.mask = TVIF_TEXT | TVIF_PARAM;
 
    // Set the text of the item. 
    tvi.pszText = lpszItem; 
    tvi.cchTextMax = lstrlen(lpszItem); 
 
    // Assume the item is not a parent item, so give it a 
    // document image. 
    // tvi.iImage = g_nDocument; 
    // tvi.iSelectedImage = g_nDocument; 
 
    // Save the heading level in the item's application-defined 
    // data area. 
    tvi.lParam = (LPARAM) data; 
 
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 
 
    // Set the parent item based on the specified level. 
    if (nLevel == 1) 
        tvins.hParent = TVI_ROOT; 
    else if (nLevel == 2) 
        tvins.hParent = hPrevRootItem; 
    else 
        tvins.hParent = hPrevLev2Item; 
 
    // Add the item to the tree view control. 
    hPrev = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEM, 0, 
                                    (LPARAM) (LPTVINSERTSTRUCT) &tvins); 
 
    // Save the handle to the item. 
    if (nLevel == 1) 
        hPrevRootItem = hPrev; 
    else if (nLevel == 2) 
        hPrevLev2Item = hPrev; 

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


void AddPlanetarySystemToTree(const PlanetarySystem* sys, HWND treeView, int level)
{
    for (int i = 0; i < sys->getSystemSize(); i++)
    {
        Body* world = sys->getBody(i);
        (void) AddItemToTree(treeView, 
                             const_cast<char*>(world->getName().c_str()),
                             level,
                             static_cast<void*>(world));

        const PlanetarySystem* satellites = world->getSatellites();
        if (satellites != NULL)
            AddPlanetarySystemToTree(satellites, treeView, level + 1);
    }
}


BOOL APIENTRY SolarSystemBrowserProc(HWND hDlg,
                                     UINT message,
                                     UINT wParam,
                                     LONG lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            HWND hwnd = GetDlgItem(hDlg, IDC_SSBROWSER_TREE);
            const SolarSystem* solarSys = appCore->getSimulation()->getNearestSolarSystem();
            if (solarSys != NULL)
            {
                HTREEITEM rootItem = AddItemToTree(hwnd, "Sun", 1, NULL);
                const PlanetarySystem* planets = solarSys->getPlanets();
                if (planets != NULL)
                    AddPlanetarySystemToTree(planets, hwnd, 2);

                SendMessage(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM) rootItem); 
            }
        }
        return(TRUE);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            ssBrowserWindow = 0;
            EndDialog(hDlg, 0);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDC_BUTTON_CENTER)
        {
            appCore->charEntered('C');
        }
        else if (LOWORD(wParam) == IDC_BUTTON_GOTO)
        {
            appCore->charEntered('G');
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
                    appCore->getSimulation()->setSelection(Selection(body));
                }
            }
        }
    }

    return FALSE;
}


/*
 * A whole ton of code to implement the star browser tool.  Support
 * displaying, selecting, sorting of information from the star
 * database.  This should all probably be moved into a separate module.
 */

enum {
    BrightestStars = 0,
    NearestStars = 1,
};

struct StarBrowserInstance
{
    StarBrowserInstance(Simulation*);

    HWND hwnd;

    // The star browser data is valid for a particular point
    // in space, and for performance issues is not continuously
    // updated.
    Point3f pos;
    UniversalCoord ucPos;
    int predicate;
    int nStars;
};

StarBrowserInstance::StarBrowserInstance(Simulation* sim)
{
    ucPos = sim->getObserver().getPosition();
    pos = (Point3f) ucPos;
    predicate = NearestStars;
    nStars = 100;
}

static StarBrowserInstance* starBrowser = NULL;


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


// Find the nearest/brightest/whatever N stars in a database.  The
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

bool InitStarBrowserItems(HWND listView, int pred, int nItems)
{
    Simulation* sim = appCore->getSimulation();
    StarDatabase* stardb = sim->getStarDatabase();
    if (starBrowser == NULL)
        return false;

    vector<const Star*>* stars = NULL;
    switch (pred)
    {
    case BrightestStars:
        {
            BrighterStarPredicate brighterPred;
            brighterPred.pos = starBrowser->pos;
            brighterPred.ucPos = starBrowser->ucPos;
            stars = FindStars(*stardb, brighterPred, nItems);
        }
        break;

    case NearestStars:
        {
            CloserStarPredicate closerPred;
            closerPred.pos = starBrowser->pos;
            stars = FindStars(*stardb, closerPred, nItems);
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


void StarBrowserDisplayItem(LPNMLVDISPINFOA nm, Simulation* sim)
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
            starNameString = sim->getStarDatabase()->getStarName(star->getCatalogNumber());
            nm->item.pszText = const_cast<char*>(starNameString.c_str());
        }
        break;
            
    case 1:
        {
            sprintf(callbackScratch, "%.3f",
                    starBrowser->pos.distanceTo(star->getPosition()));
            nm->item.pszText = callbackScratch;
        }
        break;

    case 2:
        {
            Vec3f r = star->getPosition() - starBrowser->ucPos;
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
    Simulation* sim = appCore->getSimulation();

    switch (message)
    {
    case WM_INITDIALOG:
        {
            HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
            InitStarBrowserColumns(hwnd);
            InitStarBrowserItems(hwnd, starBrowser->predicate, starBrowser->nStars);
            CheckRadioButton(hDlg, IDC_RADIO_NEAREST, IDC_RADIO_BRIGHTEST, IDC_RADIO_NEAREST);
            return(TRUE);
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            delete starBrowser;
            starBrowser = NULL;
            EndDialog(hDlg, 0);
            return TRUE;

        case IDC_BUTTON_CENTER:
            appCore->charEntered('C');
            break;

        case IDC_BUTTON_GOTO:
            appCore->charEntered('G');
            break;

        case IDC_RADIO_BRIGHTEST:
            starBrowser->predicate = BrightestStars;
            break;

        case IDC_RADIO_NEAREST:
            starBrowser->predicate = NearestStars;
            break;

        case IDC_BUTTON_REFRESH:
            {
                starBrowser->ucPos = sim->getObserver().getPosition();
                starBrowser->pos = (Point3f) starBrowser->ucPos;
                HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
                if (hwnd != 0)
                {
                    ListView_DeleteAllItems(hwnd);
                    InitStarBrowserItems(hwnd, starBrowser->predicate, starBrowser->nStars);
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
                StarBrowserDisplayItem((LPNMLVDISPINFOA) lParam, sim);
            }
            else if (hdr->code == LVN_ITEMCHANGED)
            {
                LPNMLISTVIEW nm = (LPNMLISTVIEW) lParam;
                if ((nm->uNewState & LVIS_SELECTED) != 0)
                {
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
                    sortInfo.ucPos = starBrowser->ucPos;
                    sortInfo.pos = starBrowser->pos;
                    ListView_SortItems(hwnd, StarBrowserCompareFunc,
                                       reinterpret_cast<LPARAM>(&sortInfo));
                }
            }
        }
        break;
    }

    return FALSE;
}



HMENU CreateMenuBar()
{
    return LoadMenu(appInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));
}

static void setMenuItemCheck(int menuItem, bool checked)
{
    CheckMenuItem(menuBar, menuItem, checked ? MF_CHECKED : MF_UNCHECKED);
}


HMENU CreatePlanetarySystemMenu(const PlanetarySystem* planets)
{
    HMENU menu = CreatePopupMenu();
    
    for (int i = 0; i < planets->getSystemSize(); i++)
    {
        Body* body = planets->getBody(i);
        AppendMenu(menu, MF_STRING, MENU_CHOOSE_PLANET + i,
                   body->getName().c_str());
    }

    return menu;
}


VOID APIENTRY handlePopupMenu(HWND hwnd,
                              float x, float y,
                              const Selection& sel)
{
    HMENU hMenu;
    string name;

    hMenu = CreatePopupMenu();

    if (sel.body != NULL)
    {
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, sel.body->getName().c_str());
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, "&Goto");
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_FOLLOW, "&Follow");
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_SYNCORBIT, "S&ync Orbit");
        AppendMenu(hMenu, MF_STRING, ID_INFO, "&Info");

        const PlanetarySystem* satellites = sel.body->getSatellites();
        if (satellites != NULL && satellites->getSystemSize() != 0)
        {
            HMENU satMenu = CreatePlanetarySystemMenu(satellites);
            AppendMenu(hMenu, MF_POPUP | MF_STRING, (DWORD) satMenu,
                       "&Satellites");
        }
    }
    else if (sel.star != NULL)
    {
        Simulation* sim = appCore->getSimulation();
        name = sim->getStarDatabase()->getStarName(sel.star->getCatalogNumber());
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_CENTER, name.c_str());
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hMenu, MF_STRING, ID_NAVIGATION_GOTO, "&Goto");
        AppendMenu(hMenu, MF_STRING, ID_INFO, "&Info");

        SolarSystemCatalog* solarSystemCatalog = sim->getSolarSystemCatalog();
        SolarSystemCatalog::iterator iter = solarSystemCatalog->find(sel.star->getCatalogNumber());
        if (iter != solarSystemCatalog->end())
        {
            SolarSystem* solarSys = iter->second;
            HMENU planetsMenu = CreatePlanetarySystemMenu(solarSys->getPlanets());
            AppendMenu(hMenu,
                       MF_POPUP | MF_STRING,
                       (DWORD) planetsMenu,
                       "&Planets");
        }
    }

    POINT point;
    point.x = (int) x;
    point.y = (int) y;
    
    if (!fullscreen)
        ClientToScreen(hwnd, (LPPOINT) &point);

    appCore->getSimulation()->setSelection(sel);
    TrackPopupMenu(hMenu, 0, point.x, point.y, 0, hwnd, NULL);

    // TODO: Do we need to explicitly destroy submenus or does DestroyMenu
    // work recursively?
    DestroyMenu(hMenu);
}


// This still needs a lot of work . . .
// TODO: spawn isn't the best way to launch IE--we don't want to start a new
//       process every time.
// TODO: the location of IE is assumed to be c:\program files\internet explorer
// TODO: get rid of fixed urls
void ShowWWWInfo(const Selection& sel)
{
    string url;
    if (sel.body != NULL)
    {
        string name = sel.body->getName();
        for (int i = 0; i < name.size(); i++)
            name[i] = tolower(name[i]);

        url = string("http://www.nineplanets.org/") + name + ".html";
    }
    else if (sel.star != NULL)
    {
        char name[32];
        if ((sel.star->getCatalogNumber() & 0xf0000000) == 0)
            sprintf(name, "HD%d", sel.star->getCatalogNumber());
        else
            sprintf(name, "HIP%d", sel.star->getCatalogNumber() & ~0xf0000000);

        url = string("http://simbad.u-strasbg.fr/sim-id.pl?protocol=html&Ident=") + name;
    }

    char* command = "c:\\program files\\internet explorer\\iexplore";

    _spawnl(_P_NOWAIT, command, "iexplore", url.c_str(), NULL);
}


void ContextMenu(float x, float y, Selection sel)
{
    handlePopupMenu(mainWindow, x, y, sel);
}


void handleKey(WPARAM key, bool down)
{
    int k = -1;
    switch (key)
    {
    case VK_UP:
        k = CelestiaCore::Key_Up;
        break;
    case VK_DOWN:
        k = CelestiaCore::Key_Down;
        break;
    case VK_LEFT:
        k = CelestiaCore::Key_Left;
        break;
    case VK_RIGHT:
        k = CelestiaCore::Key_Right;
        break;
    case VK_HOME:
        k = CelestiaCore::Key_Home;
        break;
    case VK_END:
        k = CelestiaCore::Key_End;
        break;
    case VK_F1:
        k = CelestiaCore::Key_F1;
        break;
    case VK_F2:
        k = CelestiaCore::Key_F2;
        break;
    case VK_F3:
        k = CelestiaCore::Key_F3;
        break;
    case VK_F4:
        k = CelestiaCore::Key_F4;
        break;
    case VK_F5:
        k = CelestiaCore::Key_F5;
        break;
    case VK_F6:
        k = CelestiaCore::Key_F6;
        break;
    }

    if (k >= 0)
    {
        if (down)
            appCore->keyDown(k);
        else
            appCore->keyUp(k);
    }
}


// Select the pixel format for a given device context
void SetDCPixelFormat(HDC hDC)
{
    int nPixelFormat;

    static PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	// Size of this structure
	1,				// Version of this structure
	PFD_DRAW_TO_WINDOW |		// Draw to Window (not to bitmap)
	PFD_SUPPORT_OPENGL |		// Support OpenGL calls in window
	PFD_DOUBLEBUFFER,		// Double buffered mode
	PFD_TYPE_RGBA,			// RGBA Color mode
	GetDeviceCaps(hDC, BITSPIXEL),	// Want the display bit depth		
	0,0,0,0,0,0,			// Not used to select mode
	0,0,				// Not used to select mode
	0,0,0,0,0,			// Not used to select mode
	16,				// Size of depth buffer
	0,				// Not used to select mode
	0,				// Not used to select mode
	PFD_MAIN_PLANE,			// Draw in main plane
	0,				// Not used to select mode
	0,0,0                           // Not used to select mode
    };

    // Choose a pixel format that best matches that described in pfd
    nPixelFormat = ChoosePixelFormat(hDC, &pfd);

    // Set the pixel format for the device context
    SetPixelFormat(hDC, nPixelFormat, &pfd);
}


static void BuildFavoritesMenu()
{
    // Add favorites to locations menu
    const FavoritesList* favorites = appCore->getFavorites();
    if (favorites != NULL)
    {
        MENUITEMINFO menuInfo;
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.fMask = MIIM_SUBMENU;
        if (GetMenuItemInfo(menuBar, 4, TRUE, &menuInfo))
        {
            HMENU locationsMenu = menuInfo.hSubMenu;

            menuInfo.cbSize = sizeof MENUITEMINFO;
            menuInfo.fMask = MIIM_TYPE | MIIM_STATE;
            menuInfo.fType = MFT_SEPARATOR;
            menuInfo.fState = MFS_UNHILITE;
            InsertMenuItem(locationsMenu, 1, TRUE, &menuInfo);

            int index = 0;
            for (FavoritesList::const_iterator iter = favorites->begin();
                 iter != favorites->end();
                 iter++, index++)
            {
                menuInfo.cbSize = sizeof MENUITEMINFO;
                menuInfo.fMask = MIIM_TYPE | MIIM_ID;
                menuInfo.fType = MFT_STRING;
                // menuInfo.fState = MFS_DEFAULT;
                menuInfo.wID = ID_LOCATIONS_FIRSTLOCATION + index;
                menuInfo.dwTypeData = const_cast<char*>((*iter)->name.c_str());
                InsertMenuItem(locationsMenu, index + 2, TRUE, &menuInfo);
            }
        }
    }
}


static void syncMenusWithRendererState()
{
    int renderFlags = appCore->getRenderer()->getRenderFlags();
    int labelMode = appCore->getRenderer()->getLabelMode();

    setMenuItemCheck(ID_RENDER_SHOWORBITS, (renderFlags & Renderer::ShowOrbits) != 0);
    setMenuItemCheck(ID_RENDER_SHOWCONSTELLATIONS,
                     (renderFlags & Renderer::ShowDiagrams) != 0);
    setMenuItemCheck(ID_RENDER_SHOWATMOSPHERES,
                     (renderFlags & Renderer::ShowCloudMaps) != 0);
    setMenuItemCheck(ID_RENDER_SHOWGALAXIES,
                     (renderFlags & Renderer::ShowGalaxies) != 0);
    setMenuItemCheck(ID_RENDER_SHOWCELESTIALSPHERE,
                     (renderFlags & Renderer::ShowCelestialSphere) != 0);

    setMenuItemCheck(ID_RENDER_SHOWPLANETLABELS,
                     (labelMode & Renderer::MajorPlanetLabels) != 0);
    setMenuItemCheck(ID_RENDER_SHOWSTARLABELS,
                     (labelMode & Renderer::StarLabels) != 0);
    setMenuItemCheck(ID_RENDER_SHOWCONSTLABELS,
                     (labelMode & Renderer::ConstellationLabels) != 0);
    setMenuItemCheck(ID_RENDER_SHOWMINORPLANETLABELS,
                     (labelMode & Renderer::MinorPlanetLabels) != 0);
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    // Say we're not ready to render yet.
    bReady = false;

    appInstance = hInstance;

    // Setup the window class.
    WNDCLASS wc;
    HWND hWnd;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) MainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CELESTIA_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = AppName;

    if (strstr(lpCmdLine, "-fullscreen"))
	fullscreen = true;
    else
	fullscreen = false;

    if (RegisterClass(&wc) == 0)
    {
	MessageBox(NULL,
                   "Failed to register the window class.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
	return FALSE;
    }

    appCore = new CelestiaCore();
    if (appCore == NULL)
    {
        MessageBox(NULL,
                   "Out of memory.", "Fatal Error",
                   MB_OK | MB_ICONERROR);
        return false;
    }

    if (!appCore->initSimulation())
    {
        return 1;
    }

    if (!fullscreen)
    {
        
        menuBar = CreateMenuBar();
        acceleratorTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));

	hWnd = CreateWindow(AppName,
			    AppName,
			    WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
			    CW_USEDEFAULT, CW_USEDEFAULT,
			    800, 600,
			    NULL,
                            menuBar,
			    hInstance,
			    NULL );
    }
    else
    {
	hWnd = CreateWindow(AppName,
			    AppName,
			    WS_POPUP,
			    0, 0,
			    800, 600,
			    NULL, NULL,
			    hInstance,
			    NULL );
    }

    if (hWnd == NULL)
    {
	MessageBox(NULL,
		   "Failed to create the application window.",
		   "Fatal Error",
		   MB_OK | MB_ICONERROR);
	return FALSE;
    }

    mainWindow = hWnd;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_DATE_CLASSES;
    InitCommonControlsEx(&icex);

    if (!appCore->initRenderer())
    {
        return 1;
    }

    timer = CreateTimer();

    BuildFavoritesMenu();
    syncMenusWithRendererState();

    appCore->setContextMenuCallback(ContextMenu);

    bReady = true;
    appCore->start((double) time(NULL) / 86400.0 +
                   (double) astro::Date(1970, 1, 1));

    MSG msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
	// Use PeekMessage() if the app is active, so we can use idle time to
	// render the scene.  Else, use GetMessage() to avoid eating CPU time.
	int bGotMsg = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);

	if (bGotMsg)
        {
	    // Translate and dispatch the message
            if (!TranslateAccelerator(hWnd, acceleratorTable, &msg))
                TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
        else
        {
	    // Get the current time, and update the time controller.
	    double lastTime = currentTime;
            currentTime = timer->getTime();
	    double dt = currentTime - lastTime;

            // Tick the simulation
            appCore->tick(dt);

            // And force a redraw
	    InvalidateRect(hWnd, NULL, FALSE);
	}
    }

    // Not ready to render anymore.
    bReady = false;

    return msg.wParam;
}


bool modifiers(WPARAM wParam, WPARAM mods)
{
    return (wParam & mods) == mods;
}


LRESULT CALLBACK MainWindowProc(HWND hWnd,
                                UINT uMsg,
                                WPARAM wParam, LPARAM lParam)
{

    static HGLRC hRC;
    static HDC hDC;

    switch(uMsg) {
    case WM_CREATE:
	hDC = GetDC(hWnd);
	if(fullscreen)
	    ChangeDisplayMode();
	SetDCPixelFormat(hDC);
	hRC = wglCreateContext(hDC);
	wglMakeCurrent(hDC, hRC);
	break;

    case WM_MOUSEMOVE:
        {
	    int x, y;
	    x = LOWORD(lParam);
	    y = HIWORD(lParam);
            if ((wParam & (MK_LBUTTON | MK_RBUTTON)) != 0)
            {
#ifdef INFINITE_MOUSE
                // A bit of mouse tweaking here . . .  we want to allow the
                // user to rotate and zoom continuously, without having to
                // pick up the mouse every time it leaves the window.  So,
                // once we start dragging, we'll hide the mouse and reset
                // its position every time it's moved.
                POINT pt;
                pt.x = lastX;
                pt.y = lastY;
                ClientToScreen(hWnd, &pt);
                if (x - lastX != 0 || y - lastY != 0)
                    SetCursorPos(pt.x, pt.y);
                if (cursorVisible)
                {
                    ShowCursor(FALSE);
                    cursorVisible = false;
                }
#else
                lastX = x;
                lastY = y;
#endif // INFINITE_MOUSE
            }

            int buttons = 0;
            if ((wParam & MK_LBUTTON) != 0)
                buttons |= CelestiaCore::LeftButton;
            if ((wParam & MK_RBUTTON) != 0)
                buttons |= CelestiaCore::RightButton;
            if ((wParam & MK_MBUTTON) != 0)
                buttons |= CelestiaCore::MiddleButton;
            if ((wParam & MK_SHIFT) != 0)
                buttons |= CelestiaCore::ShiftKey;
            if ((wParam & MK_CONTROL) != 0)
                buttons |= CelestiaCore::ControlKey;
            appCore->mouseMove(x - lastX, y - lastY, buttons);
        }
        break;

    case WM_LBUTTONDOWN:
        lastX = LOWORD(lParam);
        lastY = HIWORD(lParam);
        appCore->mouseButtonDown(LOWORD(lParam), HIWORD(lParam),
                                 CelestiaCore::LeftButton);
	break;
    case WM_RBUTTONDOWN:
        lastX = LOWORD(lParam);
        lastY = HIWORD(lParam);
        appCore->mouseButtonDown(LOWORD(lParam), HIWORD(lParam),
                                 CelestiaCore::RightButton);
        break;
    case WM_MBUTTONDOWN:
        lastX = LOWORD(lParam);
        lastY = HIWORD(lParam);
        appCore->mouseButtonDown(LOWORD(lParam), HIWORD(lParam),
                                 CelestiaCore::MiddleButton);
        break;

    case WM_LBUTTONUP:
        if (!cursorVisible)
        {
            ShowCursor(TRUE);
            cursorVisible = true;
        }
        appCore->mouseButtonUp(LOWORD(lParam), HIWORD(lParam),
                               CelestiaCore::LeftButton);
	break;

    case WM_RBUTTONUP:
        if (!cursorVisible)
        {
            ShowCursor(TRUE);
            cursorVisible = true;
        }
#if 0
        if (mouseMotion < 3)
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            Vec3f pickRay = renderer->getPickRay(LOWORD(lParam),
                                                 HIWORD(lParam));
            Selection sel = sim->pickObject(pickRay);
            if (!sel.empty())
                handlePopupMenu(hWnd, pt, sel);
        }
#endif
        appCore->mouseButtonUp(LOWORD(lParam), HIWORD(lParam),
                               CelestiaCore::RightButton);
        break;

    case WM_MOUSEWHEEL:
        appCore->mouseWheel((short) HIWORD(wParam) > 0 ? -1.0f : 1.0f);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_ESCAPE:
            appCore->charEntered('\033');
            break;
        default:
            handleKey(wParam, true);
            break;
        }
	break;

    case WM_KEYUP:
	handleKey(wParam, false);
	break;

    case WM_CHAR:
        {
            int oldRenderFlags = appCore->getRenderer()->getRenderFlags();
            int oldLabelMode = appCore->getRenderer()->getLabelMode();
            appCore->charEntered((char) wParam);
            if (appCore->getRenderer()->getRenderFlags() != oldRenderFlags ||
                appCore->getRenderer()->getLabelMode() != oldLabelMode)
            {
                syncMenusWithRendererState();
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_NAVIGATION_CENTER:
            appCore->charEntered('C');
            break;
        case ID_NAVIGATION_GOTO:
            appCore->charEntered('G');
            break;
        case ID_NAVIGATION_FOLLOW:
            appCore->charEntered('F');
            break;
        case ID_NAVIGATION_SYNCORBIT:
            appCore->charEntered('Y');
            break;
        case ID_NAVIGATION_HOME:
            appCore->charEntered('H');
            break;
        case ID_NAVIGATION_SELECT:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_FINDOBJECT), hWnd, FindObjectProc);
            break;
        case ID_NAVIGATION_SSBROWSER:
            if (ssBrowserWindow == 0)
                ssBrowserWindow = CreateDialog(appInstance, MAKEINTRESOURCE(IDD_SSBROWSER), hWnd, SolarSystemBrowserProc);
            break;

        case ID_NAVIGATION_STARBROWSER:
            if (starBrowser == NULL)
            {
                starBrowser = new StarBrowserInstance(appCore->getSimulation());
                HWND w = CreateDialog(appInstance, MAKEINTRESOURCE(IDD_STARBROWSER), hWnd, StarBrowserProc);
                if (w == 0)
                {
                    delete starBrowser;
                    starBrowser = NULL;
                }
            }
            break;

        case ID_RENDER_SHOWHUDTEXT:
            appCore->charEntered('V');
            break;
        case ID_RENDER_SHOWPLANETLABELS:
            appCore->charEntered('N');
            syncMenusWithRendererState();
            break;
        case ID_RENDER_SHOWMINORPLANETLABELS:
            appCore->charEntered('M');
            syncMenusWithRendererState();
            break;
        case ID_RENDER_SHOWSTARLABELS:
            appCore->charEntered('B');
            break;
        case ID_RENDER_SHOWCONSTLABELS:
            appCore->charEntered('=');
            syncMenusWithRendererState();
            break;

        case ID_RENDER_SHOWORBITS:
            appCore->charEntered('O');
            syncMenusWithRendererState();
            break;
        case ID_RENDER_SHOWCONSTELLATIONS:
            appCore->charEntered('/');
            syncMenusWithRendererState();
            break;
        case ID_RENDER_SHOWATMOSPHERES:
            appCore->charEntered('I');
            syncMenusWithRendererState();
            break;
        case ID_RENDER_SHOWGALAXIES:
            appCore->charEntered('U');
            syncMenusWithRendererState();
            break;
        case ID_RENDER_SHOWCELESTIALSPHERE:
            appCore->charEntered(';');
            syncMenusWithRendererState();
            break;

        case ID_RENDER_MORESTARS:
            appCore->charEntered(']');
            break;

        case ID_RENDER_FEWERSTARS:
            appCore->charEntered(']');
            break;

        case ID_RENDER_AMBIENTLIGHT_NONE:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_CHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
            appCore->getRenderer()->setAmbientLightLevel(0.0f);
            break;
        case ID_RENDER_AMBIENTLIGHT_LOW:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_CHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_UNCHECKED);
            appCore->getRenderer()->setAmbientLightLevel(0.1f);
            break;
        case ID_RENDER_AMBIENTLIGHT_MEDIUM:
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_NONE,   MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_LOW,    MF_UNCHECKED);
            CheckMenuItem(menuBar, ID_RENDER_AMBIENTLIGHT_MEDIUM, MF_CHECKED);
            appCore->getRenderer()->setAmbientLightLevel(0.25f);
            break;
#if 0
        case ID_RENDER_PERPIXEL_LIGHTING:
            if (renderer->perPixelLightingSupported())
            {
                bool enabled = !renderer->getPerPixelLighting();
                CheckMenuItem(menuBar, ID_RENDER_PERPIXEL_LIGHTING,
                              enabled ? MF_CHECKED : MF_UNCHECKED);
                renderer->setPerPixelLighting(enabled);
            }
            break;
#endif
        case ID_TIME_FASTER:
            appCore->charEntered('L');
            break;
        case ID_TIME_SLOWER:
            appCore->charEntered('K');
            break;
        case ID_TIME_REALTIME:
            appCore->charEntered('\\');
            break;

        case ID_TIME_FREEZE:
            appCore->charEntered(' ');
            break;
        case ID_TIME_REVERSE:
            appCore->charEntered('J');
            break;
        case ID_TIME_SETTIME:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_SETTIME), hWnd, SetTimeProc);
            break;

        case ID_LOCATIONS_ADDLOCATION:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_ADDLOCATION), hWnd, AddLocationProc);
            break;

        case ID_HELP_RUNDEMO:
            appCore->charEntered('D');
            break;
            
        case ID_HELP_ABOUT:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutProc);
            break;

        case ID_HELP_GLINFO:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_GLINFO), hWnd, GLInfoProc);
            break;

        case ID_HELP_LICENSE:
            DialogBox(appInstance, MAKEINTRESOURCE(IDD_LICENSE), hWnd, LicenseProc);
            break;

        case ID_INFO:
            ShowWWWInfo(appCore->getSimulation()->getSelection());
            break;

        case ID_FILE_EXIT:
	    DestroyWindow(hWnd);
            break;

        default:
            {
                const FavoritesList* favorites = appCore->getFavorites();
                if (favorites != NULL &&
                    LOWORD(wParam) - ID_LOCATIONS_FIRSTLOCATION < favorites->size())
                {
                    int whichFavorite = LOWORD(wParam) - ID_LOCATIONS_FIRSTLOCATION;
                    appCore->activateFavorite(*(*favorites)[whichFavorite]);
                }
                else if (LOWORD(wParam) >= MENU_CHOOSE_PLANET && 
                         LOWORD(wParam) < MENU_CHOOSE_PLANET + 1000)
                {
                    appCore->getSimulation()->selectPlanet(LOWORD(wParam) - MENU_CHOOSE_PLANET);
                }
            }
            break;
        }
        break;

    case WM_DESTROY:
	wglMakeCurrent(hDC, NULL);
	wglDeleteContext(hRC);
	if (fullscreen)
	    RestoreDisplayMode();
	PostQuitMessage(0);
	break;

    case WM_SIZE:
        appCore->resize(LOWORD(lParam), HIWORD(lParam));
	break;

    case WM_PAINT:
	if (bReady)
        {
            appCore->draw();
	    SwapBuffers(hDC);
	    ValidateRect(hWnd, NULL);
	}
	break;

    default:
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}
