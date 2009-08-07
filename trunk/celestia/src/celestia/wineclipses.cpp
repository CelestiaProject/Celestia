// wineclipses.cpp by Kendrix <kendrix@wanadoo.fr>
// modified by Chris Laurel
// 
// Compute Solar Eclipses for our Solar System planets
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <string>
#include <sstream>
#include <algorithm>
#include <set>
#include <cassert>
#include <windows.h>
#include <commctrl.h>
#include "eclipsefinder.h"
#include "wineclipses.h"
#include "res/resource.h"
#include "celmath/geomutil.h"
#include "celutil/util.h"
#include "celutil/winutil.h"
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

WNDPROC oldListViewProc;

static vector<Eclipse> eclipseList;

extern void SetMouseCursor(LPCTSTR lpCursor);

char* MonthNames[12] =
{
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

bool InitEclipseFinderColumns(HWND listView)
{
    LVCOLUMN lvc;
    LVCOLUMN columns[5];

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_CENTER;
    lvc.pszText = "";

    int nColumns = sizeof(columns) / sizeof(columns[0]);
    int i;

    for (i = 0; i < nColumns; i++)
        columns[i] = lvc;

    bind_textdomain_codeset("celestia", CurrentCP());
    columns[0].pszText = _("Planet");
    columns[0].cx = 50;
    columns[1].pszText = _("Satellite");
    columns[1].cx = 65;
    columns[2].pszText = _("Date");
    columns[2].cx = 80;
    columns[3].pszText = _("Start");
    columns[3].cx = 55;
    columns[4].pszText = _("Duration");
    columns[4].cx = 55;
    bind_textdomain_codeset("celestia", "UTF8");

    for (i = 0; i < nColumns; i++)
    {
        columns[i].iSubItem = i;
        if (ListView_InsertColumn(listView, i, &columns[i]) == -1)
            return false;
    }

    return true;
}


bool InitEclipseFinderItems(HWND listView, const vector<Eclipse>& eclipses)
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
        lvi.lParam = (LPARAM) &(eclipses[i]);
        ListView_InsertItem(listView, &lvi);
    }

    return true;
}


static char callbackScratch[256];
void EclipseFinderDisplayItem(LPNMLVDISPINFOA nm)
{

    Eclipse* eclipse = reinterpret_cast<Eclipse*>(nm->item.lParam);
    if (eclipse == NULL)
    {
        nm->item.pszText = "";
        return;
    }

    switch (nm->item.iSubItem)
    {
    case 0:
        {
            strncpy(callbackScratch, UTF8ToCurrentCP(_(eclipse->planete.c_str())).c_str(), sizeof(callbackScratch) - 1);
            nm->item.pszText = callbackScratch;
        }
        break;
            
    case 1:
        {
            if (!strcmp(eclipse->planete.c_str(),"None")) 
            {
                sprintf(callbackScratch,"");
                nm->item.pszText = callbackScratch;
            }
            else
            {
                strncpy(callbackScratch, UTF8ToCurrentCP(_(eclipse->sattelite.c_str())).c_str(), sizeof(callbackScratch) - 1);
                nm->item.pszText = callbackScratch;
            }
        }
        break;

    case 2:
        {
            bind_textdomain_codeset("celestia", CurrentCP());
            astro::Date startDate(eclipse->startTime);
            if (!strcmp(eclipse->planete.c_str(),"None"))
                sprintf(callbackScratch,"");
            else
                sprintf(callbackScratch, "%2d %s %4d",
                        startDate.day,
                        _(MonthNames[startDate.month - 1]),
                        startDate.year);
            nm->item.pszText = callbackScratch;
            bind_textdomain_codeset("celestia", "UTF8");
        }
        break;
            
    case 3:
        {
            astro::Date startDate(eclipse->startTime);
            if (!strcmp(eclipse->planete.c_str(),"None"))
                sprintf(callbackScratch,"");
            else
            {
                sprintf(callbackScratch, "%02d:%02d",
                        startDate.hour, startDate.minute);
            }
            nm->item.pszText = callbackScratch;
        }
        break;

    case 4:
        {
            if (!strcmp(eclipse->planete.c_str(),"None")) 
            {
                sprintf(callbackScratch,"");
                nm->item.pszText = callbackScratch;
            }
            else
            {
                int minutes = (int) ((eclipse->endTime - eclipse->startTime) *
                                    24 * 60);
                sprintf(callbackScratch, "%02d:%02d", minutes / 60, minutes % 60);
                nm->item.pszText = callbackScratch;
            }
        }
        break;
    }
}


