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
#include "res/resource.h"

static LPTSTR hyperLinkFromStatic = "_Hyperlink_From_Static_";
static LPTSTR hyperLinkOriginalProc = "_Hyperlink_Original_Proc_";
static LPTSTR hyperLinkOriginalFont = "_Hyperlink_Original_Font_";
static LPTSTR hyperLinkUnderlineFont = "_Hyperlink_Underline_Font_";

bool GetTextRect(HWND hWnd, RECT* rectText)
{
    bool result = false;
    SIZE sizeText;
    RECT rectControl;
    char staticText[1024];
    HDC hDC;
    HFONT hFont, hOldFont;
    HWND hWndScreen;

    // Get DC of static control and select font so that text extent is computed accurately
    if (hDC = GetDC(hWnd))
    {
        hFont = (HFONT)GetProp(hWnd, hyperLinkOriginalFont);
        hOldFont = (HFONT)SelectObject(hDC, hFont);
        GetWindowText(hWnd, staticText, sizeof(staticText) - 1);
        if (GetTextExtentPoint32(hDC, staticText, strlen(staticText), &sizeText))
        {
            // Construct bounding rectangle of text, assuming text is centered in client area
            if (GetClientRect(hWnd, &rectControl))
            {
                hWndScreen = GetDesktopWindow();
                rectText->left = (rectControl.right - sizeText.cx) / 2;
                rectText->top = (rectControl.bottom - sizeText.cy) / 2;
                rectText->right = rectText->left + sizeText.cx;
                rectText->bottom = rectText->top + sizeText.cy;
                result = true;
            }
        }
        SelectObject(hDC, hOldFont);
        ReleaseDC(hWnd, hDC);
    }

    return result;
}

LRESULT CALLBACK _HyperlinkParentProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC origProc = (WNDPROC)GetProp(hWnd, hyperLinkOriginalProc);

    switch (message)
    {
    case WM_CTLCOLORSTATIC:
        {
            HDC hDC = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;

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
            SetWindowLong(hWnd, GWL_WNDPROC, (LONG)origProc);
            RemoveProp(hWnd, hyperLinkOriginalProc);
            break;
        }
    }

    return CallWindowProc(origProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK _HyperlinkProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC origProc = (WNDPROC)GetProp(hWnd, hyperLinkOriginalProc);

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
                    HFONT hFont = (HFONT)GetProp(hWnd, hyperLinkUnderlineFont);
                    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, FALSE);
                    InvalidateRect(hWnd, NULL, FALSE);
                    SetCapture(hWnd);
                    HCURSOR hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));
                    if (NULL == hCursor)
                        hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
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
                    HFONT hFont = (HFONT)GetProp(hWnd, hyperLinkOriginalFont);
                    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, FALSE);
                    InvalidateRect(hWnd, NULL, FALSE);
                    ReleaseCapture();
                }
            }
            break;
        }
    case WM_DESTROY:
        {
            SetWindowLong(hWnd, GWL_WNDPROC, (LONG)origProc);
            RemoveProp(hWnd, hyperLinkOriginalProc);

            HFONT hOrigFont = (HFONT)GetProp(hWnd, hyperLinkOriginalFont);
            SendMessage(hWnd, WM_SETFONT, (WPARAM) hOrigFont, 0);
            RemoveProp(hWnd, hyperLinkOriginalFont);

            HFONT hFont = (HFONT)GetProp(hWnd, hyperLinkUnderlineFont);
            DeleteObject(hFont);
            RemoveProp(hWnd, hyperLinkUnderlineFont);

            RemoveProp(hWnd, hyperLinkFromStatic);

            break;
        }
    }

    return CallWindowProc(origProc, hWnd, message, wParam, lParam);
}

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
            WNDPROC origProc = (WNDPROC)GetWindowLong(hParent, GWL_WNDPROC);
            if (origProc != _HyperlinkParentProc)
            {
                SetProp(hParent, hyperLinkOriginalProc, (HANDLE)origProc);
                SetWindowLong(hParent, GWL_WNDPROC, (LONG)(WNDPROC)_HyperlinkParentProc);
            }
        }

        // Make sure the control will send notifications.
        DWORD dwStyle = GetWindowLong(hCtrl, GWL_STYLE);
        SetWindowLong(hCtrl, GWL_STYLE, dwStyle | SS_NOTIFY);

        // Subclass the existing control.
        WNDPROC origProc = (WNDPROC)GetWindowLong(hCtrl, GWL_WNDPROC);
        SetProp(hCtrl, hyperLinkOriginalProc, (HANDLE)origProc);
        SetWindowLong(hCtrl, GWL_WNDPROC, (LONG)(WNDPROC)_HyperlinkProc);

        // Create an updated font by adding an underline.
        HFONT hOrigFont = (HFONT) SendMessage(hCtrl, WM_GETFONT, 0, 0);
        SetProp(hCtrl, hyperLinkOriginalFont, (HANDLE)hOrigFont);

        LOGFONT lf;
        GetObject(hOrigFont, sizeof(lf), &lf);
        lf.lfUnderline = TRUE;

        HFONT hFont = CreateFontIndirect(&lf);
        SetProp(hCtrl, hyperLinkUnderlineFont, (HANDLE)hFont);

        // Set a flag on the control so we know what color it should be.
        SetProp(hCtrl, hyperLinkFromStatic, (HANDLE)1);

        result = TRUE;
    }

    return result;
}
