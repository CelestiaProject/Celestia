// wintime.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Original version:
// Copyright (C) 2005, Chris Laurel <claurel@shatters.net>
//
// Win32 set time dialog box for Celestia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "wintime.h"

#include <array>
#include <cstdint>
#include <memory>
#include <system_error>

#include <fmt/format.h>
#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <winuser.h>
#include <commctrl.h>
#include <time.h>

#include <celastro/date.h>
#include <celcompat/charconv.h>
#include <celestia/celestiacore.h>
#include <celutil/gettext.h>
#include "res/resource.h"
#include "tcharconv.h"
#include "tstring.h"

namespace celestia::win32
{

namespace
{

class SetTimeDialog
{
public:
    SetTimeDialog(CelestiaCore*);
    ~SetTimeDialog();

    void init(HWND _hDlg);
    double getTime() const;
    void setTime(const double _tdb);

    void updateControls();

    LRESULT command(WPARAM wParam, LPARAM lParam);
    LRESULT notify(int id, const NMHDR& nmhdr);

private:
    HWND hDlg;
    CelestiaCore* appCore;
    double tdb;
    bool useLocalTime;
    bool useUTCOffset;
    int localTimeZoneBiasInSeconds;

    void getLocalTimeZoneInfo();
};

SetTimeDialog::SetTimeDialog(CelestiaCore* _appCore) :
    hDlg(0),
    appCore(_appCore),
    tdb(astro::J2000),
    useLocalTime(false),
    useUTCOffset(false),
    localTimeZoneBiasInSeconds(0)
{
}

SetTimeDialog::~SetTimeDialog()
{
}

void
SetTimeDialog::init(HWND _hDlg)
{
    hDlg = _hDlg;

    SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LPARAM>(this));
    getLocalTimeZoneInfo();

    tdb = appCore->getSimulation()->getTime();
    useLocalTime = appCore->getTimeZoneBias() != 0;
    useUTCOffset = appCore->getDateFormat() == 2;

    tstring item0 = UTF8ToTString(_("Universal Time"));
    tstring item1 = UTF8ToTString(_("Local Time"));
    tstring item2 = UTF8ToTString(_("Time Zone Name"));
    tstring item3 = UTF8ToTString(_("UTC Offset"));

    SendDlgItemMessage(hDlg, IDC_COMBOBOX_TIMEZONE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item0.c_str()));
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_TIMEZONE, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item1.c_str()));
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_DATE_FORMAT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item2.c_str()));
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_DATE_FORMAT, CB_ADDSTRING, 0,  reinterpret_cast<LPARAM>(item3.c_str()));

    SendDlgItemMessage(hDlg, IDC_COMBOBOX_TIMEZONE, CB_SETCURSEL, useLocalTime ? 1 : 0, 0);
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_DATE_FORMAT, CB_SETCURSEL, useUTCOffset ? 1 : 0, 0);

    EnableWindow(GetDlgItem(hDlg, IDC_COMBOBOX_DATE_FORMAT), useLocalTime);

    updateControls();
}

void
SetTimeDialog::getLocalTimeZoneInfo()
{
    TIME_ZONE_INFORMATION tzi;
    DWORD dst = GetTimeZoneInformation(&tzi);
    if (dst == TIME_ZONE_ID_INVALID)
        return;

    LONG dstBias = 0;

    if (dst == TIME_ZONE_ID_STANDARD)
        dstBias = tzi.StandardBias;
    else if (dst == TIME_ZONE_ID_DAYLIGHT)
        dstBias = tzi.DaylightBias;

    localTimeZoneBiasInSeconds = (tzi.Bias + dstBias) * -60;
}

double
SetTimeDialog::getTime() const
{
    return tdb;
}

void
SetTimeDialog::setTime(double _tdb)
{
    tdb = _tdb;
}

void
SetTimeDialog::updateControls()
{
    HWND timeItem = NULL;
    HWND dateItem = NULL;
    HWND jdItem = NULL;
    SYSTEMTIME sysTime;
    double tztdb = tdb;

    if (useLocalTime)
        tztdb += localTimeZoneBiasInSeconds / 86400.0;

    astro::Date newTime = astro::TDBtoUTC(tztdb);

    sysTime.wYear = newTime.year;
    sysTime.wMonth = newTime.month;
    sysTime.wDay = newTime.day;
    sysTime.wDayOfWeek = ((int) ((double) newTime + 0.5) + 1) % 7;
    sysTime.wHour = newTime.hour;
    sysTime.wMinute = newTime.minute;
    sysTime.wSecond = (int) newTime.seconds;
    sysTime.wMilliseconds = 0;

    dateItem = GetDlgItem(hDlg, IDC_DATEPICKER);
    if (dateItem != NULL)
    {
        DateTime_SetFormat(dateItem, TEXT("dd' 'MMM' 'yyy"));
        DateTime_SetSystemtime(dateItem, GDT_VALID, &sysTime);
    }
    timeItem = GetDlgItem(hDlg, IDC_TIMEPICKER);
    if (timeItem != NULL)
    {
        DateTime_SetFormat(timeItem, TEXT("HH':'mm':'ss"));
        DateTime_SetSystemtime(timeItem, GDT_VALID, &sysTime);
    }

    jdItem = GetDlgItem(hDlg, IDC_JDPICKER);
    if (jdItem != NULL)
    {
        auto jdUtc = astro::TAItoJDUTC(astro::TTtoTAI(astro::TDBtoTT(tdb)));

        std::array<TCHAR, 16> jd;
        jd.fill(TEXT('\0'));
        fmt::format_to_n(jd.begin(), jd.size() - 1, TEXT("{:.5f}"), jdUtc);
        SetWindowText(GetDlgItem(hDlg, IDC_JDPICKER), jd.data());
    }
}

