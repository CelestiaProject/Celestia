// wineclipses.cpp by Kendrix <kendrix@wanadoo.fr>
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
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
#include "wineclipses.h"
#include "res/resource.h"
#include "celmath/ray.h"
#include "celmath/distance.h"

using namespace std;

vector<Eclipse> Eclipses_;

extern void SetMouseCursor(LPCTSTR lpCursor);

char* MonthNames[12] =
{
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

Eclipse::Eclipse(int Y, int M, int D)
{
    date = new astro::Date(Y, M, D);
}

Eclipse::Eclipse(double JD)
{
    date = new astro::Date(JD);
}

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

    columns[0].pszText = "Planet";
    columns[0].cx = 50;
    columns[1].pszText = "Satellite";
    columns[1].cx = 65;
    columns[2].pszText = "Date";
    columns[2].cx = 80;
    columns[3].pszText = "Start";
    columns[3].cx = 55;
    columns[4].pszText = "Duration";
    columns[4].cx = 55;

    for (i = 0; i < nColumns; i++)
    {
        columns[i].iSubItem = i;
        if (ListView_InsertColumn(listView, i, &columns[i]) == -1)
            return false;
    }

    return true;
}


bool InitEclipseFinderLVItems(HWND listView)
{
    LVITEM lvi;

    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;

    for (int i = 0; i < Eclipses_.size(); i++)
    {
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.lParam = (LPARAM) &(Eclipses_[i]);
        ListView_InsertItem(listView, &lvi);
    }

    return true;
}


bool InitEclipseFinderItems(HWND listView)
{
    bool succeeded = InitEclipseFinderLVItems(listView);
    return succeeded;
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
            nm->item.pszText = const_cast<char*>(eclipse->planete.c_str());
        }
        break;
            
    case 1:
        {
            nm->item.pszText = const_cast<char*>(eclipse->sattelite.c_str());
        }
        break;

    case 2:
        {
            astro::Date startDate(eclipse->startTime);
            if (!strcmp(eclipse->planete.c_str(),"None"))
                sprintf(callbackScratch,"");
            else
                sprintf(callbackScratch, "%2d %s %4d",
                        startDate.day,
                        MonthNames[startDate.month - 1],
                        startDate.year);
            nm->item.pszText = callbackScratch;
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
            int minutes = (int) ((eclipse->endTime - eclipse->startTime) *
                                 24 * 60);
            sprintf(callbackScratch, "%02d:%02d", minutes / 60, minutes % 60);
            nm->item.pszText = callbackScratch;
        }
        break;
    }
}


bool testEclipse(const Body& receiver, const Body& caster,
                 double now)
{
    // Ignore situations where the shadow casting body is much smaller than
    // the receiver, as these shadows aren't likely to be relevant.  Also,
    // ignore eclipses where the caster is not an ellipsoid, since we can't
    // generate correct shadows in this case.
    if (caster.getRadius() * 100 >= receiver.getRadius() &&
        caster.getMesh() == InvalidResource)
    {
        // All of the eclipse related code assumes that both the caster
        // and receiver are spherical.  Irregular receivers will work more
        // or less correctly, but casters that are sufficiently non-spherical
        // will produce obviously incorrect shadows.  Another assumption we
        // make is that the distance between the caster and receiver is much
        // less than the distance between the sun and the receiver.  This
        // approximation works everywhere in the solar system, and likely
        // works for any orbitally stable pair of objects orbiting a star.
        Point3d posReceiver = receiver.getHeliocentricPosition(now);
        Point3d posCaster = caster.getHeliocentricPosition(now);

        const Star* sun = receiver.getSystem()->getStar();
        assert(sun != NULL);
        double distToSun = posReceiver.distanceFromOrigin();
        float appSunRadius = (float) (sun->getRadius() / distToSun);

        Vec3d dir = posCaster - posReceiver;
        double distToCaster = dir.length() - receiver.getRadius();
        float appOccluderRadius = (float) (caster.getRadius() / distToCaster);

        // The shadow radius is the radius of the occluder plus some additional
        // amount that depends upon the apparent radius of the sun.  For
        // a sun that's distant/small and effectively a point, the shadow
        // radius will be the same as the radius of the occluder.
        float shadowRadius = (1 + appSunRadius / appOccluderRadius) *
            caster.getRadius();

        // Test whether a shadow is cast on the receiver.  We want to know
        // if the receiver lies within the shadow volume of the caster.  Since
        // we're assuming that everything is a sphere and the sun is far
        // away relative to the caster, the shadow volume is a
        // cylinder capped at one end.  Testing for the intersection of a
        // singly capped cylinder is as simple as checking the distance
        // from the center of the receiver to the axis of the shadow cylinder.
        // If the distance is less than the sum of the caster's and receiver's
        // radii, then we have an eclipse.
        float R = receiver.getRadius() + shadowRadius;
        double dist = distance(posReceiver,
                               Ray3d(posCaster, posCaster - Point3d(0, 0, 0)));
        if (dist < R)
        {
            return true;
        }
    }

    return false;
}


