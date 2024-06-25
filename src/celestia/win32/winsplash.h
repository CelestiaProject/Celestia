// winsplash.h
//
// Copyright (C) 2005, Chris Laurel <claurel@shatters.net>
//
// Win32 splash window
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <memory>
#include <string_view>

#include <windows.h>

#include <celcompat/filesystem.h>
#include "tstring.h"

namespace celestia
{

namespace engine
{
class Image;
} // end namespace celestia::engine

namespace win32
{

class SplashWindow
{
public:
    SplashWindow(const fs::path& _imageFileName);
    ~SplashWindow();

    void showSplash();
    int close();
    void setMessage(std::string_view msg);

    LRESULT CALLBACK windowProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void init();
    void paint(HDC hDC);
    HWND createWindow();
    int messageLoop();
    bool createBitmap();
    void updateWindow();

    HWND hwnd{ NULL };
    fs::path imageFileName;
    std::unique_ptr<celestia::engine::Image> image;
    HBITMAP hBitmap{ NULL };
    HBITMAP hCompositionBitmap{ NULL };
    tstring versionString;
    tstring message;
    unsigned int winWidth{ 640U };
    unsigned int winHeight{ 480U };
    unsigned int imageWidth{ 0U };
    unsigned int imageHeight{ 0U };
};

} // end namespace celestia::win32

} // end namespace celestia
