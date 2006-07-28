// windatepicker.cpp
//
// Copyright (C) 2005, Chris Laurel <claurel@shatters.net>
//
// Windows front end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <windows.h>
#include <commctrl.h>
#include "celutil/basictypes.h"
#include "celengine/astro.h"
#include "celutil/util.h"
#include "celutil/winutil.h"


// DatePicker is a Win32 control for setting the date. It replaces the
// date picker from commctl, adding a number of features appropriate
// for astronomical applications:
//
// - The standard Windows date picker does not permit setting years
//   prior to 1752, the point that the US and UK switched to the 
//   Gregorian calendar. Celestia's date picker allows setting any
//   year from -9999 to 9999.
//
// - Astronomical year conventions are used for dates before the
//   year 1. This means that the year 0 is not omitted, and the year
//   2 BCE is entered as -1.
//
// - The first adoption of the Gregorian calendar was in 1582, when
//   days 5-14 were skipped in the month of October. All dates are
//   based on the initial 1582 reform, even though most countries
//   didn't adopt the Gregorian calendar until many years later.
//
// - No invalid date is permitted, including the skipped days in
//   October 1582.

static char* Months[12] = 
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

enum DatePickerField
{
    InvalidField = -1,
    DayField     =  0,
    MonthField   =  1,
    YearField    =  2,
    NumFields    =  3,
};

class DatePicker
{
public:
    DatePicker(HWND _hwnd, CREATESTRUCT& cs);
    ~DatePicker();
    
    LRESULT paint(HDC hdc);
    void redraw(HDC hdc);
    LRESULT keyDown(DWORD vkcode, LPARAM lParam);
    LRESULT killFocus(HWND lostFocus);
    LRESULT setFocus(HWND lostFocus);
    LRESULT enable(bool);
    LRESULT leftButtonDown(WORD key, int x, int y);
    LRESULT notify(int id, const NMHDR& nmhdr);
    LRESULT command(WPARAM wParam, LPARAM lParam);
    LRESULT resize(WORD flags, int width, int height);

    bool sendNotify(UINT code);
    bool notifyDateChanged();

    LRESULT setSystemTime(DWORD flag, SYSTEMTIME* sysTime);
    LRESULT getSystemTime(SYSTEMTIME* sysTime);

    LRESULT destroy();

private:
    int getFieldWidth(DatePickerField field, HDC hdc);
    void incrementField();
    void decrementField();

private:
    HWND hwnd;
    HWND parent;
    astro::Date date;
    DatePickerField selectedField;
    char textBuffer[64];
    HFONT hFont;
    DWORD style;
    
    bool haveFocus;
    bool firstDigit;

    RECT fieldRects[NumFields];
    RECT clientRect;
};


DatePicker::DatePicker(HWND _hwnd, CREATESTRUCT& cs) :
    hwnd(_hwnd),
    parent(cs.hwndParent),
    date(1970, 10, 25),
    selectedField(YearField),
    haveFocus(false),
    firstDigit(true)
{
    textBuffer[0] = '\0';

    hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

    clientRect.left = 0;
    clientRect.right = 0;
    clientRect.top = 0;
    clientRect.bottom = 0;
}


DatePicker::~DatePicker()
{
}


LRESULT
DatePicker::paint(HDC hdc)
{
    PAINTSTRUCT ps;

    if (!hdc)
    {
        hdc = BeginPaint(hwnd, &ps);
        redraw(hdc);
        EndPaint(hwnd, &ps);
    }
    else
    {
        redraw(hdc);
    }

    return 0;
}