double findEclipseSpan(const Body& receiver, const Body& caster,
                       double now, double dt)
{
    double t = now;
    while (testEclipse(receiver, caster, t))
        t += dt;

    return t;
}


void CalculateEclipses(CelestiaCore* appCore, string planetetofindon, SYSTEMTIME fromTime, SYSTEMTIME toTime, bool bSolar)
{
    Simulation* sim = appCore->getSimulation();
    Eclipse* eclipse;
    astro::Date from(fromTime.wYear, fromTime.wMonth, fromTime.wDay), to(toTime.wYear, toTime.wMonth, toTime.wDay);
    double JDfrom, JDto;
    double* JDback = NULL;
    int nIDplanetetofindon;
    int nSattelites;

    JDfrom = (double)from;
    JDto = (double)to;

    const SolarSystem* sys = sim->getNearestSolarSystem();
    if ((!sys))
    {
        MessageBox(NULL, "Only usable in our Solar System.", "Eclipse Finder Error", MB_OK | MB_ICONERROR);
        eclipse = new Eclipse(0.);
        eclipse->planete = "None";
        Eclipses_.insert(Eclipses_.end(), *eclipse);
        delete eclipse;
        return;
    }
    else if ((sys->getStar()->getCatalogNumber(0) != 0) && (sys->getStar()->getCatalogNumber(1) != 0))
    {
        MessageBox(NULL, "Only usable in our Solar System.", "Eclipse Finder Error", MB_OK | MB_ICONERROR);
        eclipse = new Eclipse(0.);
        eclipse->planete = "None";
        Eclipses_.insert(Eclipses_.end(), *eclipse);
        delete eclipse;
        return;
    }

    PlanetarySystem* system = sys->getPlanets();
    int nbPlanets = system->getSystemSize();

    for (int i = 0; i < nbPlanets; ++i)
    {
        Body* planete = system->getBody(i);
        if (planete != NULL)
            if (planetetofindon == planete->getName())
            {
                nIDplanetetofindon = i;
                PlanetarySystem* satellites = planete->getSatellites();
                if (satellites)
                {
                    nSattelites = satellites->getSystemSize();
                    JDback = new double[nSattelites];
                    memset(JDback, 0, nSattelites*sizeof(double));
                }
                break;
            }
    }

    Body* planete = system->getBody(nIDplanetetofindon);
    while (JDfrom < JDto)
    {
        PlanetarySystem* satellites = planete->getSatellites();
        if (satellites)
        {
            for (int j = 0; j < nSattelites; ++j)
            {
                Body* caster = NULL;
                Body* receiver = NULL;
                bool test = false;

                if (satellites->getBody(j)->getClassification() != Body::Spacecraft)
                {
                    if (bSolar)
                    {
                        caster = satellites->getBody(j);
                        receiver = planete;
                    }
                    else
                    {
                        caster = planete;
                        receiver = satellites->getBody(j);
                    }

                    test = testEclipse(*receiver, *caster, JDfrom);
                }

                if (test && JDfrom - JDback[j] > 1)
                {
                    JDback[j] = JDfrom;
                    eclipse = new Eclipse(JDfrom);
                    eclipse->startTime = findEclipseSpan(*receiver, *caster,
                                                         JDfrom,
                                                         -1.0 / (24.0 * 60.0));
                    eclipse->endTime   = findEclipseSpan(*receiver, *caster,
                                                         JDfrom,
                                                         1.0 / (24.0 * 60.0));
                    eclipse->body = receiver;
                    eclipse->planete = planete->getName();
                    eclipse->sattelite = satellites->getBody(j)->getName();
                    Eclipses_.insert(Eclipses_.end(), *eclipse);
                    delete eclipse;
                }
            }
        }
        JDfrom += 1.0 / 24.0;
    }
    if (JDback)
        delete JDback;
    if (Eclipses_.empty())
    {
        eclipse = new Eclipse(0.);
        eclipse->planete = "None";
        Eclipses_.insert(Eclipses_.end(), *eclipse);
        delete eclipse;
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
        else
            return 1;

    case 4:
        if ((eclipse0->endTime - eclipse0->startTime) <
            (eclipse1->endTime - eclipse1->startTime))
            return -1;
        else
            return 1;

    default:
        if (eclipse0->startTime < eclipse1->startTime)
            return -1;
        else
            return 1;
    }
}


