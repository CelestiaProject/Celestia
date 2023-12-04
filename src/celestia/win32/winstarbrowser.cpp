// winstarbrowser.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Original version:
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Star browser tool for Windows.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winstarbrowser.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <string_view>

#include <fmt/format.h>
#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <celengine/observer.h>
#include <celengine/selection.h>
#include <celengine/simulation.h>
#include <celengine/star.h>
#include <celengine/stardb.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include <celmath/mathlib.h>
#include <celutil/gettext.h>

#include <commctrl.h>

#include "res/resource.h"
#include "tstring.h"
#include "winuiutils.h"

namespace celestia::win32
{

namespace
{

bool
InitStarBrowserColumns(HWND listView)
{
    constexpr std::size_t numColumns = 5;
    std::array<tstring, numColumns> headers
    {
        UTF8ToTString(_("Name")),
        UTF8ToTString(_("Distance (ly)")),
        UTF8ToTString(_("App. mag")),
        UTF8ToTString(_("Abs. mag")),
        UTF8ToTString(_("Type")),
    };

    constexpr std::array<int, numColumns> widths
    {
        100, 115, 65, 65, 60
    };

    constexpr std::array<int, 5> formats
    {
        LVCFMT_LEFT, LVCFMT_RIGHT, LVCFMT_RIGHT, LVCFMT_RIGHT, LVCFMT_LEFT
    };

    for (std::size_t i = 0; i < numColumns; ++i)
    {
        LVCOLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = formats[i];
        lvc.cx = DpToPixels(widths[i], listView);
        lvc.pszText = const_cast<TCHAR*>(headers[i].c_str());
        lvc.iSubItem = static_cast<int>(i);
        if (ListView_InsertColumn(listView, static_cast<int>(i), &lvc) == -1)
            return false;
    }

    return true;
}

bool
InitStarBrowserLVItems(HWND listView, std::vector<engine::StarBrowserRecord>& records)
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

struct StarBrowserSortInfo
{
    const StarDatabase* stardb;
    bool reverse;
};

int CALLBACK
StarBrowserCompareName(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    int result = CompareUTF8Localized(info->stardb->getStarName(*record0->star, true),
                                      info->stardb->getStarName(*record1->star, true));

    return info->reverse ? -result : result;
}

int CALLBACK
StarBrowserCompareDistance(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = static_cast<int>(math::sign(record0->distance - record1->distance));
    return info->reverse ? -result : result;
}

int CALLBACK
StarBrowserCompareAppMag(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = static_cast<int>(math::sign(record0->appMag - record1->appMag));
    return info->reverse ? -result : result;
}

int CALLBACK
StarBrowserCompareAbsMag(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = static_cast<int>(math::sign(record0->star->getAbsoluteMagnitude() - record1->star->getAbsoluteMagnitude()));
    return info->reverse ? -result : result;
}

int CALLBACK
StarBrowserCompareSpectralType(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto record0 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam0);
    auto record1 = reinterpret_cast<const engine::StarBrowserRecord*>(lParam1);
    auto info = reinterpret_cast<const StarBrowserSortInfo*>(lParamInfo);

    auto result = std::strcmp(record0->star->getSpectralType(), record1->star->getSpectralType());
    return info->reverse ? -result : result;
}

constexpr std::array<PFNLVCOMPARE, 5> compareFuncs
{
    &StarBrowserCompareName,
    &StarBrowserCompareDistance,
    &StarBrowserCompareAppMag,
    &StarBrowserCompareAbsMag,
    &StarBrowserCompareSpectralType,
};

void StarBrowserDisplayItem(NMLVDISPINFO* nm, StarBrowser* browser)
{
    double tdb = browser->appCore->getSimulation()->getTime();

    auto record = reinterpret_cast<const engine::StarBrowserRecord*>(nm->item.lParam);
    if (record == nullptr)
    {
        nm->item.pszText[0] = TEXT('\0');
        return;
    }

    switch (nm->item.iSubItem)
    {
    case 0:
        {
            Universe* u = browser->appCore->getSimulation()->getUniverse();
            int length = UTF8ToTChar(u->getStarCatalog()->getStarName(*record->star, true), nm->item.pszText, nm->item.cchTextMax - 1);
            nm->item.pszText[length] = TEXT('\0');
            break;
        }
    case 1:
        {
            auto out = fmt::format_to_n(nm->item.pszText, nm->item.cchTextMax - 1, TEXT("{:.4g}"), record->distance).out;
            *out = TEXT('\0');
            break;
        }
    case 2:
        {
            auto out = fmt::format_to_n(nm->item.pszText, nm->item.cchTextMax - 1, TEXT("{:.2f}"), record->appMag).out;
            *out = TEXT('\0');
            break;
        }
    case 3:
        {
            auto out = fmt::format_to_n(nm->item.pszText, nm->item.cchTextMax - 1, TEXT("{:.2f}"), record->star->getAbsoluteMagnitude()).out;
            *out = TEXT('\0');
            break;
        }
    case 4:
        {
            int length = UTF8ToTChar(record->star->getSpectralType(), nm->item.pszText, nm->item.cchTextMax - 1);
            nm->item.pszText[length] = TEXT('\0');
            break;
        }
    }
}

void
RefreshItems(HWND hDlg, StarBrowser* browser)
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

INT_PTR APIENTRY
StarBrowserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
            SetDlgItemInt(hDlg, IDC_MAXSTARS_EDIT, browser->starBrowser.size(), FALSE);
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
                UINT nNewStars = GetDlgItemInt(hDlg, IDC_MAXSTARS_EDIT, nullptr, FALSE);

                // Check if new value is different from old. Don't want to
                // cause a refresh to occur if not necessary.
                if (nNewStars != browser->starBrowser.size())
                {
                    LRESULT minRange = SendDlgItemMessage(hDlg, IDC_MAXSTARS_SLIDER, TBM_GETRANGEMIN, 0, 0);
                    LRESULT maxRange = SendDlgItemMessage(hDlg, IDC_MAXSTARS_SLIDER, TBM_GETRANGEMAX, 0, 0);
                    bool outOfRange = false;
                    if (nNewStars < minRange)
                    {
                        outOfRange = true;
                        nNewStars = minRange;
                    }
                    else if (nNewStars > maxRange)
                    {
                        outOfRange = true;
                        nNewStars = maxRange;
                    }

                    // If new value has been adjusted from what was entered,
                    // reflect new value back in edit control.
                    if (outOfRange)
                        SetDlgItemInt(hDlg, IDC_MAXSTARS_EDIT, nNewStars, FALSE);

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
                    StarBrowserDisplayItem(reinterpret_cast<NMLVDISPINFO*>(lParam), browser);
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
            auto sliderPos = sbValue == SB_THUMBTRACK
                ? static_cast<UINT>(HIWORD(wParam))
                : static_cast<UINT>(SendMessage(GetDlgItem(hDlg, IDC_MAXSTARS_SLIDER), TBM_GETPOS, 0, 0));

            SetDlgItemInt(hDlg, IDC_MAXSTARS_EDIT, sliderPos, FALSE);

            if (sbValue != SB_THUMBTRACK)
            {
                browser->starBrowser.setSize(HIWORD(wParam));
                RefreshItems(hDlg, browser);
            }
        }
        break;
    }

    return FALSE;
}

} // end unnamed namespace

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
                             &StarBrowserProc,
                             reinterpret_cast<LONG_PTR>(this));
}

StarBrowser::~StarBrowser()
{
    SetWindowLongPtr(hwnd, DWLP_USER, 0);
}

} // end namespace celestia::win32
