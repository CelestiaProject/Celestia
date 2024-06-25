// winhyperlinks.cpp
//
// Copyright (C) 2003, Chris Laurel <claurel@shatters.net>
//
// Code to convert a static control to a hyperlink.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winhyperlinks.h"

#include <array>

#include "res/resource.h"

namespace celestia::win32
{

namespace
{

constexpr TCHAR hyperLinkFromStatic[] = TEXT("_Hyperlink_From_Static_");
constexpr TCHAR hyperLinkOriginalProc[] = TEXT("_Hyperlink_Original_Proc_");
constexpr TCHAR hyperLinkOriginalFont[] = TEXT("_Hyperlink_Original_Font_");
constexpr TCHAR hyperLinkUnderlineFont[] = TEXT("_Hyperlink_Underline_Font_");

bool
GetTextRect(HWND hWnd, RECT* rectText)
{

    // Get DC of static control and select font so that text extent is computed accurately
    HDC hDC = GetDC(hWnd);
    if (!hDC)
        return false;

    auto hFont = static_cast<HFONT>(GetProp(hWnd, hyperLinkOriginalFont));
    auto hOldFont = static_cast<HFONT>(SelectObject(hDC, hFont));

    bool result = false;
    std::array<TCHAR, 1024> staticText;
    int length = GetWindowText(hWnd, staticText.data(), staticText.size());

    if (SIZE sizeText; GetTextExtentPoint32(hDC, staticText.data(), length, &sizeText))
    {
        // Construct bounding rectangle of text, assuming text is centered in client area
        RECT rectControl;
        if (GetClientRect(hWnd, &rectControl))
        {
            HWND hWndScreen = GetDesktopWindow();
            rectText->left = (rectControl.right - sizeText.cx) / 2;
            rectText->top = (rectControl.bottom - sizeText.cy) / 2;
            rectText->right = rectText->left + sizeText.cx;
            rectText->bottom = rectText->top + sizeText.cy;
            result = true;
        }
    }

    SelectObject(hDC, hOldFont);
    ReleaseDC(hWnd, hDC);

    return result;
}

LRESULT CALLBACK
HyperlinkParentProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC origProc = (WNDPROC)GetProp(hWnd, hyperLinkOriginalProc);

    switch (message)
    {
    case WM_CTLCOLORSTATIC:
        {
            auto hDC = reinterpret_cast<HDC>(wParam);
            auto hCtrl = reinterpret_cast<HWND>(lParam);

            // Change the color of the static text to hyperlink color (blue).
            if (GetProp(hCtrl, hyperLinkFromStatic) != NULL)
            {
                LRESULT result = CallWindowProc(origProc, hWnd, message, wParam, lParam);
                SetTextColor(hDC, RGB(0, 0, 192));
                return result;
            }

            break;
        }
    case WM_DESTROY:
        {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)origProc);
            RemoveProp(hWnd, hyperLinkOriginalProc);
            break;
        }
    }

    return CallWindowProc(origProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK
HyperlinkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto origProc = static_cast<WNDPROC>(GetProp(hWnd, hyperLinkOriginalProc));

    switch (message)
    {
    case WM_MOUSEMOVE:
        {
            if (GetCapture() != hWnd)
            {
                RECT rect;
                if (!GetTextRect(hWnd, &rect))
                    GetClientRect(hWnd, &rect);

                POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                if (PtInRect(&rect, pt))
                {
                    auto hFont = static_cast<HFONT>(GetProp(hWnd, hyperLinkUnderlineFont));
                    SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                    InvalidateRect(hWnd, NULL, FALSE);
                    SetCapture(hWnd);
                    HCURSOR hCursor = LoadCursor(NULL, IDC_HAND);
                    if (NULL == hCursor)
                        hCursor = LoadCursor(NULL, IDC_ARROW);
                    SetCursor(hCursor);
                }
            }
            else
            {
                RECT rect;
                if (!GetTextRect(hWnd, &rect))
                    GetClientRect(hWnd, &rect);

                POINT pt = { LOWORD(lParam), HIWORD(lParam) };
                if (!PtInRect(&rect, pt))
                {
                    auto hFont = static_cast<HFONT>(GetProp(hWnd, hyperLinkOriginalFont));
                    SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), FALSE);
                    InvalidateRect(hWnd, NULL, FALSE);
                    ReleaseCapture();
                }
            }
            break;
        }
    case WM_DESTROY:
        {
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(origProc));
            RemoveProp(hWnd, hyperLinkOriginalProc);

            auto hOrigFont = static_cast<HFONT>(GetProp(hWnd, hyperLinkOriginalFont));
            SendMessage(hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(hOrigFont), 0);
            RemoveProp(hWnd, hyperLinkOriginalFont);

            auto hFont = static_cast<HFONT>(GetProp(hWnd, hyperLinkUnderlineFont));
            DeleteObject(hFont);
            RemoveProp(hWnd, hyperLinkUnderlineFont);

            RemoveProp(hWnd, hyperLinkFromStatic);

            break;
        }
    }

    return CallWindowProc(origProc, hWnd, message, wParam, lParam);
}

} // end unnamed namespace

BOOL MakeHyperlinkFromStaticCtrl(HWND hDlg, UINT ctrlID)
{
    BOOL result = FALSE;

    HWND hCtrl = GetDlgItem(hDlg, ctrlID);
    if (hCtrl)
    {
        // Subclass the parent so we can color the controls as we desire.
        HWND hParent = GetParent(hCtrl);
        if (hParent)
        {
            auto origProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hParent, GWLP_WNDPROC));
            if (origProc != HyperlinkParentProc)
            {
                SetProp(hParent, hyperLinkOriginalProc, (HANDLE)origProc);
                SetWindowLongPtr(hParent, GWLP_WNDPROC,
                                 reinterpret_cast<LONG_PTR>(static_cast<WNDPROC>(HyperlinkParentProc)));
            }
        }

        // Make sure the control will send notifications.
        LONG_PTR dwStyle = GetWindowLongPtr(hCtrl, GWL_STYLE);
        SetWindowLongPtr(hCtrl, GWL_STYLE, dwStyle | SS_NOTIFY);

        // Subclass the existing control.
        auto origProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hCtrl, GWLP_WNDPROC));
        SetProp(hCtrl, hyperLinkOriginalProc, static_cast<HANDLE>(origProc));
        SetWindowLongPtr(hCtrl, GWLP_WNDPROC,
                         reinterpret_cast<LONG_PTR>(static_cast<WNDPROC>(HyperlinkProc)));

        // Create an updated font by adding an underline.
        auto hOrigFont = reinterpret_cast<HFONT>(SendMessage(hCtrl, WM_GETFONT, 0, 0));
        SetProp(hCtrl, hyperLinkOriginalFont, static_cast<HANDLE>(hOrigFont));

        LOGFONT lf;
        GetObject(hOrigFont, sizeof(lf), &lf);
        lf.lfUnderline = TRUE;

        HFONT hFont = CreateFontIndirect(&lf);
        SetProp(hCtrl, hyperLinkUnderlineFont, static_cast<HANDLE>(hFont));

        // Set a flag on the control so we know what color it should be.
        SetProp(hCtrl, hyperLinkFromStatic, reinterpret_cast<HANDLE>(LONG_PTR(1)));

        result = TRUE;
    }

    return result;
}

} // end namespace celestia::win32