void InitDateControls(HWND hDlg, astro::Date& newTime, SYSTEMTIME& fromTime, SYSTEMTIME& toTime)
{
    HWND dateItem = NULL;

    fromTime.wYear = newTime.year - 1;
    fromTime.wMonth = newTime.month;
    fromTime.wDay = newTime.day;
    fromTime.wDayOfWeek = ((int) ((double) newTime + 0.5) + 1) % 7;
    fromTime.wHour = 0;
    fromTime.wMinute = 0;
    fromTime.wSecond = (int) 0;
    fromTime.wMilliseconds = 0;

    toTime = fromTime;
    toTime.wYear += 2;

    dateItem = GetDlgItem(hDlg, IDC_DATEFROM);
    if (dateItem != NULL)
    {
        DateTime_SetFormat(dateItem, "dd' 'MMM' 'yyy");
        DateTime_SetSystemtime(dateItem, GDT_VALID, &fromTime);
    }
    dateItem = GetDlgItem(hDlg, IDC_DATETO);
    if (dateItem != NULL)
    {
        DateTime_SetFormat(dateItem, "dd' 'MMM' 'yyy");
        DateTime_SetSystemtime(dateItem, GDT_VALID, &toTime);
    }
}


struct EclipseFinderSortInfo
{
    int         subItem;
    string      planete;
    string      sattelite;
    int32       Year;
    int8        Month;
    int8        Day;
    int8        Hour;
};


int CALLBACK EclipseFinderCompareFunc(LPARAM lParam0, LPARAM lParam1,
                                    LPARAM lParamSort)
{
    EclipseFinderSortInfo* sortInfo = reinterpret_cast<EclipseFinderSortInfo*>(lParamSort);
    Eclipse* eclipse0 = reinterpret_cast<Eclipse*>(lParam0);
    Eclipse* eclipse1 = reinterpret_cast<Eclipse*>(lParam1);

    switch (sortInfo->subItem)
    {
    case 1:
        if (eclipse0->sattelite < eclipse1->sattelite)
            return -1;
        else if (eclipse1->sattelite < eclipse0->sattelite)
            return 1;
        else
            return 0;

    case 4:
        {
            double duration0 = eclipse0->endTime - eclipse0->startTime;
            double duration1 = eclipse1->endTime - eclipse1->startTime;
            if (duration0 < duration1)
                return -1;
            else if (duration1 < duration0)
                return 1;
            else
                return 0;
        }

    default:
        if (eclipse0->startTime < eclipse1->startTime)
            return -1;
        else if (eclipse1->startTime < eclipse0->startTime)
            return 1;
        else
            return 0;
    }
}

BOOL APIENTRY EclipseListViewProc(HWND hWnd,
                                  UINT message,
                                  UINT wParam,
                                  LONG lParam)
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
                SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(IDSETDATEANDGO, 0), NULL);
            }
        }
        break;
    }

    return CallWindowProc(oldListViewProc, hWnd, message, wParam, lParam);
}