void
DatePicker::redraw(HDC hdc)
{
    RECT rect;
    GetClientRect(hwnd, &rect);

    SelectObject(hdc, hFont);
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);

    char dayBuf[32];
    char monthBuf[32];
    char yearBuf[32];

    bind_textdomain_codeset("celestia", CurrentCP());
    sprintf(dayBuf, "%02d", date.day);
    sprintf(monthBuf, "%s", _(Months[date.month - 1]));
    sprintf(yearBuf, "%5d", date.year);
    bind_textdomain_codeset("celestia", "UTF8");

    char* fieldText[NumFields];
    fieldText[DayField] = dayBuf;
    fieldText[MonthField] = monthBuf;
    fieldText[YearField] = yearBuf;

    int right = 2;
    for (unsigned int i = 0; i < NumFields; i++)
    {
        SIZE size;
        GetTextExtentPoint(hdc, fieldText[i], strlen(fieldText[i]), &size);
        int fieldWidth = getFieldWidth(DatePickerField(i), hdc);
        fieldRects[i].left = right;
        fieldRects[i].right = right + fieldWidth;
        fieldRects[i].top = rect.top;
        fieldRects[i].bottom = rect.bottom;

        right = fieldRects[i].right;

        if (i == selectedField && haveFocus)
        {
            RECT r = fieldRects[i];
            r.top = (clientRect.bottom - size.cy) / 2;
            r.bottom = r.top + size.cy + 1;

            HBRUSH hbrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            FillRect(hdc, &r, hbrush);
            DeleteObject(hbrush);
			
			SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
		else
		{
			SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
		}

        DrawText(hdc, fieldText[i], strlen(fieldText[i]), &fieldRects[i], DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
}


static bool isLeapYear(unsigned int year)
{
    if (year > 1582)
    {
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    }
    else
    {
        return year % 4 == 0;
    }
}


static unsigned int daysInMonth(unsigned int month, unsigned int year)
{
    static unsigned int daysPerMonth[12] =
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 2)
        return isLeapYear(year) ? 29 : 28;
    else
        return daysPerMonth[month - 1];
}


static void clampToValidDate(astro::Date& date)
{
    int days = (int) daysInMonth(date.month, date.year);
    if (date.day > days)
        date.day = days;

    // 10 days skipped in Gregorian calendar reform
    if (date.year == 1582 && date.month == 10 && date.day > 4 && date.day < 15)
    {
        if (date.day < 10)
            date.day = 4;
        else
            date.day = 15;
    }
}


LRESULT
DatePicker::keyDown(DWORD vkcode, LPARAM flags)
{
    if (!haveFocus)
        return 0;

    if (vkcode >= '0' && vkcode <= '9')
    {
        unsigned int digit = vkcode - '0';

        if (firstDigit)
        {
            switch (selectedField)
            {
            case DayField:
                if (digit != 0)
                    date.day = digit;
                break;
            case MonthField:
                if (digit != 0)
                    date.month = digit;
                break;
            case YearField:
                if (digit != 0)
                    date.year = digit;
                break;
            }
            firstDigit = false;
        }
        else
        {
            switch (selectedField)
            {
            case DayField:
                {
                    unsigned int day = date.day * 10 + digit;
                    if (day >= 10)
                        firstDigit = true;
                    if (day > daysInMonth(date.month, date.year))
                        day = 1;
                    date.day = day;
                }
                break;
            
            case MonthField:
                {
                    unsigned int month = date.month * 10 + digit;
                    if (month > 1)
                        firstDigit = true;
                    if (month > 12)
                        month = 1;
                    date.month = month;
                }
                break;
                
            case YearField:
                {
                    unsigned int year = date.year * 10 + digit;
                    if (year >= 1000)
                        firstDigit = true;
                    if (year <= 9999)
                        date.year = year;
                }
                break;
            }
        }
        clampToValidDate(date);
        notifyDateChanged();
    }
    else if (vkcode == VK_SUBTRACT || vkcode == VK_OEM_MINUS)
    {
        if (selectedField == YearField)
        {
            date.year = -date.year;
            clampToValidDate(date);
            notifyDateChanged();
        }
    }
    else
    {
        firstDigit = true;

        switch (vkcode)
        {
        case VK_LEFT:
            if ((int) selectedField == 0)
                selectedField = DatePickerField((int) NumFields - 1);
            else 
                selectedField = DatePickerField((int) selectedField - 1);
            break;

        case VK_RIGHT:
            if ((int) selectedField == (int) NumFields - 1)
                selectedField = DatePickerField(0);
            else
                selectedField = DatePickerField((int) selectedField + 1);
            break;
            
        case VK_UP:
            incrementField();
            notifyDateChanged();
            break;

        case VK_DOWN:
            decrementField();
            notifyDateChanged();
            break;
            
        default:
            break;
        }
    }

    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
}


LRESULT
DatePicker::leftButtonDown(WORD key, int x, int y)
{
    POINT pt;
    pt.x = x;
    pt.y = y;

    if (PtInRect(&fieldRects[DayField], pt))
        selectedField = DayField;
    else if (PtInRect(&fieldRects[MonthField], pt))
        selectedField = MonthField;
    else if (PtInRect(&fieldRects[YearField], pt))
        selectedField = YearField;

    InvalidateRect(hwnd, NULL, TRUE);
	
	::SetFocus(hwnd); // note that this is the Win32 API function, not the class method

    return 0;
}


LRESULT
DatePicker::setFocus(HWND lostFocus)
{
    if (!haveFocus)
    {
        sendNotify(NM_SETFOCUS);
        haveFocus = true;
    }

    firstDigit = true;

    InvalidateRect(hwnd, NULL, TRUE);
    
    return 0;
}


LRESULT
DatePicker::killFocus(HWND lostFocus)
{
    if (haveFocus)
    {
        sendNotify(NM_KILLFOCUS);
        haveFocus = false;
    }

    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
}


LRESULT
DatePicker::enable(bool b)
{
    if (!b)
        style &= ~WS_DISABLED;
    else
        style |= WS_DISABLED;

    return 0;
}


LRESULT
DatePicker::notify(int id, const NMHDR& nmhdr)
{
    return 0;
}


LRESULT
DatePicker::command(WPARAM, LPARAM)
{
    return 0;
}


bool
DatePicker::sendNotify(UINT code)
{
    NMHDR nmhdr;

    ZeroMemory(&nmhdr, sizeof(nmhdr));
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom   = GetWindowLongPtr(hwnd, GWLP_ID);
    nmhdr.code     = code;

    return SendMessage(parent, WM_NOTIFY,
                       nmhdr.idFrom,
                       reinterpret_cast<LPARAM>(&nmhdr)) ? true : false;
}


bool
DatePicker::notifyDateChanged()
{
    NMDATETIMECHANGE change;

    ZeroMemory(&change, sizeof(change));
    change.nmhdr.hwndFrom = hwnd;
    change.nmhdr.idFrom   = GetWindowLongPtr(hwnd, GWLP_ID);
    change.nmhdr.code     = DTN_DATETIMECHANGE;
    change.st.wYear       = date.year;
    change.st.wMonth      = date.month;
    change.st.wDay        = date.day;

    return SendMessage(parent, WM_NOTIFY,
                       change.nmhdr.idFrom,
                       reinterpret_cast<LPARAM>(&change)) ? true : false;
}


int
DatePicker::getFieldWidth(DatePickerField field, HDC hdc)
{
    char* maxWidthText = "\0";

    switch (field)
    {
    case YearField:
        maxWidthText = "-2222 ";
        break;

    case MonthField:
        maxWidthText = " Oct ";
        break;

    case DayField:
        maxWidthText = "22 ";
        break;
    }

    SIZE size;
    GetTextExtentPoint32(hdc, maxWidthText, strlen(maxWidthText), &size);

    return size.cx;
}


void
DatePicker::incrementField()
{
    switch (selectedField)
    {
    case YearField:
        date.year++;
        clampToValidDate(date);
        break;
    case MonthField:
        date.month++;
        if (date.month > 12)
            date.month = 1;
        clampToValidDate(date);
        break;
    case DayField:
        date.day++;
        if (date.day > (int) daysInMonth(date.month, date.year))
            date.day = 1;
        // Skip 10 days deleted in Gregorian calendar reform
        if (date.year == 1582 && date.month == 10 && date.day == 5)
            date.day = 15;
        break;
    }
}


void
DatePicker::decrementField()
{
    switch (selectedField)
    {
    case YearField:
        date.year--;
        clampToValidDate(date);
        break;
    case MonthField:
        date.month--;
        if (date.month < 1)
            date.month = 12;
        clampToValidDate(date);
        break;
    case DayField:
        date.day--;
        if (date.day < 1)
            date.day = daysInMonth(date.month, date.year);
        // Skip 10 days deleted in Gregorian calendar reform
        if (date.year == 1582 && date.month == 10 && date.day == 14)
            date.day = 4;
        break;
    }
}


LRESULT
DatePicker::destroy()
{
    return 0;
}


LRESULT
DatePicker::resize(WORD flags, int width, int height)
{
    clientRect.bottom = height;
    clientRect.right = width;

    InvalidateRect(hwnd, NULL, FALSE);

    return 0;
}


LRESULT
DatePicker::setSystemTime(DWORD flag, SYSTEMTIME* sysTime)
{
    date.year = (int16) sysTime->wYear;
    date.month = sysTime->wMonth;
    date.day = sysTime->wDay;

    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
}


LRESULT
DatePicker::getSystemTime(SYSTEMTIME* sysTime)
{
    if (sysTime != NULL)
    {
        sysTime->wYear = date.year;
        sysTime->wMonth = date.month;
        sysTime->wDay = date.day;
    }
    
    return GDT_VALID;
}


static LRESULT
DatePickerNCCreate(HWND hwnd, CREATESTRUCT& cs)
{
    DWORD exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    
    exStyle |= WS_EX_CLIENTEDGE;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    return DefWindowProc(hwnd, WM_NCCREATE, 0, reinterpret_cast<LPARAM>(&cs));
}


static LRESULT
DatePickerCreate(HWND hwnd, CREATESTRUCT& cs)
{
    DatePicker* dp = new DatePicker(hwnd, cs);
    if (dp == NULL)
        return -1;

    SetWindowLongPtr(hwnd, 0, reinterpret_cast<DWORD_PTR>(dp));

    return 0;
}


static LRESULT WINAPI
DatePickerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DatePicker* dp = reinterpret_cast<DatePicker*>(GetWindowLongPtr(hwnd, 0));

    if (!dp && (uMsg != WM_CREATE) && (uMsg != WM_NCCREATE))
        return DefWindowProc(hwnd, uMsg, wParam, lParam);


    switch (uMsg)
    {
    case DTM_SETSYSTEMTIME:
        return dp->setSystemTime(wParam, reinterpret_cast<SYSTEMTIME*>(lParam));
        break;
        
    case DTM_GETSYSTEMTIME:
        return dp->getSystemTime(reinterpret_cast<SYSTEMTIME*>(lParam));
        break;

    case WM_NOTIFY:
        return dp->notify((int) wParam, *reinterpret_cast<NMHDR*>(lParam));
        break;

    case WM_ENABLE:
        return dp->enable(wParam != 0 ? true : false);
        break;

        //case WM_ERASEBKGND:
        //break;

    case WM_PAINT:
        return dp->paint(reinterpret_cast<HDC>(wParam));
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_KEYDOWN:
        return dp->keyDown(wParam, lParam);
        break;

    case WM_KILLFOCUS:
        return dp->killFocus(reinterpret_cast<HWND>(wParam));
        break;

    case WM_SETFOCUS:
        return dp->setFocus(reinterpret_cast<HWND>(wParam));
        break;

    case WM_NCCREATE:
        return DatePickerNCCreate(hwnd, *reinterpret_cast<CREATESTRUCT*>(lParam));
        break;

    case WM_SIZE:
        return dp->resize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_LBUTTONDOWN:
        return dp->leftButtonDown((WORD) wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_LBUTTONUP:
        return 0;
        break;

    case WM_CREATE:
        return DatePickerCreate(hwnd, *reinterpret_cast<CREATESTRUCT*>(lParam));
        break;

    case WM_DESTROY:
        return dp->destroy();
        break;

    case WM_COMMAND:
        return dp->command(wParam, lParam);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}


void
RegisterDatePicker()
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.style           = CS_GLOBALCLASS;
    wc.lpfnWndProc     = DatePickerProc;
    wc.cbClsExtra      = 0;
    wc.cbWndExtra      = sizeof(DatePicker*);
    wc.hCursor         = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground   = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszClassName   = "CelestiaDatePicker";

    RegisterClass(&wc);
}
