// winsplash.cpp
//
// Copyright (C) 2005, Chris Laurel <claurel@shatters.net>
//
// Win32 splash window
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winsplash.h"

#include <fmt/format.h>
#ifdef _UNICODE
#include <fmt/xchar.h>
#endif

#include <celimage/image.h>
#include <celutil/gettext.h>
#include "res/resource.h"
#include "tstring.h"
#include "winuiutils.h"

namespace celestia::win32
{

namespace
{

constexpr const TCHAR className[] = TEXT("CELSPLASH");

LRESULT CALLBACK
SplashWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static SplashWindow *splash = NULL;

    if (uMsg == WM_CREATE)
    {
        splash = reinterpret_cast<SplashWindow*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
    }

    if (splash)
        return splash->windowProc(hwnd, uMsg, wParam, lParam);
    else
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

} // end unnamed namespace

SplashWindow::SplashWindow(const fs::path& _imageFileName) :
    imageFileName(_imageFileName)
{
    init();
}

SplashWindow::~SplashWindow()
{
    if (hBitmap)
        ::DeleteObject(hBitmap);
    if (hCompositionBitmap)
        ::DeleteObject(hCompositionBitmap);
}

LRESULT CALLBACK
SplashWindow::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
            return TRUE;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam) ;
}

void
SplashWindow::paint(HDC hDC)
{
    RECT rect;
    ::GetClientRect(hwnd, &rect);

    if (hBitmap)
    {
        // Display the splash image
        HDC hMemDC = ::CreateCompatibleDC(hDC);
        auto hOldBitmap = static_cast<HBITMAP>(::SelectObject(hMemDC, hBitmap));

        StretchBlt(hDC, 0, 0, winWidth, winHeight, hMemDC, 0, 0, imageWidth, imageHeight, SRCCOPY);

        ::SelectObject(hMemDC, hOldBitmap);
        ::DeleteDC(hMemDC);

        SetTextColor(hDC, RGB(255, 255, 255));
        SetBkMode(hDC, TRANSPARENT);
    }
    else
    {
        // If the splash image couldn't be loaded, just paint a black background
        HBRUSH hbrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hDC, &rect, hbrush);
        DeleteObject(hbrush);
    }

    // Show the message text
    RECT r;
    r.left   = rect.right - DpToPixels(250, hwnd);
    r.top    = rect.bottom - DpToPixels(70, hwnd);
    r.right  = rect.right;
    r.bottom = r.top + DpToPixels(30, hwnd);

    HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    SelectObject(hDC, hFont);
    DrawText(hDC, message.data(), message.size(), &r, DT_LEFT | DT_VCENTER);
}

void
SplashWindow::init()
{
    hwnd = NULL;

    versionString = UTF8ToTString(fmt::format(_("Version: {}"), VERSION_STRING));
    message = versionString + TEXT('\n');
    image = engine::Image::load(imageFileName);
}

void
SplashWindow::updateWindow()
{
    // Draw into the compositing bitmap and use it as the source for UpdateLayeredWindow.
    HDC hwndDC = GetDC(hwnd);
    HDC hDC = CreateCompatibleDC(hwndDC);

    auto hOldBitmap = static_cast<HBITMAP>(::SelectObject(hDC, hCompositionBitmap));
    paint(hDC);

    SIZE size;
    POINT origin = { 0, 0 };
    size.cx = winWidth;
    size.cy = winHeight;
    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = AC_SRC_ALPHA;
    blend.SourceConstantAlpha = 0xff;
    UpdateLayeredWindow(hwnd, hwndDC, NULL, &size, hDC, &origin, 0, &blend, ULW_ALPHA);

    ::SelectObject(hDC, hOldBitmap);
    ::DeleteDC(hDC);

    ::UpdateWindow(hwnd);
}

