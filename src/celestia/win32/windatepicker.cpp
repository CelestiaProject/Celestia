// windatepicker.cpp
//
// Copyright (C) 2023, Celestia Development Team
//
// Original version:
// Copyright (C) 2005, Chris Laurel <claurel@shatters.net>
//
// Windows front end for Celestia.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "windatepicker.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

#include <fmt/format.h>
#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <celastro/date.h>
#include <celutil/array_view.h>

#include <windows.h>
#include <commctrl.h>

#include "datetimehelpers.h"
#include "tstring.h"

using namespace std::string_view_literals;

namespace celestia::win32
{

namespace
{

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

constexpr bool
isLeapYear(unsigned int year)
{
    return year > 1582
        ? (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
        : (year % 4 == 0);
}

unsigned int
daysInMonth(unsigned int month, unsigned int year)
{
    constexpr std::array<unsigned int, 12> daysPerMonth
    {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    if (month == 2)
        return isLeapYear(year) ? 29 : 28;
    else
        return daysPerMonth[month - 1];
}

void
clampToValidDate(astro::Date& date)
{
    auto days = static_cast<int>(daysInMonth(date.month, date.year));
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

enum class DatePickerField : int
{
    Invalid = -1,
    Day = 0,
    Month,
    Year,
    Count_,
};

DatePickerField&
operator++(DatePickerField& dpf)
{
    switch (dpf)
    {
    case DatePickerField::Invalid:
    case DatePickerField::Count_:
        break;
    case DatePickerField::Year:
        dpf = DatePickerField::Day;
        break;
    default:
        dpf = static_cast<DatePickerField>(static_cast<int>(dpf) + 1);
        break;
    }
    return dpf;
}

DatePickerField&
operator--(DatePickerField& dpf)
{
    switch (dpf)
    {
    case DatePickerField::Invalid:
    case DatePickerField::Count_:
        break;
    case DatePickerField::Day:
        dpf = DatePickerField::Year;
        break;
    default:
        dpf = static_cast<DatePickerField>(static_cast<int>(dpf) - 1);
        break;
    }
    return dpf;
}

constexpr auto NumFields = static_cast<std::size_t>(DatePickerField::Count_);

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

private:
    int getFieldWidth(DatePickerField field, HDC hdc);
    void incrementField();
    void decrementField();

    HWND hwnd;
    HWND parent;
    astro::Date date;
    DatePickerField selectedField;
    HFONT hFont;
    DWORD style;

    bool haveFocus;
    bool firstDigit;

    std::array<RECT, NumFields> fieldRects;
    RECT clientRect;
    util::array_view<tstring> monthNames{ GetLocalizedMonthNames() };
};

DatePicker::DatePicker(HWND _hwnd, CREATESTRUCT& cs) :
    hwnd(_hwnd),
    parent(cs.hwndParent),
    date(1970, 10, 25),
    selectedField(DatePickerField::Year),
    haveFocus(false),
    firstDigit(true)
{
    hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));

    clientRect.left = 0;
    clientRect.right = 0;
    clientRect.top = 0;
    clientRect.bottom = 0;
}

DatePicker::~DatePicker() = default;

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

    fmt::basic_memory_buffer<TCHAR, 32> dayBuf;
    fmt::basic_memory_buffer<TCHAR, 32> yearBuf;

    fmt::format_to(std::back_inserter(dayBuf), TEXT("{:02d}"), date.day);
    fmt::format_to(std::back_inserter(yearBuf), TEXT("{:>5d}"), date.year);

    std::array<tstring_view, 3> fieldText
    {
        tstring_view(dayBuf.data(), dayBuf.size()),
        monthNames[static_cast<std::size_t>(date.month - 1)],
        tstring_view(yearBuf.begin(), yearBuf.size()),
    };