BOOL APIENTRY EclipseFinderProc(HWND hDlg,
                                UINT message,
                                UINT wParam,
                                LONG lParam)
{
    EclipseFinder* eclipseFinder = reinterpret_cast<EclipseFinder*>(GetWindowLong(hDlg, DWL_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            EclipseFinder* eclipse = reinterpret_cast<EclipseFinder*>(lParam);
            if (eclipse == NULL)
                return EndDialog(hDlg, 0);
            SetWindowLong(hDlg, DWL_USER, lParam);
            HWND hwnd = GetDlgItem(hDlg, IDC_ECLIPSES_LIST);
            InitEclipseFinderColumns(hwnd);
            SendDlgItemMessage(hDlg, IDC_ECLIPSES_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

            SendDlgItemMessage(hDlg, IDC_ECLIPSETYPE, CB_ADDSTRING, 0, (LPARAM)"solar");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETYPE, CB_ADDSTRING, 0, (LPARAM)"moon");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETYPE, CB_SETCURSEL, 0, 0);
            eclipse->bSolar = true;

            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)"Earth");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)"Jupiter");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)"Saturn");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)"Uranus");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)"Neptune");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_ADDSTRING, 0, (LPARAM)"Pluto");
            SendDlgItemMessage(hDlg, IDC_ECLIPSETARGET, CB_SETCURSEL, 0, 0);
            eclipse->strPlaneteToFindOn = "Earth";

            InitDateControls(hDlg, astro::Date(eclipse->appCore->getSimulation()->getTime()), eclipse->fromTime, eclipse->toTime);
        }
        return(TRUE);

    case WM_DESTROY:
        if (eclipseFinder != NULL && eclipseFinder->parent != NULL)
        {
            Eclipses_.clear();
            SendMessage(eclipseFinder->parent, WM_COMMAND, IDCLOSE,
                        reinterpret_cast<LPARAM>(eclipseFinder));
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
                Eclipses_.clear();
                if (eclipseFinder->strPlaneteToFindOn.empty())
                    eclipseFinder->strPlaneteToFindOn = "Earth";
                SetMouseCursor(IDC_WAIT);
                CalculateEclipses(eclipseFinder->appCore,
                                  eclipseFinder->strPlaneteToFindOn,
                                  eclipseFinder->fromTime,
                                  eclipseFinder->toTime,
                                  eclipseFinder->bSolar);
                InitEclipseFinderItems(hwnd);
                SetMouseCursor(IDC_ARROW);
                break;
            }

        case IDCLOSE:
            {
                if (eclipseFinder != NULL && eclipseFinder->parent != NULL)
                {
                    Eclipses_.clear();
                    SendMessage(eclipseFinder->parent, WM_COMMAND, IDCLOSE,
                                reinterpret_cast<LPARAM>(eclipseFinder));
                }
                EndDialog(hDlg, 0);
                break;
            }

        case IDSETDATEANDGO:
            if (eclipseFinder->BodytoSet_)
            {
                Simulation* sim = eclipseFinder->appCore->getSimulation();
                sim->setTime(eclipseFinder->TimetoSet_);
                sim->setSelection(Selection(eclipseFinder->BodytoSet_));
                sim->follow();
                sim->centerSelection();
                sim->gotoSelection(0., Vec3f(0, 1, 0), astro::ObserverLocal);
            }
            break;
        case IDC_ECLIPSETYPE:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                switch(SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0))
                {
                case 0:
                    eclipseFinder->bSolar = true;
                    break;
                case 1:
                    eclipseFinder->bSolar = false;
                    break;
                }
            }
        case IDC_ECLIPSETARGET:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                switch(SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0))
                {
                case 0:
                    eclipseFinder->strPlaneteToFindOn = "Earth";
                    break;
                case 1:
                    eclipseFinder->strPlaneteToFindOn = "Jupiter";
                    break;
                case 2:
                    eclipseFinder->strPlaneteToFindOn = "Saturn";
                    break;
                case 3:
                    eclipseFinder->strPlaneteToFindOn = "Uranus";
                    break;
                case 4:
                    eclipseFinder->strPlaneteToFindOn = "Neptune";
                    break;
                case 5:
                    eclipseFinder->strPlaneteToFindOn = "Pluto";
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
                                eclipseFinder->TimetoSet_ = 
                                    (eclipse->startTime + eclipse->endTime) / 2.0f;
                                eclipseFinder->BodytoSet_ = eclipse->body;
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
//                            sortInfo.pos = eclipseFinder->pos;
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
                        eclipseFinder->fromTime.wYear = change->st.wYear;
                        eclipseFinder->fromTime.wMonth = change->st.wMonth;
                        eclipseFinder->fromTime.wDay = change->st.wDay;
                    }
                    else if (wParam == IDC_DATETO)
                    {
                        eclipseFinder->toTime.wYear = change->st.wYear;
                        eclipseFinder->toTime.wMonth = change->st.wMonth;
                        eclipseFinder->toTime.wDay = change->st.wDay;
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}


EclipseFinder::EclipseFinder(HINSTANCE appInstance,
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
