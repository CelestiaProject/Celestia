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

#include "winstarbrowser.h"

#include <string>
#include <algorithm>
#include <set>
#include <windows.h>
#include <commctrl.h>
#include <cstring>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include <celutil/winutil.h>
#include "winuiutils.h"
#include <celmath/mathlib.h>
#include "res/resource.h"
#include "winuiutils.h"

#include <fmt/format.h>

using namespace Eigen;
using namespace std;
using namespace celestia::win32;

namespace astro = celestia::astro;
namespace engine = celestia::engine;
namespace math = celestia::math;
namespace util = celestia::util;

bool InitStarBrowserColumns(HWND listView)
{
    LVCOLUMN lvc;
    LVCOLUMN columns[5];

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = DpToPixels(60, listView);
    lvc.pszText = const_cast<char*>("");

    int nColumns = sizeof(columns) / sizeof(columns[0]);
    int i;

    for (i = 0; i < nColumns; i++)
        columns[i] = lvc;

    string header0 = util::UTF8ToCurrentCP(_("Name"));
    string header1 = util::UTF8ToCurrentCP(_("Distance (ly)"));
    string header2 = util::UTF8ToCurrentCP(_("App. mag"));
    string header3 = util::UTF8ToCurrentCP(_("Abs. mag"));
    string header4 = util::UTF8ToCurrentCP(_("Type"));

    columns[0].pszText = const_cast<char*>(header0.c_str());
    columns[0].cx = DpToPixels(100, listView);
    columns[1].pszText = const_cast<char*>(header1.c_str());
    columns[1].fmt = LVCFMT_RIGHT;
    columns[1].cx = DpToPixels(115, listView);
    columns[2].pszText = const_cast<char*>(header2.c_str());
    columns[2].fmt = LVCFMT_RIGHT;
    columns[2].cx = DpToPixels(65, listView);
    columns[3].pszText = const_cast<char*>(header3.c_str());
    columns[3].fmt = LVCFMT_RIGHT;
    columns[3].cx = DpToPixels(65, listView);
    columns[4].pszText = const_cast<char*>(header4.c_str());

    for (i = 0; i < nColumns; i++)
    {
        columns[i].iSubItem = i;
        if (ListView_InsertColumn(listView, i, &columns[i]) == -1)
            return false;
    }

    return true;
}

bool InitStarBrowserLVItems(HWND listView, std::vector<engine::StarBrowserRecord>& records)
{
    LVITEM lvi;

    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;

    int index = 0;
    for (const auto& record : records)
    {
        lvi.iItem = index;
        lvi.iSubItem = 0;
        lvi.lParam = (LPARAM) &record;
        ListView_InsertItem(listView, &lvi);
        ++index;
    }

    return true;
}

bool InitStarBrowserItems(HWND listView, StarBrowser* browser)
{
    const Observer& obs = browser->appCore->getSimulation()->getObserver();
    browser->starBrowser.setPosition(obs.getPosition());
    browser->starBrowser.setTime(obs.getTime());
    browser->starBrowser.populate(browser->stars);
    bool succeeded = InitStarBrowserLVItems(listView, browser->stars);
    browser->sortColumn = -1;
    browser->sortColumnReverse = false;
    return succeeded;
}

// Crud used for the list item display callbacks
static string starNameString;
static string starNameString2;
static char callbackScratch[256];

struct StarBrowserSortInfo
{
    const StarDatabase* stardb;
    bool reverse;
};

template<std::size_t SIZE, typename Allocator>
static void
nameToWide(std::string_view name, fmt::basic_memory_buffer<wchar_t, SIZE, Allocator>& buffer)
{
    auto nameLength = static_cast<int>(name.size());
    int length = MultiByteToWideChar(CP_UTF8, 0, name.data(), nameLength, nullptr, 0);
    if (length <= 0)
        return;

    buffer.resize(static_cast<std::size_t>(length));
    MultiByteToWideChar(CP_UTF8, 0, name.data(), nameLength, buffer.data(), buffer.size());
}

static int CALLBACK StarBrowserCompareName(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    fmt::basic_memory_buffer<wchar_t, 260> wname0;
    nameToWide(info->stardb->getStarName(*record0->star, true), wname0);
    fmt::basic_memory_buffer<wchar_t, 260> wname1;
    nameToWide(info->stardb->getStarName(*record1->star, true), wname1);

    auto result = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_LINGUISTIC_CASING,
                                  wname0.data(), static_cast<int>(wname0.size()),
                                  wname1.data(), static_cast<int>(wname1.size()),
                                  nullptr, nullptr, 0);
    // MS: To maintain the C runtime convention of comparing strings, the
    // value 2 can be subtracted from a nonzero return value
    if (result != 0)
        result -= 2;

    return info->reverse ? -result : result;
}

static int CALLBACK StarBrowserCompareDistance(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = static_cast<int>(math::sign(record0->distance - record1->distance));
    return info->reverse ? -result : result;
}

static int CALLBACK StarBrowserCompareAppMag(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = static_cast<int>(math::sign(record0->appMag - record1->appMag));
    return info->reverse ? -result : result;
}