BOOL APIENTRY EclipseFinderProc(HWND hDlg,
                                UINT message,
                                UINT wParam,
                                LONG lParam)
{
    EclipseFinderDialog* eclipseFinderDlg = reinterpret_cast<EclipseFinderDialog*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            EclipseFinderDialog* efd = reinterpret_cast<EclipseFinderDialog*>(lParam);
            if (efd == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);
            HWND hwnd = GetDlgItem(hDlg, IDC_ECLIPSES_LIST);
            InitEclipseFinderColumns(hwnd);
            SendDlgItemMessage(hDlg, IDC_ECLIPSES_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

            bind_textdomain_codeset("celestia", CurrentCP());
            CheckRadioButton(hDlg, IDC_SOLARECLIPSE, IDC_LUNARECLIPSE, IDC_SOLARECLIPSE);
            efd->bSolar = true;

            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)_("Earth"));
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)_("Jupiter"));
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)_("Saturn"));
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)_("Uranus"));
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)_("Neptune"));
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)_("Pluto"));
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_SETCURSEL, 0, 0);
            efd->strPlaneteToFindOn = "Earth";
            bind_textdomain_codeset("celestia", "UTF8");

            InitDateControls(hDlg, astro::Date(efd->appCore->getSimulation()->getTime()), efd->fromTime, efd->toTime);

            // Subclass the ListView to intercept WM_LBUTTONUP messages
            HWND hCtrl;
            if (hCtrl = GetDlgItem(hDlg, IDC_ECLIPSES_LIST))
                oldListViewProc = (WNDPROC) SetWindowLong(hCtrl, GWL_WNDPROC, (DWORD) EclipseListViewProc);
        }
        return(TRUE);

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
                if (eclipseFinderDlg->strPlaneteToFindOn.empty())
                    eclipseFinderDlg->strPlaneteToFindOn = "Earth";
                SetMouseCursor(IDC_WAIT);


                astro::Date from(eclipseFinderDlg->fromTime.wYear,
                                 eclipseFinderDlg->fromTime.wMonth,
                                 eclipseFinderDlg->fromTime.wDay);
                astro::Date to(eclipseFinderDlg->toTime.wYear,
                               eclipseFinderDlg->toTime.wMonth,
                               eclipseFinderDlg->toTime.wDay);
                EclipseFinder ef(eclipseFinderDlg->appCore,
                                 eclipseFinderDlg->strPlaneteToFindOn,
                                 eclipseFinderDlg->bSolar ? Eclipse::Solar : Eclipse::Moon,
                                 (double) from,
                                 (double) to);
                eclipseList = ef.getEclipses();
                InitEclipseFinderItems(hwnd, eclipseList);
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
                sim->setFrame(ObserverFrame::PhaseLock, target, ref);
                sim->update(0.0);

                double distance = target.radius() * 4.0;
                sim->gotoLocation(UniversalCoord::Zero().offsetKm(Vector3d::UnitX() * distance), 
                                  YRotation(-PI / 2) * XRotation(-PI / 2),
                                  2.5);
            }
            break;
        case IDC_SOLARECLIPSE:
                eclipseFinderDlg->bSolar = true;
            break;
        case IDC_LUNARECLIPSE:
                eclipseFinderDlg->bSolar = false;
            break;
        case IDC_ECLIPSETARGET:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                switch(SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0))
                {
                case 0:
                    eclipseFinderDlg->strPlaneteToFindOn = "Earth";
                    break;
                case 1:
                    eclipseFinderDlg->strPlaneteToFindOn = "Jupiter";
                    break;
                case 2:
                    eclipseFinderDlg->strPlaneteToFindOn = "Saturn";
                    break;
                case 3:
                    eclipseFinderDlg->strPlaneteToFindOn = "Uranus";
                    break;
                case 4:
                    eclipseFinderDlg->strPlaneteToFindOn = "Neptune";
                    break;
                case 5:
                    eclipseFinderDlg->strPlaneteToFindOn = "Pluto";
                    break;
                }
            }
        }
        return TRUE;

    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR) lParam;

            if(hdr->idFrom == IDC_ECLIPSES_LIST)
            {
                switch(hdr->code)
                {
                case LVN_GETDISPINFO:
                    EclipseFinderDisplayItem((LPNMLVDISPINFOA) lParam);
                    break;
                case LVN_ITEMCHANGED:
                    {
                        LPNMLISTVIEW nm = (LPNMLISTVIEW) lParam;
                        if ((nm->uNewState & LVIS_SELECTED) != 0)
                        {
                            Eclipse* eclipse = reinterpret_cast<Eclipse*>(nm->lParam);
                            if (eclipse != NULL)
                            {
                                eclipseFinderDlg->TimetoSet_ = 
                                    (eclipse->startTime + eclipse->endTime) / 2.0f;
                                eclipseFinderDlg->BodytoSet_ = eclipse->body;
                            }
                        }
                        break;
                    }
                case LVN_COLUMNCLICK:
                    {
                        HWND hwnd = GetDlgItem(hDlg, IDC_ECLIPSES_LIST);
                        if (hwnd != 0)
                        {
                            LPNMLISTVIEW nm = (LPNMLISTVIEW) lParam;
                            EclipseFinderSortInfo sortInfo;
                            sortInfo.subItem = nm->iSubItem;
//                            sortInfo.sattelite = ;
//                            sortInfo.pos = eclipseFinderDlg->pos;
                            ListView_SortItems(hwnd, EclipseFinderCompareFunc,
                                               reinterpret_cast<LPARAM>(&sortInfo));
                        }
                    }

                }
            }
            if (hdr->code == DTN_DATETIMECHANGE)
            {
                LPNMDATETIMECHANGE change = (LPNMDATETIMECHANGE) lParam;
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
                             EclipseFinderProc,
                             reinterpret_cast<LONG>(this));
}
