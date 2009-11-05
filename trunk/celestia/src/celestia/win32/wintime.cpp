// wintime.cpp
//
// Copyright (C) 2005, Chris Laurel <claurel@shatters.net>
//
// Win32 set time dialog box for Celestia
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "wintime.h"
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <time.h>
#include "res/resource.h"
#include <celengine/astro.h>
#include "celutil/util.h"
#include "celutil/winutil.h"



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


static void acronymify(char* words, int length)
{
    int n = 0;
    char lastChar = ' ';

    for (int i = 0; i < length; i++)
    {
        if (lastChar == ' ')
            words[n++] = words[i];
        lastChar = words[i];
    }
    words[n] = '\0';
}


void
SetTimeDialog::init(HWND _hDlg)
{
    hDlg = _hDlg;
    
    SetWindowLong(hDlg, DWL_USER, reinterpret_cast<LPARAM>(this));
    getLocalTimeZoneInfo();

    tdb = appCore->getSimulation()->getTime();
    useLocalTime = appCore->getTimeZoneBias() != 0;
    useUTCOffset = appCore->getDateFormat() == 2;

    bind_textdomain_codeset("celestia", CurrentCP());
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_TIMEZONE, CB_ADDSTRING, 0, (LPARAM) _("Universal Time"));
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_TIMEZONE, CB_ADDSTRING, 0, (LPARAM) _("Local Time"));
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_DATE_FORMAT, CB_ADDSTRING, 0, (LPARAM) _("Time Zone Name"));
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_DATE_FORMAT, CB_ADDSTRING, 0, (LPARAM) _("UTC Offset"));
    bind_textdomain_codeset("celestia", "UTF8");
    
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_TIMEZONE, CB_SETCURSEL, useLocalTime ? 1 : 0, 0);
    SendDlgItemMessage(hDlg, IDC_COMBOBOX_DATE_FORMAT, CB_SETCURSEL, useUTCOffset ? 1 : 0, 0);

    EnableWindow (GetDlgItem (hDlg, IDC_COMBOBOX_DATE_FORMAT), useLocalTime);

    updateControls();
}


void
SetTimeDialog::getLocalTimeZoneInfo()
{
    TIME_ZONE_INFORMATION tzi;
    DWORD dst = GetTimeZoneInformation(&tzi);
    if (dst != TIME_ZONE_ID_INVALID)
    {
        WCHAR* tzName = NULL;
        LONG dstBias = 0;

        if (dst == TIME_ZONE_ID_STANDARD)
        {
            dstBias = tzi.StandardBias;
            tzName = tzi.StandardName;
        }
        else if (dst == TIME_ZONE_ID_DAYLIGHT)
        {
            dstBias = tzi.DaylightBias;
            tzName = tzi.DaylightName;
        }

        localTimeZoneBiasInSeconds = (tzi.Bias + dstBias) * -60;
    }
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
        DateTime_SetFormat(dateItem, "dd' 'MMM' 'yyy");
        DateTime_SetSystemtime(dateItem, GDT_VALID, &sysTime);
    }
    timeItem = GetDlgItem(hDlg, IDC_TIMEPICKER);
    if (timeItem != NULL)
    {
        char timeFormat[256];
        sprintf(timeFormat, "HH':'mm':'ss");
        DateTime_SetFormat(timeItem, timeFormat);
        DateTime_SetSystemtime(timeItem, GDT_VALID, &sysTime);
    }

    jdItem = GetDlgItem(hDlg, IDC_JDPICKER);
    if (jdItem != NULL)
    {
        char jd[16];
        sprintf(jd, "%0.5f", astro::TAItoJDUTC(astro::TTtoTAI(astro::TDBtoTT(tdb))));
        SetWindowText(GetDlgItem(hDlg, IDC_JDPICKER), jd);
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
                char jd[16];
                GetWindowText(GetDlgItem(hDlg, IDC_JDPICKER), jd, sizeof(jd));
                tdb = astro::TTtoTDB(astro::TAItoTT(astro::JDUTCtoTAI(atof(jd))));
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
    
    if (hdr.code == DTN_DATETIMECHANGE)
    {
        LPNMDATETIMECHANGE change = (LPNMDATETIMECHANGE) &hdr;
        if (change->dwFlags == GDT_VALID)
        {
            if (id == IDC_DATEPICKER || id == IDC_TIMEPICKER)
            {
                SYSTEMTIME sysTime;
                SYSTEMTIME sysDate;
                DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_TIMEPICKER), &sysTime);
                DateTime_GetSystemtime(GetDlgItem(hDlg, IDC_DATEPICKER), &sysDate);
                newTime.year = (int16) sysDate.wYear;
                newTime.month = sysDate.wMonth;
                newTime.day = sysDate.wDay;
                newTime.hour = sysTime.wHour;
                newTime.minute = sysTime.wMinute;
                newTime.seconds = sysTime.wSecond + (double) sysTime.wMilliseconds / 1000.0;
                
                tdb = astro::UTCtoTDB(newTime);
                if (useLocalTime)
                    tdb -= localTimeZoneBiasInSeconds / 86400.0;

                updateControls();
            }
        }
    }
        
    return TRUE;
}


static BOOL APIENTRY 
SetTimeProc(HWND hDlg,
            UINT message,
            UINT wParam,
            LONG lParam)
{
    SetTimeDialog* timeDialog = reinterpret_cast<SetTimeDialog*>(GetWindowLong(hDlg, DWL_USER));

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


void
ShowSetTimeDialog(HINSTANCE appInstance,
                  HWND appWindow,
                  CelestiaCore* appCore)
{
    SetTimeDialog* timeDialog = new SetTimeDialog(appCore);
    
    DialogBoxParam(appInstance, MAKEINTRESOURCE(IDD_SETTIME), appWindow, SetTimeProc, reinterpret_cast<LPARAM>(timeDialog));
    
    delete timeDialog;
}