static int CALLBACK StarBrowserCompareAbsMag(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = static_cast<int>(math::sign(record0->star->getAbsoluteMagnitude() - record1->star->getAbsoluteMagnitude()));
    return info->reverse ? -result : result;
}

static int CALLBACK StarBrowserCompareSpectralType(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = std::strcmp(record0->star->getSpectralType(), record1->star->getSpectralType());
    return info->reverse ? -result : result;
}

static constexpr std::array<PFNLVCOMPARE, 5> compareFuncs
{
    &StarBrowserCompareName,
    &StarBrowserCompareDistance,
    &StarBrowserCompareAppMag,
    &StarBrowserCompareAbsMag,
    &StarBrowserCompareSpectralType,
};

void StarBrowserDisplayItem(LPNMLVDISPINFOA nm, StarBrowser* browser)
{
    double tdb = browser->appCore->getSimulation()->getTime();

    auto record = reinterpret_cast<const engine::StarBrowserRecord*>(nm->item.lParam);
    if (record == nullptr)
    {
        nm->item.pszText = const_cast<char*>("");
        return;
    }

    switch (nm->item.iSubItem)
    {
    case 0:
        {
            Universe* u = browser->appCore->getSimulation()->getUniverse();
            starNameString = util::UTF8ToCurrentCP(u->getStarCatalog()->getStarName(*record->star, true));
            nm->item.pszText = const_cast<char*>(starNameString.c_str());
        }
        break;

    case 1:
        std::snprintf(callbackScratch, sizeof(callbackScratch) - 1, "%.4g", record->distance);
        callbackScratch[sizeof(callbackScratch) - 1] = '\0';
        nm->item.pszText = callbackScratch;
        break;

    case 2:
        std::snprintf(callbackScratch, sizeof(callbackScratch) - 1, "%.2f", record->appMag);
        callbackScratch[sizeof(callbackScratch) - 1] = '\0';
        nm->item.pszText = callbackScratch;
        break;

    case 3:
        std::snprintf(callbackScratch, sizeof(callbackScratch) - 1, "%.2f", record->star->getAbsoluteMagnitude());
        callbackScratch[sizeof(callbackScratch) - 1] = '\0';
        nm->item.pszText = callbackScratch;
        break;

    case 4:
        std::strncpy(callbackScratch, record->star->getSpectralType(), sizeof(callbackScratch)); /* Flawfinder: ignore */
        callbackScratch[sizeof(callbackScratch) - 1] = '\0';
        nm->item.pszText = callbackScratch;
        break;
    }
}

void RefreshItems(HWND hDlg, StarBrowser* browser)
{
    SetMouseCursor(IDC_WAIT);

    HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
    if (hwnd != 0)
    {
        ListView_DeleteAllItems(hwnd);
        InitStarBrowserItems(hwnd, browser);
    }

    SetMouseCursor(IDC_ARROW);
}

