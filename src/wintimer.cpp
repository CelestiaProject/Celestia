// wintimer.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <windows.h>
#include "timer.h"

class WindowsTimer : public Timer
{
public:
    WindowsTimer();
    ~WindowsTimer();
    double getTime() const;
    void reset();

private:
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
};


WindowsTimer::WindowsTimer()
{
    QueryPerformanceFrequency(&freq);
    reset();
}

WindowsTimer::~WindowsTimer()
{
}

double WindowsTimer::getTime() const
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double) (t.QuadPart - start.QuadPart) / (double) freq.QuadPart;
}

void WindowsTimer::reset()
{
    QueryPerformanceCounter(&start);
}

Timer* CreateTimer()
{
    return new WindowsTimer();
}
