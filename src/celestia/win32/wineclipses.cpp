// wineclipses.cpp by Kendrix <kendrix@wanadoo.fr>
// modified by Chris Laurel
// modified by Celestia Development Team
//
// Compute Solar Eclipses for our Solar System planets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "wineclipses.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <fmt/format.h>
#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <celastro/date.h>
#include <celengine/body.h>
#include <celengine/star.h>
#include <celengine/stardb.h>
#include <celengine/univcoord.h>
#include <celengine/universe.h>
#include <celestia/celestiacore.h>
#include <celmath/geomutil.h>
#include <celmath/mathlib.h>
#include <celutil/array_view.h>
#include <celutil/gettext.h>

#include <commctrl.h>

#include "res/resource.h"
#include "datetimehelpers.h"
#include "winuiutils.h"

using namespace std::string_view_literals;

namespace celestia::win32
{

namespace
{

constexpr auto TargetBodyCount = static_cast<std::size_t>(EclipseFinderDialog::TargetBody::Count_);
constexpr std::array<std::string_view, TargetBodyCount> TargetNames
{
    "Earth"sv,
    "Jupiter"sv,
    "Saturn"sv,
    "Uranus"sv,
    "Neptune"sv,
    "Pluto"sv,
};

bool
InitEclipseFinderColumns(HWND listView)
{
    constexpr std::size_t numColumns = 5;

    std::array<tstring, numColumns> headers
    {
        UTF8ToTString(_("Body")),
        UTF8ToTString(_("Occulter")),
        UTF8ToTString(_("Date")),
        UTF8ToTString(_("Start")),
        UTF8ToTString(_("Duration")),
    };

    constexpr std::array<int, numColumns> widths
    {
        65, 65, 80, 55, 135
    };

    for (std::size_t i = 0; i < numColumns; ++i)
    {
        LVCOLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.fmt = LVCFMT_CENTER;
        lvc.cx = DpToPixels(widths[i], listView);
        lvc.pszText = const_cast<TCHAR*>(headers[i].c_str());
        lvc.iSubItem = static_cast<int>(i);
        if (ListView_InsertColumn(listView, static_cast<int>(i), &lvc) == -1)
            return false;
    }

    return true;
}

bool
InitEclipseFinderItems(HWND listView, const std::vector<Eclipse>& eclipses)
{
    LVITEM lvi;

    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;

    for (unsigned int i = 0; i < eclipses.size(); i++)
    {
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.lParam = reinterpret_cast<LPARAM>(&(eclipses[i]));
        ListView_InsertItem(listView, &lvi);
    }

    return true;
}

void
EclipseFinderDisplayItem(NMLVDISPINFO* nm, util::array_view<tstring> monthNames)
{
    auto eclipse = reinterpret_cast<const Eclipse*>(nm->item.lParam);
    if (eclipse == NULL)
    {
        nm->item.pszText[0] = TEXT('\0');
        return;
    }

    switch (nm->item.iSubItem)
    {
    case 0:
        {
            int length = UTF8ToTChar(eclipse->receiver->getName(true), nm->item.pszText, nm->item.cchTextMax - 1);
            nm->item.pszText[length] = TEXT('\0');
            break;
        }
    case 1:
        {
            int length = UTF8ToTChar(eclipse->occulter->getName(true), nm->item.pszText, nm->item.cchTextMax - 1);
            nm->item.pszText[length] = TEXT('\0');
            break;
        }
    case 2:
        {
            astro::Date startDate(eclipse->startTime);
            auto out = fmt::format_to_n(nm->item.pszText, nm->item.cchTextMax - 1, TEXT("{:>2d} {} {:>4d}"),
                                        startDate.day, monthNames[startDate.month - 1], startDate.year).out;
            *out = TEXT('\0');
            break;
        }
    case 3:
        {
            astro::Date startDate(eclipse->startTime);
            auto out = fmt::format_to_n(nm->item.pszText, nm->item.cchTextMax - 1, TEXT("{:02d}:{:02d}"),
                                        startDate.hour, startDate.minute).out;
            *out = TEXT('\0');
            break;
        }
    case 4:
        {
            int minutes = static_cast<int>((eclipse->endTime - eclipse->startTime) * 24 * 60);
            auto out = fmt::format_to_n(nm->item.pszText, nm->item.cchTextMax - 1, TEXT("{:02d}:{:02d}"),
                                        minutes / 60, minutes % 60).out;
            *out = TEXT('\0');
            break;
        }
    default:
        *(nm->item.pszText) = TEXT('\0');
        break;
    }
}

void
InitDateControls(HWND hDlg, const astro::Date& newTime, SYSTEMTIME& fromTime, SYSTEMTIME& toTime)
{
    fromTime.wYear = newTime.year - 1;
    fromTime.wMonth = newTime.month;
    fromTime.wDay = newTime.day;
    fromTime.wDayOfWeek = (static_cast<int>(static_cast<double>(newTime) + 0.5) + 1) % 7;
    fromTime.wHour = 0;
    fromTime.wMinute = 0;
    fromTime.wSecond = 0;
    fromTime.wMilliseconds = 0;

    toTime = fromTime;
    toTime.wYear += 2;

    HWND dateItem = GetDlgItem(hDlg, IDC_DATEFROM);
    if (dateItem != NULL)
    {
        DateTime_SetFormat(dateItem, TEXT("dd' 'MMM' 'yyy"));
        DateTime_SetSystemtime(dateItem, GDT_VALID, &fromTime);
    }
    dateItem = GetDlgItem(hDlg, IDC_DATETO);
    if (dateItem != NULL)
    {
        DateTime_SetFormat(dateItem, TEXT("dd' 'MMM' 'yyy"));
        DateTime_SetSystemtime(dateItem, GDT_VALID, &toTime);
    }
}

struct EclipseFinderSortInfo
{
    int  iSubItem;
    bool reverse;
};

int CALLBACK
EclipseFinderCompareBody(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto eclipse0 = reinterpret_cast<const Eclipse*>(lParam0);
    auto eclipse1 = reinterpret_cast<const Eclipse*>(lParam1);
    auto sortInfo = reinterpret_cast<const EclipseFinderSortInfo*>(lParamInfo);

    int result = 0;
    switch (sortInfo->iSubItem)
    {
    case 0:
        result = CompareUTF8Localized(eclipse0->receiver->getName(true),
                                      eclipse1->receiver->getName(true));
        break;

    case 1:
        result = CompareUTF8Localized(eclipse0->occulter->getName(true),
                                      eclipse1->receiver->getName(true));
        break;

    default:
        break;
    }

    return sortInfo->reverse ? -result : result;
}

int CALLBACK
EclipseFinderCompareDuration(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto eclipse0 = reinterpret_cast<const Eclipse*>(lParam0);
    auto eclipse1 = reinterpret_cast<const Eclipse*>(lParam1);
    auto sortInfo = reinterpret_cast<const EclipseFinderSortInfo*>(lParamInfo);

    double duration0 = eclipse0->endTime - eclipse0->startTime;
    double duration1 = eclipse1->endTime - eclipse1->startTime;
    auto result = static_cast<int>(math::sign(duration0 - duration1));
    return sortInfo->reverse ? -result : result;
}

int CALLBACK
EclipseFinderCompareStartTime(LPARAM lParam0, LPARAM lParam1, LPARAM lParamInfo)
{
    auto eclipse0 = reinterpret_cast<const Eclipse*>(lParam0);
    auto eclipse1 = reinterpret_cast<const Eclipse*>(lParam1);
    auto sortInfo = reinterpret_cast<const EclipseFinderSortInfo*>(lParamInfo);

    auto result = static_cast<int>(math::sign(eclipse0->startTime - eclipse1->startTime));
    return sortInfo->reverse ? -result : result;
}

constexpr std::array<PFNLVCOMPARE, 5> compareFuncs
{
    &EclipseFinderCompareBody,
    &EclipseFinderCompareBody,
    &EclipseFinderCompareStartTime,
    &EclipseFinderCompareStartTime,
    &EclipseFinderCompareDuration,
};

LRESULT CALLBACK
EclipseListViewProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch(message)
    {
    case WM_LBUTTONDBLCLK:
        {
            LVHITTESTINFO lvHit;
            lvHit.pt.x = LOWORD(lParam);
            lvHit.pt.y = HIWORD(lParam);
            int listIndex = ListView_HitTest(hWnd, &lvHit);
            if (listIndex >= 0)
            {
                SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(IDSETDATEANDGO, 0), 0);
            }
        }
        break;
    }