BOOL APIENTRY StarBrowserProc(HWND hDlg,
                              UINT message,
                              WPARAM wParam,
                              LPARAM lParam)
{
    StarBrowser* browser = reinterpret_cast<StarBrowser*>(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            StarBrowser* browser = reinterpret_cast<StarBrowser*>(lParam);
            if (browser == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
            InitStarBrowserColumns(hwnd);
            InitStarBrowserItems(hwnd, browser);
            CheckRadioButton(hDlg, IDC_RADIO_NEAREST, IDC_RADIO_WITHPLANETS, IDC_RADIO_NEAREST);

            //Initialize Max Stars edit box
            char val[16];
            hwnd = GetDlgItem(hDlg, IDC_MAXSTARS_EDIT);
            sprintf(val, "%d", browser->starBrowser.size());
            SetWindowText(hwnd, val);
            SendMessage(hwnd, EM_LIMITTEXT, 3, 0);

            //Initialize Max Stars Slider control
            SendDlgItemMessage(hDlg, IDC_MAXSTARS_SLIDER, TBM_SETRANGE,
                (WPARAM)TRUE, (LPARAM)MAKELONG(engine::StarBrowser::MinListStars, engine::StarBrowser::MaxListStars));
            SendDlgItemMessage(hDlg, IDC_MAXSTARS_SLIDER, TBM_SETPOS,
                (WPARAM)TRUE, (LPARAM)browser->starBrowser.size());

            return(TRUE);
        }

    case WM_DESTROY:
        if (browser != NULL && browser->parent != NULL)
        {
            SendMessage(browser->parent, WM_COMMAND, IDCLOSE,
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
                SendMessage(browser->parent, WM_COMMAND, IDCLOSE,
                            reinterpret_cast<LPARAM>(browser));
            }
            EndDialog(hDlg, 0);
            return TRUE;

        case IDC_BUTTON_CENTER:
            browser->appCore->charEntered('c');
            break;

        case IDC_BUTTON_GOTO:
            browser->appCore->charEntered('G');
            break;

        case IDC_RADIO_BRIGHTEST:
            browser->starBrowser.setComparison(engine::StarBrowser::Comparison::ApparentMagnitude);
            browser->starBrowser.setFilter(engine::StarBrowser::Filter::Visible);
            RefreshItems(hDlg, browser);
            break;

        case IDC_RADIO_NEAREST:
            browser->starBrowser.setComparison(engine::StarBrowser::Comparison::Nearest);
            browser->starBrowser.setFilter(engine::StarBrowser::Filter::Visible);
            RefreshItems(hDlg, browser);
            break;

        case IDC_RADIO_WITHPLANETS:
            browser->starBrowser.setComparison(engine::StarBrowser::Comparison::Nearest);
            browser->starBrowser.setFilter(engine::StarBrowser::Filter::WithPlanets);
            RefreshItems(hDlg, browser);
            break;

        case IDC_BUTTON_REFRESH:
            RefreshItems(hDlg, browser);
            break;

        case IDC_MAXSTARS_EDIT:
            // TODO: browser != NULL check should be in a lot more places
            if (HIWORD(wParam) == EN_KILLFOCUS && browser != NULL)
            {
                char val[16];
                DWORD nNewStars;
                DWORD minRange, maxRange;
                GetWindowText((HWND) lParam, val, sizeof(val));
                nNewStars = atoi(val);

                // Check if new value is different from old. Don't want to
                // cause a refresh to occur if not necessary.
                if (nNewStars != browser->starBrowser.size())
                {
                    minRange = SendDlgItemMessage(hDlg, IDC_MAXSTARS_SLIDER, TBM_GETRANGEMIN, 0, 0);
                    maxRange = SendDlgItemMessage(hDlg, IDC_MAXSTARS_SLIDER, TBM_GETRANGEMAX, 0, 0);
                    if (nNewStars < minRange)
                        nNewStars = minRange;
                    else if (nNewStars > maxRange)
                        nNewStars = maxRange;

                    // If new value has been adjusted from what was entered,
                    // reflect new value back in edit control.
                    if (atoi(val) != nNewStars)
                    {
                        sprintf(val, "%d", nNewStars);
                        SetWindowText((HWND)lParam, val);
                    }

                    // Recheck value if different from original.
                    if (nNewStars != browser->starBrowser.size())
                    {
                        browser->starBrowser.setSize(nNewStars);
                        SendDlgItemMessage(hDlg,
                                           IDC_MAXSTARS_SLIDER,
                                           TBM_SETPOS,
                                           (WPARAM) TRUE,
                                           (LPARAM) browser->starBrowser.size());
                        RefreshItems(hDlg, browser);
                    }
                }
            }
            break;
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR) lParam;

            if (hdr->idFrom == IDC_STARBROWSER_LIST && browser != NULL)
            {
                switch(hdr->code)
                {
                case LVN_GETDISPINFO:
                    StarBrowserDisplayItem((LPNMLVDISPINFOA) lParam, browser);
                    break;
                case LVN_ITEMCHANGED:
                    {
                        LPNMLISTVIEW nm = (LPNMLISTVIEW) lParam;
                        if ((nm->uNewState & LVIS_SELECTED) != 0)
                        {
                            Simulation* sim = browser->appCore->getSimulation();
                            auto record = reinterpret_cast<const engine::StarBrowserRecord*>(nm->lParam);
                            if (record != nullptr)
                                sim->setSelection(Selection(const_cast<Star*>(record->star)));
                        }
                        break;
                    }
                case LVN_COLUMNCLICK:
                    {
                        HWND hwnd = GetDlgItem(hDlg, IDC_STARBROWSER_LIST);
                        if (hwnd != 0)
                        {
                            LPNMLISTVIEW nm = (LPNMLISTVIEW) lParam;
                            if (auto subItem = nm->iSubItem;
                                subItem >= 0 && subItem < static_cast<int>(compareFuncs.size()))
                            {
                                if (subItem == browser->sortColumn)
                                    browser->sortColumnReverse = !browser->sortColumnReverse;
                                else
                                    browser->sortColumnReverse = false;
                                browser->sortColumn = subItem;
                                StarBrowserSortInfo info{
                                    browser->starBrowser.universe()->getStarCatalog(),
                                    browser->sortColumnReverse,
                                };
                                ListView_SortItems(hwnd, compareFuncs[subItem], reinterpret_cast<LPARAM>(&info));
                            }
                        }
                    }

                }
            }
        }
        break;

    case WM_HSCROLL:
        {
            WORD sbValue = LOWORD(wParam);
            switch(sbValue)
            {
                case SB_THUMBTRACK:
                {
                    char val[16];
                    HWND hwnd = GetDlgItem(hDlg, IDC_MAXSTARS_EDIT);
                    sprintf(val, "%d", HIWORD(wParam));
                    SetWindowText(hwnd, val);
                    break;
                }
                case SB_THUMBPOSITION:
                {
                    browser->starBrowser.setSize(HIWORD(wParam));
                    RefreshItems(hDlg, browser);
                    break;
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
    parent(_parent),
    starBrowser(_appCore->getSimulation()->getUniverse())
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_STARBROWSER),
                             parent,
                             (DLGPROC)StarBrowserProc,
                             reinterpret_cast<LONG_PTR>(this));
}


StarBrowser::~StarBrowser()
{
    SetWindowLongPtr(hwnd, DWLP_USER, 0);
}
