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

#include <windows.h>
#include <string>
#include <celengine/image.h>


class SplashWindow
{
public:
    SplashWindow(const std::string& _imageFileName);
    ~SplashWindow();

    LRESULT CALLBACK windowProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void init();
    void paint(HDC hDC);
    HWND createWindow();
    int messageLoop();
    void showSplash();
    int close();
    bool createBitmap();
    void updateWindow();
    
    void setMessage(const std::string& msg);
    
private:
    HWND hwnd;
    char* className;
    std::string imageFileName;
    Image* image;
    HBITMAP hBitmap;
    HBITMAP hCompositionBitmap;
    bool useLayeredWindow;
    std::string message;
    unsigned int winWidth;
    unsigned int winHeight;
};