    int right = 2;
    for (unsigned int i = 0; i < NumFields; i++)
    {
        int fieldWidth = getFieldWidth(DatePickerField(i), hdc);
        fieldRects[i].left = right;
        fieldRects[i].right = right + fieldWidth;
        fieldRects[i].top = rect.top;
        fieldRects[i].bottom = rect.bottom;

        right = fieldRects[i].right;

        auto fieldSize = static_cast<int>(fieldText[i].size());
        if (i == static_cast<unsigned int>(selectedField) && haveFocus)
        {
            SIZE size;
            GetTextExtentPoint(hdc, fieldText[i].data(), fieldSize, &size);

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

        DrawText(hdc, fieldText[i].data(), fieldSize, &fieldRects[i], DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
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
            case DatePickerField::Day:
                if (digit != 0)
                    date.day = digit;
                break;
            case DatePickerField::Month:
                if (digit != 0)
                    date.month = digit;
                break;
            case DatePickerField::Year:
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
            case DatePickerField::Day:
                {
                    unsigned int day = date.day * 10 + digit;
                    if (day >= 10)
                        firstDigit = true;
                    if (day > daysInMonth(date.month, date.year))
                        day = 1;
                    date.day = day;
                }
                break;

            case DatePickerField::Month:
                {
                    unsigned int month = date.month * 10 + digit;
                    if (month > 1)
                        firstDigit = true;
                    if (month > 12)
                        month = 1;
                    date.month = month;
                }
                break;

            case DatePickerField::Year:
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
        if (selectedField == DatePickerField::Year)
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
            --selectedField;
            break;

        case VK_RIGHT:
            ++selectedField;
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

    auto dpf = DatePickerField::Day;
    do
    {
        if (PtInRect(&fieldRects[static_cast<std::size_t>(dpf)], pt))
        {
            selectedField = dpf;
            break;
        }
        ++dpf;
    } while (dpf != DatePickerField::Year);

    InvalidateRect(hwnd, nullptr, TRUE);

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
    constexpr std::array<tstring_view, 3> fieldTexts
    {
        TEXT("22 "sv), TEXT(" Oct "sv), TEXT("-2222 "sv)
    };

    tstring_view maxWidthText = fieldTexts[static_cast<std::size_t>(field)];

    SIZE size;
    GetTextExtentPoint32(hdc, maxWidthText.data(), static_cast<int>(maxWidthText.size()), &size);

    return size.cx;
}

void
DatePicker::incrementField()
{
    switch (selectedField)
    {
    case DatePickerField::Day:
        date.day++;
        if (date.day > static_cast<int>(daysInMonth(date.month, date.year)))
            date.day = 1;
        // Skip 10 days deleted in Gregorian calendar reform
        if (date.year == 1582 && date.month == 10 && date.day == 5)
            date.day = 15;
        break;
    case DatePickerField::Month:
        date.month++;
        if (date.month > 12)
            date.month = 1;
        clampToValidDate(date);
        break;
    case DatePickerField::Year:
        ++date.year;
        clampToValidDate(date);
        break;
    default:
        assert(0);
        break;
    }
}

void
DatePicker::decrementField()
{
    switch (selectedField)
    {
    case DatePickerField::Day:
        date.day--;
        if (date.day < 1)
            date.day = daysInMonth(date.month, date.year);
        // Skip 10 days deleted in Gregorian calendar reform
        if (date.year == 1582 && date.month == 10 && date.day == 14)
            date.day = 4;
        break;
    case DatePickerField::Month:
        date.month--;
        if (date.month < 1)
            date.month = 12;
        clampToValidDate(date);
        break;
    case DatePickerField::Year:
        date.year--;
        clampToValidDate(date);
        break;
    default:
        assert(0);
        break;
    }
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
    date.year = static_cast<std::int16_t>(sysTime->wYear);
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

LRESULT
DatePickerNCCreate(HWND hwnd, CREATESTRUCT& cs)
{
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    exStyle |= WS_EX_CLIENTEDGE;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    return DefWindowProc(hwnd, WM_NCCREATE, 0, reinterpret_cast<LPARAM>(&cs));
}

LRESULT
DatePickerCreate(HWND hwnd, CREATESTRUCT& cs)
{
    auto dp = new DatePicker(hwnd, cs);
    SetWindowLongPtr(hwnd, 0, reinterpret_cast<LONG_PTR>(dp));

    return 0;
}

LRESULT
DatePickerDestroy(HWND hwnd, DatePicker* dp)
{
    SetWindowLongPtr(hwnd, 0, LONG_PTR(0));
    delete dp;
    return 0;
}

LRESULT WINAPI
DatePickerProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto dp = reinterpret_cast<DatePicker*>(GetWindowLongPtr(hwnd, 0));

    if (!dp && (uMsg != WM_CREATE) && (uMsg != WM_NCCREATE))
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case DTM_SETSYSTEMTIME:
        return dp->setSystemTime(wParam, reinterpret_cast<SYSTEMTIME*>(lParam));

    case DTM_GETSYSTEMTIME:
        return dp->getSystemTime(reinterpret_cast<SYSTEMTIME*>(lParam));

    case WM_NOTIFY:
        return dp->notify(static_cast<int>(wParam), *reinterpret_cast<NMHDR*>(lParam));

    case WM_ENABLE:
        return dp->enable(wParam != 0 ? true : false);

    case WM_PAINT:
        return dp->paint(reinterpret_cast<HDC>(wParam));

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_KEYDOWN:
        return dp->keyDown(wParam, lParam);

    case WM_KILLFOCUS:
        return dp->killFocus(reinterpret_cast<HWND>(wParam));

    case WM_SETFOCUS:
        return dp->setFocus(reinterpret_cast<HWND>(wParam));

    case WM_NCCREATE:
        return DatePickerNCCreate(hwnd, *reinterpret_cast<CREATESTRUCT*>(lParam));

    case WM_SIZE:
        return dp->resize(wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_LBUTTONDOWN:
        return dp->leftButtonDown(static_cast<WORD>(wParam), LOWORD(lParam), HIWORD(lParam));

    case WM_LBUTTONUP:
        return 0;

    case WM_CREATE:
        return DatePickerCreate(hwnd, *reinterpret_cast<CREATESTRUCT*>(lParam));

    case WM_DESTROY:
        return DatePickerDestroy(hwnd, dp);

    case WM_COMMAND:
        return dp->command(wParam, lParam);

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

} // end unnamed namespace

void
RegisterDatePicker()
{
    WNDCLASS wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.style           = CS_GLOBALCLASS;
    wc.lpfnWndProc     = &DatePickerProc;
    wc.cbClsExtra      = 0;
    wc.cbWndExtra      = sizeof(DatePicker*);
    wc.hCursor         = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground   = reinterpret_cast<HBRUSH>(INT_PTR(COLOR_WINDOW + 1));
    wc.lpszClassName   = TEXT("CelestiaDatePicker");

    RegisterClass(&wc);
}

} // end namespace celestia::win32