HWND
SplashWindow::createWindow()
{
    WNDCLASSEX wndclass;
    wndclass.cbSize         = sizeof (wndclass);
    wndclass.style          = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
    wndclass.lpfnWndProc    = SplashWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = ::GetModuleHandle(NULL);
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = ::LoadCursor( NULL, IDC_WAIT );
    wndclass.hbrBackground  = static_cast<HBRUSH>(::GetStockObject(LTGRAY_BRUSH));
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = className;
    wndclass.hIconSm        = NULL;

    if (!RegisterClassEx(&wndclass))
        return NULL;

    if (image != nullptr)
    {
        imageWidth = image->getWidth();
        imageHeight = image->getHeight();

        winWidth = DpToPixels(imageWidth, nullptr);
        winHeight = DpToPixels(imageHeight, nullptr);
    }

    // Create the application window centered in the middle of the screen
    DWORD nScrWidth  = GetSystemMetricsForWindow(SM_CXFULLSCREEN, nullptr);
    DWORD nScrHeight = GetSystemMetricsForWindow(SM_CYFULLSCREEN, nullptr);

    int x = (nScrWidth  - winWidth) / 2;
    int y = (nScrHeight - winHeight) / 2;
    hwnd = ::CreateWindowEx(WS_EX_TOOLWINDOW, className,
                            TEXT("Banner"), WS_POPUP, x, y,
                            winWidth, winHeight, NULL, NULL, NULL, this);

    if (hwnd)
    {
        createBitmap();

        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

        ShowWindow(hwnd, SW_SHOW);
        updateWindow();
    }

    image.reset();

    return hwnd;
}

bool
SplashWindow::createBitmap()
{
    if (image != NULL)
    {
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth    = image->getWidth();
        bmi.bmiHeader.biHeight   = image->getHeight();
        bmi.bmiHeader.biPlanes   = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = image->getWidth() * image->getHeight() * 4;

        HDC hwndDC = GetDC(hwnd);
        HDC hDC = CreateCompatibleDC(hwndDC);

        void* bmPixels = NULL;

        // create our DIB section and select the bitmap into the dc
        hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &bmPixels, NULL, 0x0);

        // Windows bitmaps are upside-down and the order of the color channels is different
        // than for a PNG. When copying pixels to the bitmap, we remap the color channels and
        // flip the image vertically.
        if (bmPixels != NULL)
        {
            unsigned char* src = image->getPixels();
            unsigned char* dst = reinterpret_cast<unsigned char*>(bmPixels);

            for (unsigned int i = 0; i < (unsigned int) image->getHeight(); i++)
            {
                for (unsigned int j = 0; j < (unsigned int) image->getWidth(); j++)
                {
                    unsigned char* srcPix = &src[4 * (image->getWidth() * (image->getHeight() - i - 1) + j)];
                    unsigned char* dstPix = &dst[4 * (image->getWidth() * i + j)];

                    // Windows requires that we premultiply by alpha
                    unsigned int alpha = srcPix[3];
                    dstPix[0] = (unsigned char) ((srcPix[2] * alpha) / 255);
                    dstPix[1] = (unsigned char) ((srcPix[1] * alpha) / 255);
                    dstPix[2] = (unsigned char) ((srcPix[0] * alpha) / 255);
                    dstPix[3] = srcPix[3];
                }
            }
        }

        DeleteDC(hDC);

        // Create the composition bitmap (always created, though it's only used when we've got
        // layered window support.)
        if (hBitmap)
        {
            hCompositionBitmap = CreateCompatibleBitmap(hwndDC, winWidth, winHeight);
        }
    }

    return hBitmap != 0 && hCompositionBitmap != 0;
}


int
SplashWindow::messageLoop()
{
    if (!hwnd)
        showSplash();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}


// Redraw the window with a new text message
void
SplashWindow::setMessage(std::string_view msg)
{
    message = fmt::format(TEXT("{}\n{}"), versionString, UTF8ToTString(msg));
    InvalidateRect(hwnd, NULL, FALSE);
    updateWindow();
}


void
SplashWindow::showSplash()
{
    close();
    createWindow();
}


int
SplashWindow::close()
{
    if (hwnd)
    {
        DestroyWindow(hwnd);
        hwnd = 0;
        UnregisterClass(className, ::GetModuleHandle(NULL));
        return 1;
    }

    return 0;
}

} // end namespace celestia::win32