    return DefSubclassProc(hWnd, message, wParam, lParam);
}

INT_PTR APIENTRY
EclipseFinderProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_INITDIALOG)
    {
        EclipseFinderDialog* efd = reinterpret_cast<EclipseFinderDialog*>(lParam);
        if (efd == NULL)
            return EndDialog(hDlg, 0);

        efd->monthNames = GetLocalizedMonthNames();

        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        HWND hwnd = GetDlgItem(hDlg, IDC_ECLIPSES_LIST);
        InitEclipseFinderColumns(hwnd);
        SendDlgItemMessage(hDlg, IDC_ECLIPSES_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

        CheckRadioButton(hDlg, IDC_SOLARECLIPSE, IDC_LUNARECLIPSE, IDC_SOLARECLIPSE);
        efd->type = Eclipse::Solar;

        constexpr std::size_t numItems = 6;
        std::array<tstring, numItems> items
        {
            UTF8ToTString(_("Earth")),
            UTF8ToTString(_("Jupiter")),
            UTF8ToTString(_("Saturn")),
            UTF8ToTString(_("Uranus")),
            UTF8ToTString(_("Neptune")),
            UTF8ToTString(_("Pluto")),
        };

        for (std::size_t i = 0; i < numItems; ++i)
        {
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(items[i].c_str()));
        }

        SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_SETCURSEL, 0, 0);
        efd->targetBody = EclipseFinderDialog::TargetBody::Earth;

        InitDateControls(hDlg, astro::Date(efd->appCore->getSimulation()->getTime()), efd->fromTime, efd->toTime);

        // Subclass the ListView to intercept WM_LBUTTONUP messages
        SetWindowSubclass(hwnd, EclipseListViewProc, 0, 0);
        return TRUE;
    }

    auto eclipseFinderDlg = reinterpret_cast<EclipseFinderDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
    case WM_DESTROY:
        if (eclipseFinderDlg != NULL && eclipseFinderDlg->parent != NULL)
        {
            SendMessage(eclipseFinderDlg->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(eclipseFinderDlg));
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCOMPUTE:
            {
                HWND hwnd = GetDlgItem(hDlg, IDC_ECLIPSES_LIST);
                ListView_DeleteAllItems(hwnd);
                eclipseFinderDlg->eclipseList.clear();
                SetMouseCursor(IDC_WAIT);

                astro::Date from(eclipseFinderDlg->fromTime.wYear,
                                 eclipseFinderDlg->fromTime.wMonth,
                                 eclipseFinderDlg->fromTime.wDay);
                astro::Date to(eclipseFinderDlg->toTime.wYear,
                               eclipseFinderDlg->toTime.wMonth,
                               eclipseFinderDlg->toTime.wDay);

                const SolarSystemCatalog* systems = eclipseFinderDlg->appCore->getSimulation()->getUniverse()->getSolarSystemCatalog();
                if (auto solarSystem = systems->find(0); solarSystem != systems->end())
                {
                    const PlanetarySystem* system = solarSystem->second->getPlanets();
                    const Body* planet = system->find(TargetNames[static_cast<std::size_t>(eclipseFinderDlg->targetBody)]);
                    if (planet != nullptr)
                    {
                        EclipseFinder ef(planet);
                        ef.findEclipses((double)from, (double)to, eclipseFinderDlg->type, eclipseFinderDlg->eclipseList);
                    }
                }

                InitEclipseFinderItems(hwnd, eclipseFinderDlg->eclipseList);
                eclipseFinderDlg->sortColumn = -1;
                SetMouseCursor(IDC_ARROW);
                break;
            }

        case IDCLOSE:
            {
                if (eclipseFinderDlg != NULL && eclipseFinderDlg->parent != NULL)
                {
                    SendMessage(eclipseFinderDlg->parent, WM_COMMAND, IDCLOSE,
                                reinterpret_cast<LPARAM>(eclipseFinderDlg));
                }
                EndDialog(hDlg, 0);
                break;
            }

        case IDSETDATEANDGO:
            if (eclipseFinderDlg->BodytoSet_)
            {
                Simulation* sim = eclipseFinderDlg->appCore->getSimulation();
                sim->setTime(eclipseFinderDlg->TimetoSet_);
                Selection target(eclipseFinderDlg->BodytoSet_);
                Selection ref(eclipseFinderDlg->BodytoSet_->getSystem()->getStar());
                // Use the phase lock coordinate system to set a position
                // on the line between the sun and the body where the eclipse
                // is occurring.
                sim->setFrame(ObserverFrame::CoordinateSystem::PhaseLock, target, ref);
                sim->update(0.0);

                double distance = target.radius() * 4.0;
                sim->gotoLocation(UniversalCoord::Zero().offsetKm(Eigen::Vector3d::UnitX() * distance),
                                  math::YRot90Conjugate<double> * math::XRot90Conjugate<double>,
                                  2.5);
            }
            break;
        case IDC_SOLARECLIPSE:
                eclipseFinderDlg->type = Eclipse::Solar;
            break;
        case IDC_LUNARECLIPSE:
                eclipseFinderDlg->type = Eclipse::Lunar;
            break;
        case IDC_ECLIPSETARGET:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                LRESULT index = SendMessage(reinterpret_cast<HWND>(lParam), CB_GETCURSEL, 0, 0);
                if (index >= 0 && index < static_cast<LRESULT>(TargetBodyCount))
                    eclipseFinderDlg->targetBody = static_cast<EclipseFinderDialog::TargetBody>(index);
            }
        }
        return TRUE;

    case WM_NOTIFY:
        {
            auto hdr = reinterpret_cast<LPNMHDR>(lParam);

            if(hdr->idFrom == IDC_ECLIPSES_LIST)
            {
                switch(hdr->code)
                {
                case LVN_GETDISPINFO:
                    EclipseFinderDisplayItem(reinterpret_cast<NMLVDISPINFO*>(lParam), eclipseFinderDlg->monthNames);
                    break;

                case LVN_ITEMCHANGED:
                    if (auto nm = reinterpret_cast<LPNMLISTVIEW>(lParam); (nm->uNewState & LVIS_SELECTED) != 0)
                    {
                        auto eclipse = reinterpret_cast<const Eclipse*>(nm->lParam);
                        if (eclipse != NULL)
                        {
                            eclipseFinderDlg->TimetoSet_ =
                                (eclipse->startTime + eclipse->endTime) / 2.0f;
                            eclipseFinderDlg->BodytoSet_ = eclipseFinderDlg->type == Eclipse::Solar
                                                         ? eclipse->receiver
                                                         : eclipse->occulter;
                        }
                    }
                    break;

                case LVN_COLUMNCLICK:
                    if (HWND hwnd = GetDlgItem(hDlg, IDC_ECLIPSES_LIST); hwnd != 0)
                    {
                        auto nm = reinterpret_cast<LPNMLISTVIEW>(lParam);
                        if (nm->iSubItem == eclipseFinderDlg->sortColumn)
                            eclipseFinderDlg->sortColumnReverse = !eclipseFinderDlg->sortColumnReverse;
                        else
                            eclipseFinderDlg->sortColumnReverse = false;
                        eclipseFinderDlg->sortColumn = nm->iSubItem;

                        EclipseFinderSortInfo sortInfo;
                        sortInfo.iSubItem = nm->iSubItem;
                        sortInfo.reverse = eclipseFinderDlg->sortColumnReverse;
                        auto compareFunc = compareFuncs[static_cast<std::size_t>(nm->iSubItem)];
                        ListView_SortItems(hwnd, compareFunc, reinterpret_cast<LPARAM>(&sortInfo));
                    }
                    break;
                }
            }
            if (hdr->code == DTN_DATETIMECHANGE)
            {
                auto change = reinterpret_cast<LPNMDATETIMECHANGE>(lParam);
                if (change->dwFlags == GDT_VALID)
                {
                    if (wParam == IDC_DATEFROM)
                    {
                        eclipseFinderDlg->fromTime.wYear = change->st.wYear;
                        eclipseFinderDlg->fromTime.wMonth = change->st.wMonth;
                        eclipseFinderDlg->fromTime.wDay = change->st.wDay;
                    }
                    else if (wParam == IDC_DATETO)
                    {
                        eclipseFinderDlg->toTime.wYear = change->st.wYear;
                        eclipseFinderDlg->toTime.wMonth = change->st.wMonth;
                        eclipseFinderDlg->toTime.wDay = change->st.wDay;
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}

} // end unnamed namespace

EclipseFinderDialog::EclipseFinderDialog(HINSTANCE appInstance,
                                         HWND _parent,
                                         CelestiaCore* _appCore) :
    appCore(_appCore),
    parent(_parent),
    BodytoSet_(NULL)
{
    hwnd = CreateDialogParam(appInstance,
                             MAKEINTRESOURCE(IDD_ECLIPSEFINDER),
                             parent,
                             &EclipseFinderProc,
                             reinterpret_cast<LPARAM>(this));
}

EclipseFinderDialog::~EclipseFinderDialog() = default;

} // end namespace celestia::win32