LRESULT
SetTimeDialog::command(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case IDOK:
            appCore->tick();
            appCore->getSimulation()->setTime(tdb);
            appCore->setTimeZoneBias(useLocalTime ? 1 : 0);
            appCore->setDateFormat((astro::Date::Format) (useLocalTime && useUTCOffset ? 2 : 1));
            EndDialog(hDlg, 0);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, 0);
            return TRUE;

        case IDC_SETCURRENTTIME:
            // Set time to the current system time
            setTime(astro::UTCtoTDB((double) time(NULL) / 86400.0 + (double) astro::Date(1970, 1, 1)));
            updateControls();
            return TRUE;

        case IDC_COMBOBOX_TIMEZONE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                LRESULT selection = SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);
                useLocalTime = (selection == 1);
                EnableWindow (GetDlgItem (hDlg, IDC_COMBOBOX_DATE_FORMAT), useLocalTime);
                updateControls();
            }
            return TRUE;

        case IDC_COMBOBOX_DATE_FORMAT:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                LRESULT selection = SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);
                useUTCOffset = (selection == 1);
                updateControls();
            }
            return TRUE;

        case IDC_JDPICKER:
            if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                std::array<TCHAR, 16> jdStr;
                GetWindowText(GetDlgItem(hDlg, IDC_JDPICKER), jdStr.data(), static_cast<int>(jdStr.size()));
                double jd;
                if (auto ec = from_tchars(jdStr.data(), jdStr.data() + jdStr.size(), jd).ec;
                    ec == std::errc{})
                {
                    tdb = astro::TTtoTDB(astro::TAItoTT(astro::JDUTCtoTAI(jd)));
                }

                updateControls();
            }
            return TRUE;

        default:
            return FALSE;
    }
}

LRESULT
SetTimeDialog::notify(int id, const NMHDR& hdr)
{
    astro::Date newTime(tdb);

    if (hdr.code != DTN_DATETIMECHANGE)
        return TRUE;

    LPNMDATETIMECHANGE change = (LPNMDATETIMECHANGE) &hdr;
    if (change->dwFlags != GDT_VALID || (id != IDC_DATEPICKER && id != IDC_TIMEPICKER))
        return TRUE;

    SYSTEMTIME sysTime;
    SYSTEMTIME sysDate;
    DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_TIMEPICKER), &sysTime);
    DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_DATEPICKER), &sysDate);
    newTime.year = static_cast<std::int16_t>(sysDate.wYear);
    newTime.month = sysDate.wMonth;
    newTime.day = sysDate.wDay;
    newTime.hour = sysTime.wHour;
    newTime.minute = sysTime.wMinute;
    newTime.seconds = sysTime.wSecond + (double) sysTime.wMilliseconds / 1000.0;

    tdb = astro::UTCtoTDB(newTime);
    if (useLocalTime)
        tdb -= localTimeZoneBiasInSeconds / 86400.0;

    updateControls();

    return TRUE;
}

BOOL APIENTRY
SetTimeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    SetTimeDialog* timeDialog = reinterpret_cast<SetTimeDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
    case WM_INITDIALOG:
        {
            timeDialog = reinterpret_cast<SetTimeDialog*>(lParam);
            if (timeDialog == NULL)
                return EndDialog(hDlg, 0);

            timeDialog->init(hDlg);
        }
        return TRUE;

    case WM_COMMAND:
        return timeDialog->command(wParam, lParam);

    case WM_NOTIFY:
        return timeDialog->notify((int) wParam, *reinterpret_cast<NMHDR*>(lParam));
    }

    return FALSE;
}

} // end unnamed namespace


void
ShowSetTimeDialog(HINSTANCE appInstance,
                  HWND appWindow,
                  CelestiaCore* appCore)
{
    auto timeDialog = std::make_unique<SetTimeDialog>(appCore);
    DialogBoxParam(appInstance,
                   MAKEINTRESOURCE(IDD_SETTIME),
                   appWindow,
                   (DLGPROC)SetTimeProc,
                   reinterpret_cast<LPARAM>(timeDialog.get()));
}

} // end namespace celestia::win32
