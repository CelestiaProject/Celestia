// timer.cpp
//
// Copyright (C) 2018, Celestia Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.


#include "timer.h"

double Timer::getTime() const
{
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = now - start;
    return fp_ms.count() / 1000.0;
}
