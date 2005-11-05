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
#include "celengine/astro.h"


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

    LRESULT setSystemTime(DWORD flag, SYSTEMTIME* sysTime);

    LRESULT destroy();

private:
    int getFieldWidth(DatePickerField field, HDC hdc);

private:
    HWND hwnd;
    HWND parent;
    astro::Date date;
    DatePickerField selectedField;
    char textBuffer[64];
    HFONT hFont;
    DWORD style;

    bool haveFocus;

    RECT fieldRects[NumFields];
    RECT clientRect;
};


DatePicker::DatePicker(HWND _hwnd, CREATESTRUCT& cs) :
    hwnd(_hwnd),
    parent(cs.hwndParent),
    date(1970, 10, 25),
    selectedField(YearField),
    haveFocus(false)
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

    sprintf(dayBuf, "%02d ", date.day);
    sprintf(monthBuf, "%s ", Months[date.month - 1]);
    sprintf(yearBuf, "%5d ", date.year);

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
            HBRUSH hbrush = CreateSolidBrush(RGB(255, 255, 0));
            FillRect(hdc, &fieldRects[i], hbrush);
            DeleteObject(hbrush);
        }

        DrawText(hdc, fieldText[i], strlen(fieldText[i]), &fieldRects[i], DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

#if 0
    if (style & DTS_UPDOWN)
    {
        DrawFrameControl(hdc,
                         0,
                         0,
                         (style & WS_DISABLED ? DFCS_INACTIVE : 0));
    }
#endif
}


LRESULT
DatePicker::keyDown(DWORD vkcode, LPARAM flags)
{
    if (!haveFocus)
        return 0;

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

    default:
        break;
    }

    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
}


LRESULT
DatePicker::leftButtonDown(WORD key, int x, int y)
{
    setFocus(hwnd);

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
        maxWidthText = "Oct ";
        break;

    case DayField:
        maxWidthText = "22 ";
        break;
    }

    SIZE size;
    GetTextExtentPoint32(hdc, maxWidthText, strlen(maxWidthText), &size);

    return size.cx;
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
    date.year = sysTime->wYear;
    date.month = sysTime->wMonth;
    date.day = sysTime->wDay;

    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
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
